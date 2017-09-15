#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <spice.h>
#include <cairo.h>
#include <glib.h>
#include <glib-object.h>

#include "ssg-qxl-instance.h"
#include "ssg-qxl-command.h"

enum
{
    PROP_0,
    PROP_WIDTH,
    PROP_HEIGHT,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };


enum {
    ATTACHE_WORKER,
    SET_COMPRESSION_LEVEL,
    GET_COMMAND,
    REQ_CMD_NOTIFICATION,
    RELEASE_RESOURCE,
    GET_CURSOR_COMMAND,
    REQ_CURSOR_NOTIFICATION,
    //FLUSH_RESOURCES,
    //ASYNC_COMPLETE,
    //UPDATE_AREA_COMPLETE
    SET_CLIENT_CAPABILITIES,
    //CLIENT_MONITORS_CONFIG,
    LAST_SIGNAL
};

static guint
ssg_qxl_instance_signals[LAST_SIGNAL] = { 0 };

typedef struct _SsgQXLInstancePrivate SsgQXLInstancePrivate;
struct _SsgQXLInstancePrivate {
    SpiceBaseInstance *base;
    guint port; /* The port to serve on */

    SsgQXLInstance *self;

    QXLInstance qxl_instance;
    QXLWorker *qxl_worker;

    uint8_t *primary_surface;
    int primary_height;
    int primary_width;

    GAsyncQueue *queue;
};

#define SSG_TYPE_QXL_INSTANCE ssg_qxl_instance_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLInstance, ssg_qxl_instance, SSG_TYPE_BASEINSTANCE)


#define MEM_SLOT_GROUP_ID 0

static void create_primary_surface(SsgQXLInstancePrivate *priv, uint32_t width,
                                   uint32_t height)
{
    QXLDevSurfaceCreate surface = { 0, };

    surface.format     = SPICE_SURFACE_FMT_32_xRGB;
    surface.width      = priv->primary_width = width;
    surface.height     = priv->primary_height = height;
    surface.stride     = -width * 4; /* negative? */
    surface.mouse_mode = TRUE; /* unused by red_worker */
    surface.flags      = 0;
    surface.type       = 0;    /* unused by red_worker */
    surface.position   = 0;    /* unused by red_worker */
    surface.mem        = (uint64_t)priv->primary_surface;
    surface.group_id   = MEM_SLOT_GROUP_ID;

    spice_qxl_create_primary_surface(&priv->qxl_instance, 0, &surface);
}

static QXLDevMemSlot slot = {
    .slot_group_id = MEM_SLOT_GROUP_ID,
    .slot_id = 0,
    .generation = 0,
    .virt_start = 0,
    .virt_end = ~0,
    .addr_delta = 0,
    .qxl_ram_size = ~0,
};


/* QXLInterface.attache_worker */
static void attache_worker(QXLInstance *qin, QXLWorker *_qxl_worker)
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);

    if (priv->qxl_worker) {
        if (priv->qxl_worker != _qxl_worker)
            printf("%s ignored, %p is set, ignoring new %p\n", __func__,
                   priv->qxl_worker, _qxl_worker);
        else
            printf("%s ignored, redundant\n", __func__);
        return;
    }
    priv->qxl_worker = _qxl_worker;
    spice_qxl_add_memslot(&priv->qxl_instance, &slot);
    create_primary_surface(priv, priv->primary_width, priv->primary_height);

    g_signal_emit(priv->self, ssg_qxl_instance_signals[ATTACHE_WORKER], 0);
}

/* QXLInterface.set_compression_level */
static void set_compression_level(SPICE_GNUC_UNUSED QXLInstance *qin,
                                  SPICE_GNUC_UNUSED int level)
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);

    g_signal_emit(priv->self, ssg_qxl_instance_signals[SET_COMPRESSION_LEVEL], 0, !!level);

}

/* QXLInterface.set_mm_time */
static void set_mm_time(SPICE_GNUC_UNUSED QXLInstance *qin,
                        SPICE_GNUC_UNUSED uint32_t mm_time)
{
    // Deprecated
}

/* QXLInterface.get_init_info */
static void get_init_info(SPICE_GNUC_UNUSED QXLInstance *qin,
                          QXLDevInitInfo *info)
{
    memset(info, 0, sizeof(*info));
    info->num_memslots = 1;
    info->num_memslots_groups = 1;
    info->memslot_id_bits = 1;
    info->memslot_gen_bits = 1;
    info->n_surfaces = 2;
}

/* QXLInterface.get_command */
// called from spice_server thread (i.e. red_worker thread)
static int get_command(SPICE_GNUC_UNUSED QXLInstance *qin,
                       struct QXLCommandExt *ext)
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);
    SsgQXLCommand *ssg_qxl_command = NULL;

    g_signal_emit(priv->self, ssg_qxl_instance_signals[GET_COMMAND], 0, &ssg_qxl_command);

    if (ssg_qxl_command)
    {
        QXLCommandExt *command_ext = ssg_qxl_command_get_command_ext (ssg_qxl_command);
        *ext = *command_ext;
        return TRUE;
    }

    return FALSE;
}

/* QXLInterface.req_cmd_notification */
static int req_cmd_notification(QXLInstance *qin)
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);
    GValue instance = {0};
    GValue result = {0};

    g_value_init (&instance, SSG_TYPE_QXL_INSTANCE);
    g_value_set_object (&instance, priv->self);
    g_value_init (&result, G_TYPE_BOOLEAN);
    g_value_set_boolean (&result, TRUE);

    g_signal_emitv(&instance, ssg_qxl_instance_signals[REQ_CMD_NOTIFICATION], 0, &result);

    g_value_unset (&instance);

    return g_value_get_boolean (&result);

}

/* QXLInterface.release_resource */
static void release_resource(SPICE_GNUC_UNUSED QXLInstance *qin,
                             struct QXLReleaseInfoExt release_info)
{
    assert(release_info.group_id == MEM_SLOT_GROUP_ID);

    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);
    SsgQXLCommand *ssg_qxl_command = (SsgQXLCommand*)(unsigned long)release_info.info->id;
    GValue *params;

    params = g_new0 (GValue, 2);
    g_value_init(&params[0], SSG_TYPE_QXL_INSTANCE);
    g_value_set_object(&params[0], priv->self);
    g_value_init (&params[1], SSG_TYPE_QXL_COMMAND);
    g_value_set_object (&params[1], ssg_qxl_command);

    g_signal_emitv(params, ssg_qxl_instance_signals[RELEASE_RESOURCE], 0, NULL);

    for (int n = 0; n < 2; n++)
        g_value_unset (&params[n]);
    g_free(params);

    g_object_unref (ssg_qxl_command);
}


/* QXLInterface.get_cursor_command */
static int get_cursor_command(QXLInstance *qin, struct QXLCommandExt *ext)
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);

    SsgQXLCommand *ssg_qxl_command = NULL;
    g_signal_emit(priv->self, ssg_qxl_instance_signals[GET_CURSOR_COMMAND], 0, &ssg_qxl_command);

    if (ssg_qxl_command)
    {
        QXLCommandExt *command_ext = ssg_qxl_command_get_command_ext (ssg_qxl_command);
        *ext = *command_ext;
        return TRUE;
    }

    return FALSE;
}

/* QXLInterface.req_cursor_notification */
static int req_cursor_notification(SPICE_GNUC_UNUSED QXLInstance *qin)
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);
    GValue instance = {0};
    GValue result = {0};

    g_value_init (&instance, SSG_TYPE_QXL_INSTANCE);
    g_value_set_object (&instance, priv->self);
    g_value_init (&result, G_TYPE_BOOLEAN);
    g_value_set_boolean (&result, TRUE);

    g_signal_emitv(&instance, ssg_qxl_instance_signals[REQ_CURSOR_NOTIFICATION], 0, &result);

    g_value_unset (&instance);

    return g_value_get_boolean (&result);
}

/* QXLInterface.notify_update */
static void notify_update(SPICE_GNUC_UNUSED QXLInstance *qin,
                          SPICE_GNUC_UNUSED uint32_t update_id)
{
    // Deprecated
}

/* QXLInterface.flush_resources */
static int flush_resources(SPICE_GNUC_UNUSED QXLInstance *qin)
{
    // TODO What, if anything, should we do here?
    return TRUE;
}

/* QXLInterface.client_monitors_config */
static int client_monitors_config(SPICE_GNUC_UNUSED QXLInstance *qin,
                                  VDAgentMonitorsConfig *monitors_config)
{
    // TODO VDAgentMonitorsConfig struct
    return 0;
}

/* QXLInterface.update_area_complete */
void update_area_complete(QXLInstance *qin, uint32_t surface_id,
                                 struct QXLRect *updated_rects,
                                 uint32_t num_updated_rects)
{
    // TODO QXLRect struct
}

/* QXLInterface.set_client_capabilities */
static void set_client_capabilities(QXLInstance *qin,
                                    uint8_t client_present,
                                    uint8_t caps[58])
{
    SsgQXLInstancePrivate *priv = SPICE_CONTAINEROF(qin, SsgQXLInstancePrivate, qxl_instance);

    guint8 *caps_copy = g_malloc(58);
    memcpy(caps_copy, caps, 58);
    GBytes *bytes = g_bytes_new_with_free_func(caps_copy, 58, g_free, caps_copy);

    g_signal_emit(priv->self, ssg_qxl_instance_signals[SET_CLIENT_CAPABILITIES], 0, client_present, bytes);
}

SpiceBaseInstance*
ssg_qxl_instance_helper_get_spice_baseinstance (SsgBaseInstance *self)
{
    SsgQXLInstancePrivate *priv = ssg_qxl_instance_get_instance_private (SSG_QXL_INSTANCE(self));
    return &priv->qxl_instance.base;
}

QXLInterface display_interface = {
        .base = {
            .type = SPICE_INTERFACE_QXL,
            .description = "SpiceServerGLib",
            .major_version = SPICE_INTERFACE_QXL_MAJOR,
            .minor_version = SPICE_INTERFACE_QXL_MINOR
        },

        .attache_worker = attache_worker,
        .set_compression_level = set_compression_level,
        .set_mm_time = set_mm_time,
        .get_init_info = get_init_info,

        .get_command = get_command,
        .req_cmd_notification = req_cmd_notification,
        .release_resource = release_resource,
        .get_cursor_command = get_cursor_command,
        .req_cursor_notification = req_cursor_notification,
        .notify_update = notify_update,
        .flush_resources = flush_resources,
        .client_monitors_config = client_monitors_config,
        .set_client_capabilities = set_client_capabilities,
        .update_area_complete = update_area_complete,
};

static void
ssg_qxl_instance_constructed (GObject *object)
{

    SsgQXLInstance *self = SSG_QXL_INSTANCE(object);
    SsgQXLInstancePrivate *priv = ssg_qxl_instance_get_instance_private (self);

    priv->self = self;
    priv->queue = g_async_queue_new();
    priv->qxl_instance.base.sif = &display_interface.base;
    priv->qxl_instance.id = 0;
    priv->primary_surface = calloc (priv->primary_width * priv->primary_height * 4, 1);

    G_OBJECT_CLASS (ssg_qxl_instance_parent_class)->constructed (object);

}

static void
ssg_qxl_instance_finalize (GObject *object)
{
    G_OBJECT_CLASS (ssg_qxl_instance_parent_class)->finalize (object);
}


static void
ssg_qxl_instance_init (SsgQXLInstance *self)
{
}

static void
ssg_qxl_instance_set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    SsgQXLInstancePrivate *priv = ssg_qxl_instance_get_instance_private (SSG_QXL_INSTANCE(object));

    switch (property_id) {

        case PROP_WIDTH:
            priv->primary_width = g_value_get_uint(value);
            break;

        case PROP_HEIGHT:
            priv->primary_height = g_value_get_uint(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }

}

static void
ssg_qxl_instance_get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
    SsgQXLInstancePrivate *priv = ssg_qxl_instance_get_instance_private (SSG_QXL_INSTANCE(object));

    switch (property_id) {

        case PROP_WIDTH:
            g_value_set_uint(value, priv->primary_width);
            break;

        case PROP_HEIGHT:
            g_value_set_uint(value, priv->primary_height);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
ssg_qxl_instance_class_init (SsgQXLInstanceClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (_class);
    object_class->set_property = ssg_qxl_instance_set_property;
    object_class->get_property = ssg_qxl_instance_get_property;
    object_class->constructed = ssg_qxl_instance_constructed;
    object_class->finalize = ssg_qxl_instance_finalize;

    SsgBaseInstanceClass *ssg_instance_class = SSG_BASEINSTANCE_CLASS (_class);
    ssg_instance_class->helper_get_spice_baseinstance = ssg_qxl_instance_helper_get_spice_baseinstance;

    obj_properties[PROP_WIDTH] =
    g_param_spec_uint ("width",
                    "Width",
                    "The width of the primary surface",
                    1,
                    65536,
                    1280,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_HEIGHT] =
    g_param_spec_uint ("height",
                    "Height",
                    "The height of the primary surface",
                    1,
                    65536,
                    1024,
                    G_PARAM_READWRITE |
                    G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

    ssg_qxl_instance_signals[ATTACHE_WORKER] =
            g_signal_new ("attache-worker",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    G_TYPE_NONE,
                    0,
                    NULL);

    ssg_qxl_instance_signals[SET_COMPRESSION_LEVEL] =
            g_signal_new ("set-compression-level",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    G_TYPE_NONE,
                    1,
                    G_TYPE_BOOLEAN); // TODO Is boolean correct here? (actual code sends 0 or 1)
    /**
     * SsgQXLInstance::get-command:
     *
     * Returns: (transfer full):
     */
    ssg_qxl_instance_signals[GET_COMMAND] =
            g_signal_new ("get-command",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0L,
                    NULL,
                    NULL,
                    NULL,
                    SSG_TYPE_QXL_COMMAND,
                    0,
                    NULL);

    ssg_qxl_instance_signals[REQ_CMD_NOTIFICATION] =
            g_signal_new ("req-cmd-notification",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    G_TYPE_BOOLEAN,
                    0,
                    NULL);

    ssg_qxl_instance_signals[RELEASE_RESOURCE] =
            g_signal_new ("release-resource",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    G_TYPE_NONE,
                    1,
                    SSG_TYPE_QXL_COMMAND);

    /**
     * SsgQXLInstance::get-cursor-command:
     *
     * Returns: (transfer full):
     */
    ssg_qxl_instance_signals[GET_CURSOR_COMMAND] =
            g_signal_new ("get-cursor-command",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    SSG_TYPE_QXL_COMMAND,
                    0,
                    NULL);

    ssg_qxl_instance_signals[REQ_CURSOR_NOTIFICATION] =
            g_signal_new ("req-cursor-notification",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    G_TYPE_BOOLEAN,
                    0,
                    NULL);

    ssg_qxl_instance_signals[SET_CLIENT_CAPABILITIES] =
            g_signal_new ("set-client-capabilities",
                    G_TYPE_FROM_CLASS(_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    G_TYPE_NONE,
                    2,
                    G_TYPE_BOOLEAN,
                    G_TYPE_BYTES);
}

/**
 * ssg_qxl_instance_waketup:
 * @server: A @SsgSpiceServer
 *
 * Return value: nothing.
 */
void
ssg_qxl_instance_wakeup (SsgQXLInstance *server)
{

    SsgQXLInstancePrivate *priv = ssg_qxl_instance_get_instance_private (server);

    spice_qxl_wakeup(&priv->qxl_instance);
}

void
ssg_qxl_instance_update_area (SsgQXLInstance *server, unsigned int surface_id, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    SsgQXLInstancePrivate *priv = ssg_qxl_instance_get_instance_private (server);

    struct QXLRect rect = {
            .left = x,
            .top = y,
            .right = x + width,
            .bottom = y + height
    };

    spice_qxl_update_area(&priv->qxl_instance, surface_id, &rect, NULL, 0, 1);
}


