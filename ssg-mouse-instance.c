#include <spice.h>

#include "ssg-mouse-instance.h"

enum {
    MOTION,
    BUTTONS,
    LAST_SIGNAL
};

static guint
ssg_mouse_instance_signals[LAST_SIGNAL] = { 0 };

typedef struct _SsgMouseInstancePrivate SsgMouseInstancePrivate;
struct _SsgMouseInstancePrivate {
    SpiceBaseInstance *base;
    SsgMouseInstance *self;
    SpiceMouseInstance mouse_instance;
};

#define SSG_TYPE_MOUSE_INSTANCE ssg_mouse_instance_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgMouseInstance, ssg_mouse_instance, SSG_TYPE_BASEINSTANCE)


static void
ssg_mouse_instance_init (SsgMouseInstance *self)
{
}

static void motion(SpiceMouseInstance *sin, int dx, int dy, int dz, uint32_t buttons_state)
{
    SsgMouseInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgMouseInstancePrivate, mouse_instance);

    g_signal_emit(priv->self, ssg_mouse_instance_signals[MOTION], 0, dx, dy, dz, buttons_state);
}

static void buttons(SpiceMouseInstance *sin, uint32_t buttons_state)
{
    SsgMouseInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgMouseInstancePrivate, mouse_instance);

    g_signal_emit(priv->self, ssg_mouse_instance_signals[BUTTONS], 0, buttons_state);
}

SpiceBaseInstance*
ssg_mouse_instance_helper_get_spice_baseinstance (SsgBaseInstance *self)
{
    SsgMouseInstancePrivate *priv = ssg_mouse_instance_get_instance_private (SSG_MOUSE_INSTANCE(self));
    return &priv->mouse_instance.base;
}

static const SpiceMouseInterface mouse_interface = {
    .base = {
        .type          = SPICE_INTERFACE_MOUSE,
        .description   = "mouse",
        .major_version = SPICE_INTERFACE_MOUSE_MAJOR,
        .minor_version = SPICE_INTERFACE_MOUSE_MINOR,
    },
    .motion = motion,
    .buttons = buttons
};

static void
ssg_mouse_instance_constructed (GObject *object)
{

    SsgMouseInstance *self = SSG_MOUSE_INSTANCE(object);
    SsgMouseInstancePrivate *priv = ssg_mouse_instance_get_instance_private (self);

    priv->self = self;
    priv->mouse_instance.base.sif = &mouse_interface.base;

    G_OBJECT_CLASS (ssg_mouse_instance_parent_class)->constructed (object);

}


static void
ssg_mouse_instance_class_finalize (GObject *object)
{
}

static void
ssg_mouse_instance_class_init (SsgMouseInstanceClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->constructed = ssg_mouse_instance_constructed;
    object_class->finalize = ssg_mouse_instance_class_finalize;

    SsgBaseInstanceClass *ssg_instance_class = SSG_BASEINSTANCE_CLASS (_class);
    ssg_instance_class->helper_get_spice_baseinstance = ssg_mouse_instance_helper_get_spice_baseinstance;

    ssg_mouse_instance_signals[MOTION] =
                g_signal_new ("motion",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        4,
                        G_TYPE_INT,
                        G_TYPE_INT,
                        G_TYPE_INT,
                        G_TYPE_UINT);

    ssg_mouse_instance_signals[BUTTONS] =
                g_signal_new ("buttons",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        1,
                        G_TYPE_UINT);

}
