#ifndef __SPICESERVERGLIB_QXLDRAWCOPYCOMMAND_H__
#define __SPICESERVERGLIB_QXLDRAWCOPYCOMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

#include "ssg-qxl-command.h"

G_BEGIN_DECLS

#define SSG_TYPE_QXL_DRAW_COPY_COMMAND ssg_qxl_draw_copy_command_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLDrawCopyCommand, ssg_qxl_draw_copy_command, SSG, QXL_DRAW_COPY_COMMAND, SsgQXLCommand)

typedef struct _SsgQXLDrawCopyCommand SsgQXLDrawCopyCommand;
typedef struct _SsgQXLDrawCopyCommandPrivate SsgQXLDrawCopyCommandPrivate;

struct _SsgQXLDrawCopyCommand {
        GObject parent;
};

struct _SsgQXLDrawCopyCommandClass {
        GObjectClass parent;
};

SsgQXLDrawCopyCommand *ssg_qxl_draw_copy_command_new (guint32 surface_id, guint32 x, guint32 y, cairo_surface_t *cairo_surface, guint32 width, guint32 height);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLDRAWCOPYCOMMAND_H__ */
