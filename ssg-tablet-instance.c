#include <spice.h>

#include "ssg-tablet-instance.h"

enum {
    SET_LOGICAL_SIZE,
    POSITION,
    WHEEL,
    BUTTONS,
    LAST_SIGNAL
};

static guint
ssg_tablet_instance_signals[LAST_SIGNAL] = { 0 };

typedef struct _SsgTabletInstancePrivate SsgTabletInstancePrivate;
struct _SsgTabletInstancePrivate {
    SpiceBaseInstance *base;
    SsgTabletInstance *self;
    SpiceTabletInstance tablet_instance;
};

#define SSG_TYPE_TABLET_INSTANCE ssg_tablet_instance_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgTabletInstance, ssg_tablet_instance, SSG_TYPE_BASEINSTANCE)


static void
ssg_tablet_instance_init (SsgTabletInstance *self)
{
}

static void set_logical_size(SpiceTabletInstance *sin, int width, int height)
{
    SsgTabletInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgTabletInstancePrivate, tablet_instance);

    g_signal_emit(priv->self, ssg_tablet_instance_signals[SET_LOGICAL_SIZE], 0, width, height);

}

static void position(SpiceTabletInstance *sin, int x, int y, uint32_t buttons_state)
{
    SsgTabletInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgTabletInstancePrivate, tablet_instance);

    g_signal_emit(priv->self, ssg_tablet_instance_signals[POSITION], 0, x, y, buttons_state);
}

static void wheel(SpiceTabletInstance *sin, int wheel_motion, uint32_t buttons_state)
{
    SsgTabletInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgTabletInstancePrivate, tablet_instance);

    g_signal_emit(priv->self, ssg_tablet_instance_signals[WHEEL], 0, wheel_motion, buttons_state);
}

static void buttons(SpiceTabletInstance *sin, uint32_t buttons_state)
{
    SsgTabletInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgTabletInstancePrivate, tablet_instance);

    g_signal_emit(priv->self, ssg_tablet_instance_signals[BUTTONS], 0, buttons_state);
}

SpiceBaseInstance*
ssg_tablet_instance_helper_get_spice_baseinstance (SsgBaseInstance *self)
{
    SsgTabletInstancePrivate *priv = ssg_tablet_instance_get_instance_private (SSG_TABLET_INSTANCE(self));
    return &priv->tablet_instance.base;
}

static const SpiceTabletInterface tablet_interface = {
    .base = {
        .type          = SPICE_INTERFACE_TABLET,
        .description   = "tablet",
        .major_version = SPICE_INTERFACE_TABLET_MAJOR,
        .minor_version = SPICE_INTERFACE_TABLET_MINOR,
    },
    .set_logical_size = set_logical_size,
    .position = position,
    .wheel = wheel,
    .buttons = buttons
};

static void
ssg_tablet_instance_constructed (GObject *object)
{

    SsgTabletInstance *self = SSG_TABLET_INSTANCE(object);
    SsgTabletInstancePrivate *priv = ssg_tablet_instance_get_instance_private (self);

    priv->self = self;
    priv->tablet_instance.base.sif = &tablet_interface.base;

    G_OBJECT_CLASS (ssg_tablet_instance_parent_class)->constructed (object);

}


static void
ssg_tablet_instance_class_finalize (GObject *object)
{
}

static void
ssg_tablet_instance_class_init (SsgTabletInstanceClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->constructed = ssg_tablet_instance_constructed;
    object_class->finalize = ssg_tablet_instance_class_finalize;

    SsgBaseInstanceClass *ssg_instance_class = SSG_BASEINSTANCE_CLASS (_class);
    ssg_instance_class->helper_get_spice_baseinstance = ssg_tablet_instance_helper_get_spice_baseinstance;

    ssg_tablet_instance_signals[SET_LOGICAL_SIZE] =
                g_signal_new ("set-logical-size",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        2,
                        G_TYPE_INT,
                        G_TYPE_INT);

    ssg_tablet_instance_signals[POSITION] =
                g_signal_new ("position",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        3,
                        G_TYPE_INT,
                        G_TYPE_INT,
                        G_TYPE_UINT);

    ssg_tablet_instance_signals[WHEEL] =
                g_signal_new ("wheel",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        2,
                        G_TYPE_INT,
                        G_TYPE_UINT);

    ssg_tablet_instance_signals[BUTTONS] =
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
