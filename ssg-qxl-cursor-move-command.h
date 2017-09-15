#ifndef __SPICESERVERGLIB_QXLCURSORMOVECOMMAND_H__
#define __SPICESERVERGLIB_QXLCURSORMOVECOMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

#include "ssg-qxl-command.h"

G_BEGIN_DECLS

#define SSG_TYPE_QXL_CURSOR_MOVE_COMMAND ssg_qxl_cursor_move_command_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLCursorMoveCommand, ssg_qxl_cursor_move_command, SSG, QXL_CURSOR_MOVE_COMMAND, SsgQXLCommand)

typedef struct _SsgQXLCursorMoveCommand SsgQXLCursorMoveCommand;
typedef struct _SsgQXLCursorMoveCommandPrivate SsgQXLCursorMoveCommandPrivate;

struct _SsgQXLCursorMoveCommand {
        GObject parent;
};

struct _SsgQXLCursorMoveCommandClass {
        GObjectClass parent;
};

SsgQXLCursorMoveCommand *ssg_qxl_cursor_move_command_new (int x, int y);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLCURSORMOVECOMMAND_H__ */
