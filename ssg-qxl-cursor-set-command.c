#include "ssg-qxl-cursor-set-command.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>

typedef struct _SsgQXLCursorSetCommandPrivate {
    QXLCommandExt ext;
    QXLCursorCmd cursor_command;
    QXLCursor *cursor;
} SsgQXLCursorSetCommandPrivate;

#define SSG_TYPE_QXL_CURSOR_SET_COMMAND ssg_qxl_cursor_set_command_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLCursorSetCommand, ssg_qxl_cursor_set_command, SSG_TYPE_QXL_COMMAND)

static void
ssg_qxl_cursor_set_command_finalize (GObject *object)
{
    SsgQXLCursorSetCommandPrivate *priv = ssg_qxl_cursor_set_command_get_instance_private (SSG_QXL_CURSOR_SET_COMMAND (object));

    g_free(priv->cursor);
}

static QXLCommandExt *
helper_get_command_ext(SsgQXLCursorSetCommand *command)
{
    SsgQXLCursorSetCommandPrivate *priv = ssg_qxl_cursor_set_command_get_instance_private (command);

    return &priv->ext;
}

static void
ssg_qxl_cursor_set_command_class_init (SsgQXLCursorSetCommandClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->finalize = ssg_qxl_cursor_set_command_finalize;

    SsgQXLCommandClass *ssg_qxl_command_class = SSG_QXL_COMMAND_CLASS (_class);
    ssg_qxl_command_class->helper_get_command_ext = helper_get_command_ext;
}

static void
ssg_qxl_cursor_set_command_init (SsgQXLCursorSetCommand *object)
{

}


#define MEM_SLOT_GROUP_ID 0

static void build_qxl_cursor_set_command(SsgQXLCursorSetCommand *self, cairo_surface_t *cairo_surface, int hot_x, int hot_y, int x, int y)
{
    SsgQXLCursorSetCommandPrivate *priv = ssg_qxl_cursor_set_command_get_instance_private (self);

    int width = cairo_image_surface_get_width(cairo_surface);
    int height = cairo_image_surface_get_height(cairo_surface);
    int size = width * height * 4;

    priv->cursor = g_malloc0(sizeof(*priv->cursor) + size);

    priv->cursor->header.unique = 0;
    priv->cursor->header.type = SPICE_CURSOR_TYPE_ALPHA;
    priv->cursor->header.width = width;
    priv->cursor->header.height = height;
    priv->cursor->header.hot_spot_x = hot_x;
    priv->cursor->header.hot_spot_y = hot_y;
    priv->cursor->data_size = size;

    // TODO +128 comment in test-display-base.c ???
    priv->cursor->chunk.data_size = priv->cursor->data_size;
    priv->cursor->chunk.prev_chunk = priv->cursor->chunk.next_chunk = 0;
    memcpy(priv->cursor->chunk.data, cairo_image_surface_get_data(cairo_surface), size);

    priv->cursor_command.release_info.id = (intptr_t)self;

    priv->cursor_command.type = QXL_CURSOR_SET;
    priv->cursor_command.u.set.position.x = x + hot_x;
    priv->cursor_command.u.set.position.y = y + hot_y;
    priv->cursor_command.u.set.visible = TRUE;
    priv->cursor_command.u.set.shape = (intptr_t)priv->cursor;

    priv->ext.cmd.data = (intptr_t)&priv->cursor_command;
    priv->ext.cmd.type = QXL_CMD_CURSOR;
    priv->ext.group_id = MEM_SLOT_GROUP_ID;
    priv->ext.flags    = 0;

}

// TODO properties
SsgQXLCursorSetCommand *ssg_qxl_cursor_set_command_new(cairo_surface_t *cairo_surface, int hot_x, int hot_y, int x, int y)
{
    SsgQXLCursorSetCommand *self = g_object_new (SSG_TYPE_QXL_CURSOR_SET_COMMAND, NULL);
    SsgQXLCursorSetCommandPrivate *priv = ssg_qxl_cursor_set_command_get_instance_private (self);

    build_qxl_cursor_set_command(self, cairo_surface, hot_x, hot_y, x, y);

    return self;

}


