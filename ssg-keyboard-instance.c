#include "ssg-keyboard-instance.h"

#include <spice.h>

enum {
    PUSH_KEY,
    LAST_SIGNAL
};

static guint
ssg_keyboard_instance_signals[LAST_SIGNAL] = { 0 };

typedef struct _SsgKeyboardInstancePrivate SsgKeyboardInstancePrivate;
struct _SsgKeyboardInstancePrivate {
    SpiceBaseInstance *base;

    SsgKeyboardInstance *self;

    SpiceKbdInstance kbd_instance;
    int ledstate;


};

#define SSG_TYPE_KEYBOARD_INSTANCE ssg_keyboard_instance_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgKeyboardInstance, ssg_keyboard_instance, SSG_TYPE_BASEINSTANCE)


static void
ssg_keyboard_instance_init (SsgKeyboardInstance *self)
{
}

static void push_key(SpiceKbdInstance *sin, uint8_t scancode)
{
    SsgKeyboardInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgKeyboardInstancePrivate, kbd_instance);

    g_signal_emit(priv->self, ssg_keyboard_instance_signals[PUSH_KEY], 0, scancode);

}

static uint8_t get_leds(SpiceKbdInstance *sin)
{
    SsgKeyboardInstancePrivate *priv = SPICE_CONTAINEROF(sin, SsgKeyboardInstancePrivate, kbd_instance);

    return priv->ledstate;
}

static const SpiceKbdInterface keyboard_interface = {
    .base = {
        .type          = SPICE_INTERFACE_KEYBOARD,
        .description   = "keyboard",
        .major_version = SPICE_INTERFACE_KEYBOARD_MAJOR,
        .minor_version = SPICE_INTERFACE_KEYBOARD_MINOR,
    },
    .push_scan_freg     = push_key,
    .get_leds           = get_leds,
};

SpiceBaseInstance*
ssg_keyboard_instance_helper_get_spice_baseinstance (SsgBaseInstance *self)
{
    SsgKeyboardInstancePrivate *priv = ssg_keyboard_instance_get_instance_private (SSG_KEYBOARD_INSTANCE(self));
    return &priv->kbd_instance.base;
}

static void
ssg_keyboard_instance_constructed (GObject *object)
{

    SsgKeyboardInstance *self = SSG_KEYBOARD_INSTANCE(object);
    SsgKeyboardInstancePrivate *priv = ssg_keyboard_instance_get_instance_private (self);

    priv->self = self;
    priv->kbd_instance.base.sif = &keyboard_interface.base;
    priv->ledstate = 0;

    G_OBJECT_CLASS (ssg_keyboard_instance_parent_class)->constructed (object);

}


static void
ssg_keyboard_instance_class_finalize (GObject *object)
{
}

static void
ssg_keyboard_instance_class_init (SsgKeyboardInstanceClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->constructed = ssg_keyboard_instance_constructed;
    object_class->finalize = ssg_keyboard_instance_class_finalize;

    SsgBaseInstanceClass *ssg_instance_class = SSG_BASEINSTANCE_CLASS (_class);
    ssg_instance_class->helper_get_spice_baseinstance = ssg_keyboard_instance_helper_get_spice_baseinstance;

    ssg_keyboard_instance_signals[PUSH_KEY] =
                g_signal_new ("push-key",
                        G_TYPE_FROM_CLASS(_class),
                        G_SIGNAL_RUN_LAST,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        G_TYPE_NONE,
                        1,
                        G_TYPE_UCHAR);
}

gint ssg_keyboard_instance_set_leds(SsgKeyboardInstance *self, int ledstate)
{
    SsgKeyboardInstancePrivate *priv = ssg_keyboard_instance_get_instance_private (self);
    priv->ledstate = ledstate;
}


