#include <spice.h>
#include <string.h>

#include "ssg-char-device-instance.h"

enum
{
    PROP_0,
    PROP_SUBTYPE,
    PROP_PORTNAME,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

enum {
    STATE,
    WRITE,
    READ,
    EVENT,
    LAST_SIGNAL
};

static guint
ssg_char_device_instance_signals[LAST_SIGNAL] = { 0 };

typedef struct _SsgCharDeviceInstancePrivate SsgCharDeviceInstancePrivate;
struct _SsgCharDeviceInstancePrivate {
    SpiceBaseInstance *base;
    SsgCharDeviceInstance *self;
    SpiceCharDeviceInstance char_device_instance;
};

#define SSG_TYPE_CHAR_DEVICE_INSTANCE ssg_char_device_instance_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgCharDeviceInstance, ssg_char_device_instance, SSG_TYPE_BASEINSTANCE)


static void
ssg_char_device_instance_init (SsgCharDeviceInstance *self)
{
}

void
char_device_state(SpiceCharDeviceInstance *sin, int connected)
{
    SsgCharDeviceInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgCharDeviceInstancePrivate, char_device_instance);

    g_signal_emit(priv->self, ssg_char_device_instance_signals[STATE], 0, !!connected);
}

int
char_device_write(SpiceCharDeviceInstance *sin, const uint8_t *buf, int len)
{
    SsgCharDeviceInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgCharDeviceInstancePrivate, char_device_instance);

    guint8 *buf_copy = g_new(guint8, len);
    memcpy (buf_copy, buf, len);

    GBytes *buf_bytes = g_bytes_new_with_free_func(buf_copy, len, g_free, buf_copy);
    int result;

    g_signal_emit(priv->self, ssg_char_device_instance_signals[WRITE], 0, buf_bytes, &result);

    return result;
}

int
char_device_read(SpiceCharDeviceInstance *sin, uint8_t *buf, int len)
{
    SsgCharDeviceInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgCharDeviceInstancePrivate, char_device_instance);

    guint8 *buf_copy = g_new(guint8, len);
    memcpy (buf_copy, buf, len);

    GBytes *result;

    g_signal_emit(priv->self, ssg_char_device_instance_signals[READ], 0, len, &result);

    int bytes_read = 0;
    if (result != NULL)
    {
        bytes_read = g_bytes_get_size(result);
        g_return_val_if_fail(bytes_read <= len, 0);
        memcpy(buf,g_bytes_get_data(result,NULL),bytes_read);
    }

    return bytes_read;

}

void
char_device_event(SpiceCharDeviceInstance *sin, uint8_t event)
{
    SsgCharDeviceInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgCharDeviceInstancePrivate, char_device_instance);

    g_signal_emit(priv->self, ssg_char_device_instance_signals[STATE], 0, event);
}

SpiceBaseInstance*
ssg_char_device_instance_helper_get_spice_baseinstance (SsgBaseInstance *self)
{
    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (SSG_CHAR_DEVICE_INSTANCE(self));
    return &priv->char_device_instance.base;
}

static const SpiceCharDeviceInterface char_device_interface = {
    .base = {
        .type          = SPICE_INTERFACE_CHAR_DEVICE,
        .description   = "char_device",
        .major_version = SPICE_INTERFACE_CHAR_DEVICE_MAJOR,
        .minor_version = SPICE_INTERFACE_CHAR_DEVICE_MINOR,
    },
    .state = char_device_state,
    .write = char_device_write,
    .read = char_device_read,
    .event = char_device_event,
    .flags = SPICE_CHAR_DEVICE_NOTIFY_WRITABLE
};

static void
ssg_char_device_instance_constructed (GObject *object)
{

    SsgCharDeviceInstance *self = SSG_CHAR_DEVICE_INSTANCE(object);
    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (self);

    priv->self = self;
    priv->char_device_instance.base.sif = &char_device_interface.base;

    G_OBJECT_CLASS (ssg_char_device_instance_parent_class)->constructed (object);

}

static void
ssg_char_device_instance_set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (SSG_CHAR_DEVICE_INSTANCE(object));

    switch (property_id) {

        case PROP_SUBTYPE:
            priv->char_device_instance.subtype = g_value_dup_string(value);
            break;

        case PROP_PORTNAME:
            priv->char_device_instance.portname = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }

}

static void
ssg_char_device_instance_get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (SSG_CHAR_DEVICE_INSTANCE(object));

    switch (property_id) {

        case PROP_SUBTYPE:
            g_value_set_string(value, priv->char_device_instance.subtype);
            break;

        case PROP_PORTNAME:
            g_value_set_string(value, priv->char_device_instance.portname);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ssg_char_device_instance_class_finalize (GObject *object)
{
    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (SSG_CHAR_DEVICE_INSTANCE(object));

    g_free((char *)priv->char_device_instance.subtype);
    g_free((char *)priv->char_device_instance.portname);
}

static void
ssg_char_device_instance_class_init (SsgCharDeviceInstanceClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->set_property = ssg_char_device_instance_set_property;
    object_class->get_property = ssg_char_device_instance_get_property;
    object_class->constructed = ssg_char_device_instance_constructed;
    object_class->finalize = ssg_char_device_instance_class_finalize;

    SsgBaseInstanceClass *ssg_instance_class = SSG_BASEINSTANCE_CLASS (_class);
    ssg_instance_class->helper_get_spice_baseinstance = ssg_char_device_instance_helper_get_spice_baseinstance;

    obj_properties[PROP_SUBTYPE] =
            g_param_spec_string ("subtype",
                    "Subtype",
                    "The subtype of the port",
                    NULL,
                    G_PARAM_READABLE |
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_PORTNAME] =
            g_param_spec_string ("portname",
                    "Port name",
                    "The name of the port",
                    NULL,
                    G_PARAM_READABLE |
                    G_PARAM_WRITABLE |
                    G_PARAM_CONSTRUCT);

    g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

    ssg_char_device_instance_signals[STATE] =
                g_signal_new ("state",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        1,
                        G_TYPE_BOOLEAN);

    ssg_char_device_instance_signals[WRITE] =
                g_signal_new ("write",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_INT,
                        1,
                        G_TYPE_BYTES);

    ssg_char_device_instance_signals[READ] =
                g_signal_new ("read",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_BYTES,
                        1,
                        G_TYPE_INT);

    ssg_char_device_instance_signals[EVENT] =
                g_signal_new ("event",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        1,
                        G_TYPE_INT);

}

void ssg_char_device_instance_wakeup(SsgCharDeviceInstance *self) {

    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (self);

    spice_server_char_device_wakeup(&priv->char_device_instance);

}

/**
 * ssg_char_device_instance_port_event:
 * @event: (type spice_port_event_t)
 */
void ssg_char_device_instance_port_event(SsgCharDeviceInstance *self, guint8 event) {

    SsgCharDeviceInstancePrivate *priv = ssg_char_device_instance_get_instance_private (self);

    spice_server_port_event(&priv->char_device_instance, event);

}


