#ifndef __SPICESERVERGLIB_KEYBOARDINSTANCE_H__
#define __SPICESERVERGLIB_KEYBOARDINSTANCE_H__

#include <glib-object.h>

#include "ssg-baseinstance.h"

G_BEGIN_DECLS

#define SSG_TYPE_KEYBOARD_INSTANCE ssg_keyboard_instance_get_type()
G_DECLARE_FINAL_TYPE (SsgKeyboardInstance, ssg_keyboard_instance, SSG, KEYBOARD_INSTANCE, SsgBaseInstance)

typedef struct _SsgKeyboardInstance {
    SsgBaseInstance parent_instance;
} SsgKeyboardInstance;

gint ssg_keyboard_instance_set_leds(SsgKeyboardInstance *instance, int ledstate);

G_END_DECLS

#endif /* __SPICESERVERGLIB_KEYBOARDINSTANCE_H__ */
