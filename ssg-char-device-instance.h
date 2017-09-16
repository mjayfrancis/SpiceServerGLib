#ifndef __SPICESERVERGLIB_CHARDEVICEINSTANCE_H__
#define __SPICESERVERGLIB_CHARDEVICEINSTANCE_H__

#include <glib-object.h>

#include "ssg-baseinstance.h"

G_BEGIN_DECLS

#define SSG_TYPE_CHAR_DEVICE_INSTANCE ssg_char_device_instance_get_type()
G_DECLARE_FINAL_TYPE (SsgCharDeviceInstance, ssg_char_device_instance, SSG, CHAR_DEVICE_INSTANCE, SsgBaseInstance)

typedef struct _SsgCharDeviceInstance {
    SsgBaseInstance parent_instance;
} SsgCharDeviceInstance;

void ssg_char_device_instance_wakeup(SsgCharDeviceInstance *self);
void ssg_char_device_instance_port_event(SsgCharDeviceInstance *self, gchar *device, guint8 event);

G_END_DECLS

#endif /* __SPICESERVERGLIB_CHARDEVICEINSTANCE_H__ */
