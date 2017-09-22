#include "ssg-qxl-surface-create-command.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>


typedef struct _SsgQXLSurfaceCreateCommandPrivate {
    QXLCommandExt ext;
    QXLSurfaceCmd surface_cmd;
    QXLImage image;
    cairo_surface_t *cairo_surface;
} SsgQXLSurfaceCreateCommandPrivate;

#define SSG_TYPE_QXL_SURFACE_CREATE_COMMAND ssg_qxl_surface_create_command_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLSurfaceCreateCommand, ssg_qxl_surface_create_command, SSG_TYPE_QXL_COMMAND)

static void
ssg_qxl_surface_create_command_finalize (GObject *object)
{
    SsgQXLSurfaceCreateCommandPrivate *priv = ssg_qxl_surface_create_command_get_instance_private (SSG_QXL_SURFACE_CREATE_COMMAND (object));

    cairo_surface_destroy (priv->cairo_surface);
}

static QXLCommandExt *
helper_get_command_ext(SsgQXLSurfaceCreateCommand *command)
{
    SsgQXLSurfaceCreateCommandPrivate *priv = ssg_qxl_surface_create_command_get_instance_private (command);

    return &priv->ext;
}

static void
ssg_qxl_surface_create_command_class_init (SsgQXLSurfaceCreateCommandClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->finalize = ssg_qxl_surface_create_command_finalize;

    SsgQXLCommandClass *ssg_qxl_command_class = SSG_QXL_COMMAND_CLASS (_class);
    ssg_qxl_command_class->helper_get_command_ext = helper_get_command_ext;
}

static void
ssg_qxl_surface_create_command_init (SsgQXLSurfaceCreateCommand *object)
{

}


#define MEM_SLOT_GROUP_ID 0

static int unique = 1;

static void set_cmd(QXLCommandExt *ext, uint32_t type, QXLPHYSICAL data)
{
    ext->cmd.type = type;
    ext->cmd.data = data;
    ext->cmd.padding = 0;
    ext->group_id = MEM_SLOT_GROUP_ID;
    ext->flags = 0;
}

static void simple_set_release_info(QXLReleaseInfo *info, intptr_t ptr)
{
    info->id = ptr;
}

// TODO properties
SsgQXLSurfaceCreateCommand *ssg_qxl_surface_create_command_new (int surface_id, cairo_surface_t *cairo_surface)
{
    SsgQXLSurfaceCreateCommand *self = g_object_new (SSG_TYPE_QXL_SURFACE_CREATE_COMMAND, NULL);
    SsgQXLSurfaceCreateCommandPrivate *priv = ssg_qxl_surface_create_command_get_instance_private (self);

    cairo_surface_reference (cairo_surface);

    QXLSurfaceCmd *surface_cmd = &priv->surface_cmd;
    int bpp = 4;

    simple_set_release_info(&surface_cmd->release_info, (intptr_t) self);
    surface_cmd->type = QXL_SURFACE_CMD_CREATE;
    surface_cmd->flags = 0; // ?
    surface_cmd->surface_id = surface_id;
    surface_cmd->u.surface_create.format = SPICE_SURFACE_FMT_32_ARGB;
    surface_cmd->u.surface_create.width = cairo_image_surface_get_width (cairo_surface);
    surface_cmd->u.surface_create.height = cairo_image_surface_get_height (cairo_surface);
    surface_cmd->u.surface_create.stride = -cairo_image_surface_get_stride (cairo_surface);
    surface_cmd->u.surface_create.data = (intptr_t) cairo_image_surface_get_data (cairo_surface);

    set_cmd(&priv->ext, QXL_CMD_SURFACE, (intptr_t) &priv->surface_cmd);

    return self;

}


