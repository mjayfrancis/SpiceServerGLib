#ifndef __SPICESERVERGLIB_QXLINSTANCE_H__
#define __SPICESERVERGLIB_QXLINSTANCE_H__

#include <glib-object.h>

#include "ssg-baseinstance.h"

G_BEGIN_DECLS

#define SSG_TYPE_QXL_INSTANCE ssg_qxl_instance_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLInstance, ssg_qxl_instance, SSG, QXL_INSTANCE, SsgBaseInstance)

typedef struct _SsgQXLInstance {
	SsgBaseInstance parent_instance;
} SsgQXLInstance;

void ssg_qxl_instance_wakeup (SsgQXLInstance *server);

void ssg_qxl_instance_update_area (SsgQXLInstance *server, unsigned int surface_id, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLINSTANCE_H__ */
