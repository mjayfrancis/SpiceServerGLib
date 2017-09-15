#ifndef __SPICESERVERGLIB_QXLCOMMAND_H__
#define __SPICESERVERGLIB_QXLCOMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

G_BEGIN_DECLS

#define SSG_TYPE_QXL_COMMAND ssg_qxl_command_get_type()
G_DECLARE_DERIVABLE_TYPE (SsgQXLCommand, ssg_qxl_command, SSG, QXL_COMMAND, GObject)

typedef struct _SsgQXLCommandPrivate SsgQXLCommandPrivate;

struct _SsgQXLCommandClass {
	    GObjectClass parent;
	    QXLCommandExt* (*helper_get_command_ext)();
};

QXLCommandExt* ssg_qxl_command_get_command_ext (SsgQXLCommand *self);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLCOMMAND_H__ */
