#ifndef __SPICESERVERGLIB_TABLETINSTANCE_H__
#define __SPICESERVERGLIB_TABLETINSTANCE_H__

#include <glib-object.h>

#include "ssg-baseinstance.h"

G_BEGIN_DECLS

#define SSG_TYPE_TABLET_INSTANCE ssg_tablet_instance_get_type()
G_DECLARE_FINAL_TYPE (SsgTabletInstance, ssg_tablet_instance, SSG, TABLET_INSTANCE, SsgBaseInstance)

typedef struct _SsgTabletInstance {
    SsgBaseInstance parent_instance;
} SsgTabletInstance;

G_END_DECLS

#endif /* __SPICESERVERGLIB_TABLETINSTANCE_H__ */
