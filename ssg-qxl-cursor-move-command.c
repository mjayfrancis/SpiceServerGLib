#include "ssg-qxl-cursor-move-command.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>

#define CURSOR_WIDTH 32
#define CURSOR_HEIGHT 32
typedef struct _SsgQXLCursorMoveCommandPrivate {
    QXLCommandExt ext;
    QXLCursorCmd cursor_command;
    QXLCursor cursor;
} SsgQXLCursorMoveCommandPrivate;

#define SSG_TYPE_QXL_CURSOR_MOVE_COMMAND ssg_qxl_cursor_move_command_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLCursorMoveCommand, ssg_qxl_cursor_move_command, SSG_TYPE_QXL_COMMAND)

static void
ssg_qxl_cursor_move_command_finalize (GObject *object)
{
}

static QXLCommandExt *
helper_get_command_ext(SsgQXLCursorMoveCommand *command)
{
    SsgQXLCursorMoveCommandPrivate *priv = ssg_qxl_cursor_move_command_get_instance_private (command);

    return &priv->ext;
}

static void
ssg_qxl_cursor_move_command_class_init (SsgQXLCursorMoveCommandClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->finalize = ssg_qxl_cursor_move_command_finalize;

    SsgQXLCommandClass *ssg_qxl_command_class = SSG_QXL_COMMAND_CLASS (_class);
    ssg_qxl_command_class->helper_get_command_ext = helper_get_command_ext;
}

static void
ssg_qxl_cursor_move_command_init (SsgQXLCursorMoveCommand *object)
{

}


#define MEM_SLOT_GROUP_ID 0

static void build_qxl_cursor_move_command(SsgQXLCursorMoveCommand *self, int x, int y)
{
    SsgQXLCursorMoveCommandPrivate *priv = ssg_qxl_cursor_move_command_get_instance_private (self);

    priv->cursor.header.unique = 0;
    priv->cursor.header.type = SPICE_CURSOR_TYPE_COLOR32;
    priv->cursor.header.width = CURSOR_WIDTH;
    priv->cursor.header.height = CURSOR_HEIGHT;
    priv->cursor.header.hot_spot_x = 0;
    priv->cursor.header.hot_spot_y = 0;
    priv->cursor.data_size = CURSOR_WIDTH * CURSOR_HEIGHT * 4;

    // TODO +128 comment in test-display-base.c ???
    priv->cursor.chunk.data_size = priv->cursor.data_size;
    priv->cursor.chunk.prev_chunk = priv->cursor.chunk.next_chunk = 0;

    priv->cursor_command.release_info.id = (intptr_t)self;

    priv->cursor_command.type = QXL_CURSOR_MOVE;
    priv->cursor_command.u.position.x = x;
    priv->cursor_command.u.position.y = y;

    priv->ext.cmd.data = (intptr_t)&priv->cursor_command;
    priv->ext.cmd.type = QXL_CMD_CURSOR;
    priv->ext.group_id = MEM_SLOT_GROUP_ID;
    priv->ext.flags    = 0;

}

// TODO properties
SsgQXLCursorMoveCommand *ssg_qxl_cursor_move_command_new(int x, int y)
{
    SsgQXLCursorMoveCommand *self = g_object_new (SSG_TYPE_QXL_CURSOR_MOVE_COMMAND, NULL);
    SsgQXLCursorMoveCommandPrivate *priv = ssg_qxl_cursor_move_command_get_instance_private (self);

    build_qxl_cursor_move_command(self, x, y);

    return self;

}


