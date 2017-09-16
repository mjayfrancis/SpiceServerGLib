#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include <spice.h>

#include "ssg-server.h"
#include "ssg-enum-types.h"


#define G_SSG_SERVER_ERROR g_ssg_server_error_quark()

GQuark g_ssg_server_error_quark()
{
    return g_quark_from_static_string ("g-spiceserver-error-quark");
}

enum GSsgServerError {
    G_SSG_SERVER_ERROR_INIT
};

typedef struct _SsgServerPrivate {
    GError *init_error;

    /* Temporary property storage during construction */
    gint addr_flags;
    gint ticket_lifetime;
    gboolean tls_set;
    gint tls_port;
    gchar *tls_ca_cert_file;
    gchar *tls_certs_file;
    gchar *tls_private_key_file;
    gchar *tls_key_passwd;
    gchar *tls_dh_key_file;
    gchar *tls_ciphersuite;

    /* Constructed Spice server */
    SpiceServer *server;
    GList *interfaces;
} SsgServerPrivate;


static gboolean ssg_server_initable_init (GInitable *initable, GCancellable *cancellable, GError **error);

static void
ssg_server_initable_iface_init (GInitableIface *iface)
{
    iface->init = ssg_server_initable_init;
}

G_DEFINE_TYPE_EXTENDED (SsgServer, ssg_server, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE(SsgServer)
                        G_IMPLEMENT_INTERFACE(G_TYPE_INITABLE, ssg_server_initable_iface_init))

enum
{
    PROP_0,
    PROP_COMPAT_VERSION,
    PROP_PORT,
    PROP_ADDR_FLAGS,
    PROP_ADDR,
    PROP_EXIT_ON_DISCONNECT,
    PROP_NOAUTH,
    PROP_SASL,
    PROP_SASL_APPNAME,
    PROP_TICKET_LIFETIME,           // Must be before TICKET_PASSWD
    PROP_TICKET_PASSWD,
    PROP_TLS_KEY_PASSWD,            // Must be before the mandatory TLS arguments (TLS_PORT, ...)
    PROP_TLS_DH_KEY_FILE,           // Must be before the mandatory TLS arguments (TLS_PORT, ...)
    PROP_TLS_CIPHERSUITE,           // Must be before the mandatory TLS arguments (TLS_PORT, ...)
    PROP_TLS_PORT,
    PROP_TLS_CA_CERT_FILE,
    PROP_TLS_CERTS_FILE,
    PROP_TLS_PRIVATE_KEY_FILE,
    PROP_IMAGE_COMPRESSION,
    PROP_JPEG_COMPRESSION,
    PROP_ZLIB_GLZ_COMPRESSION,
    PROP_DEFAULT_CHANNEL_SECURITY,  // Must be before SSL_CHANNELS and INSECURE_CHANNELS
    PROP_SSL_CHANNELS,
    PROP_INSECURE_CHANNELS,
    PROP_STREAMING_VIDEO,
    PROP_VIDEO_CODECS,
    PROP_PLAYBACK_COMPRESSION,
    PROP_AGENT_MOUSE,
    PROP_AGENT_COPYPASTE,
    PROP_AGENT_FILE_XFER,
    PROP_NAME,
    PROP_UUID,
    PROP_SEAMLESS_MIGRATION,
    PROP_CHAR_DEVICE_RECOGNIZED_SUBTYPES,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

struct SpiceTimer {
    GMainContext *context;
    SpiceTimerFunc func;
    void *opaque;
    GSource *source;
};

struct SpiceWatch {
    GMainContext *context;
    void *opaque;
    GSource *source;
    GIOChannel *channel;
    SpiceWatchFunc func;
};

static SpiceTimer* timer_add(SpiceTimerFunc func, void *opaque)
{
    SpiceTimer *timer = calloc(1, sizeof(SpiceTimer));

    timer->context = g_main_context_default();
    timer->func = func;
    timer->opaque = opaque;

    return timer;
}

static void timer_cancel(SpiceTimer *timer)
{
    if (timer->source) {
        g_source_destroy(timer->source);
        g_source_unref(timer->source);
        timer->source = NULL;
    }
}

static gboolean timer_func(gpointer user_data)
{
    SpiceTimer *timer = user_data;

    timer->func(timer->opaque);
    /* timer might be free after func(), don't touch */

    return FALSE;
}

static void timer_start(SpiceTimer *timer, uint32_t ms)
{
    timer_cancel(timer);

    timer->source = g_timeout_source_new(ms);
    assert(timer->source != NULL);

    g_source_set_callback(timer->source, timer_func, timer, NULL);

    g_source_attach(timer->source, timer->context);
}

static void timer_remove(SpiceTimer *timer)
{
    timer_cancel(timer);
    assert(timer->source == NULL);
    free(timer);
}

static void watch_update_mask(SpiceWatch *watch, int event_mask);

static SpiceWatch *watch_add(int fd, int event_mask, SpiceWatchFunc func, void *opaque)
{
    SpiceWatch *watch;

    g_return_val_if_fail(fd != -1, NULL);
    g_return_val_if_fail(func != NULL, NULL);

    watch = calloc (1,sizeof(SpiceWatch));
    watch->context = g_main_context_default();
    watch->channel = g_io_channel_unix_new(fd);
    watch->func = func;
    watch->opaque = opaque;

    watch_update_mask(watch, event_mask);

    return watch;
}

static GIOCondition spice_event_to_giocondition(int event_mask)
{
    GIOCondition condition = 0;

    if (event_mask & SPICE_WATCH_EVENT_READ)
        condition |= G_IO_IN;
    if (event_mask & SPICE_WATCH_EVENT_WRITE)
        condition |= G_IO_OUT;

    return condition;
}

static int giocondition_to_spice_event(GIOCondition condition)
{
    int event = 0;

    if (condition & G_IO_IN)
        event |= SPICE_WATCH_EVENT_READ;
    if (condition & G_IO_OUT)
        event |= SPICE_WATCH_EVENT_WRITE;

    return event;
}

static gboolean watch_func(GIOChannel *source, GIOCondition condition, gpointer data)
{
    SpiceWatch *watch = data;
    int fd = g_io_channel_unix_get_fd(source);

    watch->func(fd, giocondition_to_spice_event(condition), watch->opaque);

    return TRUE;
}

static void watch_update_mask(SpiceWatch *watch, int event_mask)
{
    if (watch->source) {
        g_source_destroy(watch->source);
        g_source_unref(watch->source);
        watch->source = NULL;
    }

    if (!event_mask)
        return;

    watch->source = g_io_create_watch(watch->channel, spice_event_to_giocondition(event_mask));
    g_source_set_callback(watch->source, (GSourceFunc)watch_func, watch, NULL);
    g_source_attach(watch->source, watch->context);
}

static void watch_remove(SpiceWatch *watch)
{
    watch_update_mask(watch, 0);
    assert(watch->source == NULL);

    g_io_channel_unref(watch->channel);
    free(watch);
}

static void event_loop_channel_event(int event, SpiceChannelEventInfo *info)
{

}

static SpiceCoreInterface core = {
    .base = {
        .major_version = SPICE_INTERFACE_CORE_MAJOR,
        .minor_version = SPICE_INTERFACE_CORE_MINOR,
    },

    .timer_add = timer_add,
    .timer_start = timer_start,
    .timer_cancel = timer_cancel,
    .timer_remove = timer_remove,

    .watch_add = watch_add,
    .watch_update_mask = watch_update_mask,
    .watch_remove = watch_remove,

    .channel_event = event_loop_channel_event,
};

static void ignore_sigpipe(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigfillset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);
}


static void
ssg_server_init (SsgServer *object)
{
    SsgServerPrivate *priv = ssg_server_get_instance_private (object);

    // TODO is this sensible to do here?
    ignore_sigpipe();

    SpiceServer* server = spice_server_new();

    priv->server = server;
    priv->interfaces = NULL;

}

static void
ssg_server_finalize (GObject *object)
{
    SsgServer *server = SSG_SERVER(object);
    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    ssg_server_destroy(server);

    g_free(priv->tls_ca_cert_file);
    g_free(priv->tls_certs_file);
    g_free(priv->tls_private_key_file);
    g_free(priv->tls_key_passwd);
    g_free(priv->tls_dh_key_file);
    g_free(priv->tls_ciphersuite);

    G_OBJECT_CLASS (ssg_server_parent_class)->finalize (object);

}

static gboolean
ssg_server_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
    SsgServerPrivate *priv = ssg_server_get_instance_private (SSG_SERVER(initable));

    g_return_val_if_fail (priv->server != NULL, FALSE);

    if (priv->init_error == NULL) {
        if (!priv->tls_set && (
                priv->tls_port != -1 ||
                priv->tls_ca_cert_file != NULL ||
                priv->tls_certs_file != NULL ||
                priv->tls_private_key_file != NULL ||
                priv->tls_key_passwd != NULL ||
                priv->tls_dh_key_file != NULL ||
                priv->tls_ciphersuite != NULL))
        {
            priv->init_error = g_error_new (G_SSG_SERVER_ERROR, G_SSG_SERVER_ERROR_INIT, "Missing TLS parameter");
        }
    }

    if (priv->init_error != NULL) {
        g_propagate_error(error, priv->init_error);
        return FALSE;
    }
    return TRUE;
}


static void
ssg_server_constructed (GObject *object)
{

    SsgServerPrivate *priv = ssg_server_get_instance_private (SSG_SERVER(object));

    spice_server_init(priv->server, &core);

    G_OBJECT_CLASS (ssg_server_parent_class)->constructed (object);

}

static void
ssg_server_set_property (GObject *object,
              guint property_id,
              const GValue *value,
              GParamSpec *pspec)
{
    SsgServerPrivate *priv = ssg_server_get_instance_private (SSG_SERVER(object));

    g_return_if_fail (priv->server != NULL);

    int result = -1;

    switch (property_id) {
        case PROP_COMPAT_VERSION:
            result = spice_server_set_compat_version(priv->server, g_value_get_enum(value));
            break;

        case PROP_PORT:
            {
                result = 0;
                GValue *unboxed_value = g_value_get_boxed(value);
                if (unboxed_value != NULL) {
                    result = -1;
                    if (G_VALUE_HOLDS_INT(unboxed_value)) {
                        result = spice_server_set_port(priv->server, g_value_get_int(unboxed_value));
                    }
                }
            }
            break;

        case PROP_ADDR_FLAGS:
            priv->addr_flags = g_value_get_flags(value);
            result = 0;
            break;

        case PROP_ADDR:
            {
                const gchar *addr = g_value_get_string(value);
                if (addr)
                    spice_server_set_addr(priv->server, addr, priv->addr_flags);
                result = 0;
            }
            break;

        case PROP_EXIT_ON_DISCONNECT:
            result = spice_server_set_exit_on_disconnect(priv->server, g_value_get_boolean(value));
            break;

        case PROP_NOAUTH:
            if (g_value_get_boolean(value))
                result = spice_server_set_noauth(priv->server);
            else
                result = 0;
            break;

        case PROP_SASL:
            if (g_value_get_boolean(value))
                result = spice_server_set_sasl(priv->server, 1);
            else
                result = 0;
            break;

        case PROP_SASL_APPNAME:
            {
                const gchar *sasl_appname = g_value_get_string(value);
                if (sasl_appname != NULL)
                    result = spice_server_set_sasl_appname(priv->server, sasl_appname);
                else
                    result = 0;
            }
            break;

        case PROP_TICKET_LIFETIME:
            priv->ticket_lifetime = g_value_get_int(value);
            result = 0;
            break;

        case PROP_TICKET_PASSWD:
            {
                const gchar *ticket_passwd = g_value_get_string(value);
                if (ticket_passwd != NULL)
                    result = spice_server_set_ticket(priv->server, ticket_passwd, priv->ticket_lifetime, FALSE, FALSE);
                else
                    result = 0;
            }
            break;

        case PROP_TLS_KEY_PASSWD:
            priv->tls_key_passwd = g_value_dup_string(value);
            result = 0;
            break;

        case PROP_TLS_DH_KEY_FILE:
            priv->tls_dh_key_file = g_value_dup_string(value);
            result = 0;
            break;

        case PROP_TLS_CIPHERSUITE:
            priv->tls_ciphersuite = g_value_dup_string(value);
            result = 0;
            break;

        case PROP_TLS_PORT:
            priv->tls_port = g_value_get_int(value);
            result = 0;
            break;

        case PROP_TLS_CA_CERT_FILE:
            priv->tls_ca_cert_file = g_value_dup_string(value);
            result = 0;
            break;

        case PROP_TLS_CERTS_FILE:
            priv->tls_certs_file = g_value_dup_string(value);
            result = 0;
            break;

        case PROP_TLS_PRIVATE_KEY_FILE:
            priv->tls_private_key_file = g_value_dup_string(value);
            result = 0;
            if (priv->tls_port != -1 && priv->tls_ca_cert_file != NULL && priv->tls_certs_file != NULL && priv->tls_private_key_file != NULL) {
                result = spice_server_set_tls(priv->server, priv->tls_port, priv->tls_ca_cert_file, priv->tls_certs_file, priv->tls_private_key_file, priv->tls_key_passwd, priv->tls_dh_key_file, priv->tls_ciphersuite);
                priv->tls_set = TRUE;
            }
            break;

        case PROP_IMAGE_COMPRESSION:
            result = spice_server_set_image_compression(priv->server, g_value_get_enum(value));
            break;

        case PROP_JPEG_COMPRESSION:
            result = spice_server_set_jpeg_compression(priv->server, g_value_get_enum(value));
            break;

        case PROP_ZLIB_GLZ_COMPRESSION:
            result = spice_server_set_zlib_glz_compression(priv->server, g_value_get_enum(value));
            break;

        case PROP_DEFAULT_CHANNEL_SECURITY:
            result = spice_server_set_channel_security(priv->server, NULL, g_value_get_enum(value));
            break;

        case PROP_SSL_CHANNELS:
            {
                result = 0;
                const char *channel_list = g_value_get_string(value);
                if (channel_list != NULL)
                {
                    gchar **channels = g_strsplit(channel_list, ",", 0);
                    for (int i = 0; channels[i] != NULL; i++)
                    {
                        int set_result = spice_server_set_channel_security(priv->server, channels[i], SPICE_CHANNEL_SECURITY_SSL);
                        if (set_result != 0)
                            result = -1;
                    }
                    g_strfreev(channels);
                }
            }
            break;

        case PROP_INSECURE_CHANNELS:
            {
                result = 0;
                const char *channel_list = g_value_get_string(value);
                if (channel_list != NULL)
                {
                    gchar **channels = g_strsplit(channel_list, ",", 0);
                    for (int i = 0; channels[i] != NULL; i++)
                    {
                        int set_result = spice_server_set_channel_security(priv->server, channels[i], SPICE_CHANNEL_SECURITY_NONE);
                        if (set_result != 0)
                            result = -1;
                    }
                    g_strfreev(channels);
                }
            }
            break;

        case PROP_STREAMING_VIDEO:
            result = spice_server_set_streaming_video(priv->server, g_value_get_enum(value));
            break;

        case PROP_VIDEO_CODECS:
            {
                const gchar *video_codecs = g_value_get_string(value);
                if (video_codecs)
                    result = spice_server_set_video_codecs(priv->server, video_codecs);
                else
                    result = 0;
            }
            break;

        case PROP_PLAYBACK_COMPRESSION:
            result = spice_server_set_playback_compression(priv->server, g_value_get_boolean(value));
            break;

        case PROP_AGENT_MOUSE:
            result = spice_server_set_agent_mouse(priv->server, g_value_get_boolean(value));
            break;

        case PROP_AGENT_COPYPASTE:
            result = spice_server_set_agent_copypaste(priv->server, g_value_get_boolean(value));
            break;

        case PROP_AGENT_FILE_XFER:
            result = spice_server_set_agent_file_xfer(priv->server, g_value_get_boolean(value));
            break;

        case PROP_NAME:
            {
                const gchar *name = g_value_get_string(value);
                if (name)
                    spice_server_set_name(priv->server, name);
                result = 0;
            }
            break;

        case PROP_UUID:
            {
                GBytes *bytes = g_value_get_boxed(value);
                if (bytes != NULL)
                {
                    if (g_bytes_get_size(bytes) == 16)
                    {
                        spice_server_set_uuid(priv->server, g_bytes_get_data(bytes, NULL));
                        result = 0;
                    }

                } else {
                    result = 0;
                }
            }
            break;

        case PROP_SEAMLESS_MIGRATION:
            spice_server_set_seamless_migration(priv->server, g_value_get_boolean(value));
            result = 0;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }

    if (priv->init_error == NULL && result == -1)
        priv->init_error = g_error_new (G_SSG_SERVER_ERROR, G_SSG_SERVER_ERROR_INIT, "Failed to set server property: %s", pspec->name);
}

static void
ssg_server_get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
    switch (property_id) {

        case PROP_CHAR_DEVICE_RECOGNIZED_SUBTYPES:
            g_value_set_boxed(value, spice_server_char_device_recognized_subtypes());
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ssg_server_class_init (SsgServerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = ssg_server_set_property;
    object_class->get_property = ssg_server_get_property;
    object_class->finalize = ssg_server_finalize;
    object_class->constructed = ssg_server_constructed;

    obj_properties[PROP_COMPAT_VERSION] =
            g_param_spec_enum ("compat-version",
                    "Compat Version",
                    "TODO",
                    SSG_TYPE_SPICE_COMPAT_VERSION_T,
                    SPICE_COMPAT_VERSION_0_6,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_PORT] =
            g_param_spec_boxed ("port",
                    "Port",
                    "The port to serve on.",
                    G_TYPE_VALUE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_ADDR_FLAGS] =
            g_param_spec_flags ("addr-flags",
                    "Address flags",
                    "TODO",
                    SSG_TYPE_SPICE_ADDR_FLAGS_T,
                    0,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_ADDR] =
            g_param_spec_string ("addr",
                    "Address",
                    "The address to serve on",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_EXIT_ON_DISCONNECT] =
            g_param_spec_boolean ("exit-on-disconnect",
                    "Exit on disconnect",
                    "TODO",
                    FALSE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_NOAUTH] =
            g_param_spec_boolean ("noauth",
                    "Noauth",
                    "TODO",
                    FALSE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_SASL] =
            g_param_spec_boolean ("sasl",
                    "SASL",
                    "TODO",
                    FALSE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_SASL_APPNAME] =
            g_param_spec_string ("sasl-appname",
                    "SASL Appname",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TICKET_LIFETIME] =
            g_param_spec_int ("ticket-lifetime",
                    "Ticket lifetime",
                    "The lifetime of the ticket",
                    0,
                    G_MAXINT,
                    0,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TICKET_PASSWD] =
            g_param_spec_string ("ticket-passwd",
                    "Ticket password",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TLS_KEY_PASSWD] =
            g_param_spec_string ("tls-key-passwd",
                    "TLS key password",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TLS_DH_KEY_FILE] =
            g_param_spec_string ("tls-dh-key-file",
                    "TLS DH key file",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TLS_CIPHERSUITE] =
            g_param_spec_string ("tls-ciphersuite",
                    "TLS ciphersuite",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);


    obj_properties[PROP_TLS_PORT] =
            g_param_spec_int ("tls-port",
                    "TLS port",
                    "TODO",
                    -1,
                    65535,
                    -1,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TLS_CA_CERT_FILE] =
            g_param_spec_string ("tls-ca-cert-file",
                    "TLS CA cert file",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TLS_CERTS_FILE] =
            g_param_spec_string ("tls-certs-file",
                    "TLS certs file",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_TLS_PRIVATE_KEY_FILE] =
            g_param_spec_string ("tls-private-key-file",
                    "TLS private key file",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_IMAGE_COMPRESSION] =
            g_param_spec_enum ("image-compression",
                    "Image compression",
                    "TODO",
                    SSG_TYPE_SPICE_IMAGE_COMPRESSION_T,
                    SPICE_IMAGE_COMPRESSION_QUIC,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_JPEG_COMPRESSION] =
            g_param_spec_enum ("jpeg-compression",
                    "JPEG compression",
                    "TODO",
                    SSG_TYPE_SPICE_WAN_COMPRESSION_T,
                    SPICE_WAN_COMPRESSION_AUTO,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_ZLIB_GLZ_COMPRESSION] =
            g_param_spec_enum ("zlib-glz-compression",
                    "zlib GLZ compression",
                    "TODO",
                    SSG_TYPE_SPICE_WAN_COMPRESSION_T,
                    SPICE_WAN_COMPRESSION_AUTO,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_DEFAULT_CHANNEL_SECURITY] =
            g_param_spec_enum ("default-channel-security",
                    "Default channel security",
                    "TODO",
                    SSG_TYPE_SPICE_CHANNEL_SECURITY_T,
                    SPICE_CHANNEL_SECURITY_NONE,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_SSL_CHANNELS] =
            g_param_spec_string ("ssl-channels",
                    "SSL channels",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_INSECURE_CHANNELS] =
            g_param_spec_string ("insecure-channels",
                    "Insecure channels",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_STREAMING_VIDEO] =
            g_param_spec_enum ("streaming-video",
                    "Streaming video",
                    "TODO",
                    SSG_TYPE_SPICE_STREAM_VIDEO_T,
                    SPICE_STREAM_VIDEO_OFF,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_VIDEO_CODECS] =
            g_param_spec_string ("video-codecs",
                    "Video codecs",
                    "TODO",
                    "auto",
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_PLAYBACK_COMPRESSION] =
            g_param_spec_boolean ("playback-compression",
                    "Playback compression",
                    "TODO",
                    FALSE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_AGENT_MOUSE] =
            g_param_spec_boolean ("agent-mouse",
                    "Agent mouse",
                    "TODO",
                    TRUE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_AGENT_COPYPASTE] =
            g_param_spec_boolean ("agent-copypaste",
                    "Agent copy/paste",
                    "TODO",
                    TRUE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_AGENT_FILE_XFER] =
            g_param_spec_boolean ("agent-file-xfer",
                    "Agent file transfer",
                    "TODO",
                    TRUE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_NAME] =
            g_param_spec_string ("name",
                    "Name",
                    "TODO",
                    NULL,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_UUID] =
            g_param_spec_boxed("uuid",
                    "UUID",
                    "TODO",
                    G_TYPE_BYTES,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_SEAMLESS_MIGRATION] =
            g_param_spec_boolean ("seamless-migration",
                    "Seamless migration",
                    "TODO",
                    FALSE,
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    /**
     * SsgServer:char-device-recognized-subtypes: (array zero-terminated=1) (transfer none):
     */
    obj_properties[PROP_CHAR_DEVICE_RECOGNIZED_SUBTYPES] =
            g_param_spec_boxed("char-device-recognized-subtypes",
                    "Char device recognized subtypes",
                    "TODO",
                    G_TYPE_STRV,
                    G_PARAM_READABLE);


    g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

}

gint ssg_server_set_ticket(SsgServer *server, const gchar *passwd, gint lifetime, gboolean fail_if_connected, gboolean disconnect_if_connected)
{
    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    return spice_server_set_ticket(priv->server, passwd, lifetime, fail_if_connected, disconnect_if_connected);
}

// TODO should these be merged into ssg_server_add_client(server, socket, skip_auth, security) ?
gint ssg_server_add_client(SsgServer *server, gint socket, gboolean skip_auth)
{
    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    return spice_server_add_client(priv->server, socket, skip_auth);
}

gint ssg_server_add_ssl_client(SsgServer *server, gint socket, gboolean skip_auth)
{
    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    return spice_server_add_ssl_client(priv->server, socket, skip_auth);
}

void ssg_server_vm_start(SsgServer *server) {

    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    g_return_if_fail (priv->server != NULL);

    return spice_server_vm_start (priv->server);

}

void ssg_server_vm_stop(SsgServer *server) {

    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    g_return_if_fail (priv->server != NULL);

    return spice_server_vm_stop (priv->server);

}

gint ssg_server_add_interface(SsgServer *server, SsgBaseInstance *ssg_baseinstance) {

    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    g_object_ref(G_OBJECT(ssg_baseinstance));
    priv->interfaces = g_list_append(priv->interfaces, ssg_baseinstance);
    SpiceBaseInstance* base = ssg_baseinstance_get_spice_baseinstance (ssg_baseinstance);
    return spice_server_add_interface(priv->server, base);

}

gint ssg_server_remove_interface(SsgServer *server, SsgBaseInstance *ssg_baseinstance) {

    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    priv->interfaces = g_list_remove(priv->interfaces, ssg_baseinstance);
    g_object_unref(G_OBJECT(ssg_baseinstance));
    SpiceBaseInstance* base = ssg_baseinstance_get_spice_baseinstance (ssg_baseinstance);
    return spice_server_remove_interface(base);

}

void ssg_server_destroy(SsgServer *server) {

    SsgServerPrivate *priv = ssg_server_get_instance_private (server);

    if (priv->server) {
        spice_server_destroy(priv->server);
        g_list_free_full(priv->interfaces, g_object_unref);
    }
    priv->server = NULL;

}

