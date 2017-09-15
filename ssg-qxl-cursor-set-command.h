#ifndef __SPICESERVERGLIB_QXLCURSORSETCOMMAND_H__
#define __SPICESERVERGLIB_QXLCURSORSETCOMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

#include "ssg-qxl-command.h"

G_BEGIN_DECLS

#define SSG_TYPE_QXL_CURSOR_SET_COMMAND ssg_qxl_cursor_set_command_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLCursorSetCommand, ssg_qxl_cursor_set_command, SSG, QXL_CURSOR_SET_COMMAND, SsgQXLCommand)

typedef struct _SsgQXLCursorSetCommand SsgQXLCursorSetCommand;
typedef struct _SsgQXLCursorSetCommandPrivate SsgQXLCursorSetCommandPrivate;

struct _SsgQXLCursorSetCommand {
        GObject parent;
};

struct _SsgQXLCursorSetCommandClass {
        GObjectClass parent;
};

SsgQXLCursorSetCommand *ssg_qxl_cursor_set_command_new (cairo_surface_t *cairo_surface, int hot_x, int hot_y, int x, int y);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLCURSORSETCOMMAND_H__ */
