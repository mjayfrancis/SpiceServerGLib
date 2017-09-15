#ifndef __SPICESERVERGLIB_MOUSEINSTANCE_H__
#define __SPICESERVERGLIB_MOUSEINSTANCE_H__

#include <glib-object.h>

#include "ssg-baseinstance.h"

G_BEGIN_DECLS

#define SSG_TYPE_MOUSE_INSTANCE ssg_mouse_instance_get_type()
G_DECLARE_FINAL_TYPE (SsgMouseInstance, ssg_mouse_instance, SSG, MOUSE_INSTANCE, SsgBaseInstance)

typedef struct _SsgMouseInstance {
    SsgBaseInstance parent_instance;
} SsgMouseInstance;

G_END_DECLS

#endif /* __SPICESERVERGLIB_MOUSEINSTANCE_H__ */
