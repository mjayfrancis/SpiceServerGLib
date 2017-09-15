#include "ssg-qxl-draw-copy-command.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>


typedef struct _SsgQXLDrawCopyCommandPrivate {
    QXLCommandExt ext;
    QXLDrawable drawable;
    QXLImage image;
    cairo_surface_t *cairo_surface;
} SsgQXLDrawCopyCommandPrivate;

#define SSG_TYPE_QXL_DRAW_COPY_COMMAND ssg_qxl_draw_copy_command_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLDrawCopyCommand, ssg_qxl_draw_copy_command, SSG_TYPE_QXL_COMMAND)

static void
ssg_qxl_draw_copy_command_finalize (GObject *object)
{
    SsgQXLDrawCopyCommandPrivate *priv = ssg_qxl_draw_copy_command_get_instance_private (SSG_QXL_DRAW_COPY_COMMAND (object));

    cairo_surface_destroy (priv->cairo_surface);
}

static QXLCommandExt *
helper_get_command_ext(SsgQXLDrawCopyCommand *command)
{
    SsgQXLDrawCopyCommandPrivate *priv = ssg_qxl_draw_copy_command_get_instance_private (command);

    return &priv->ext;
}

static void
ssg_qxl_draw_copy_command_class_init (SsgQXLDrawCopyCommandClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->finalize = ssg_qxl_draw_copy_command_finalize;

    SsgQXLCommandClass *ssg_qxl_command_class = SSG_QXL_COMMAND_CLASS (_class);
    ssg_qxl_command_class->helper_get_command_ext = helper_get_command_ext;
}

static void
ssg_qxl_draw_copy_command_init (SsgQXLDrawCopyCommand *object)
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


static void build_qxl_draw_copy_command(SsgQXLDrawCopyCommand *self, uint32_t surface_id,
                                                        QXLRect bbox,
                                                        cairo_surface_t *cairo_surface)
{
    SsgQXLDrawCopyCommandPrivate *priv = ssg_qxl_draw_copy_command_get_instance_private (self);

    QXLDrawable *drawable;
    QXLImage *image;
    uint32_t bw, bh;

    bh = bbox.bottom - bbox.top;
    bw = bbox.right - bbox.left;

    priv->cairo_surface = cairo_surface;
    drawable = &priv->drawable;
    image    = &priv->image;

    drawable->surface_id      = surface_id;

    drawable->bbox            = bbox;
    drawable->clip.type       = SPICE_CLIP_TYPE_NONE;
    drawable->effect          = QXL_EFFECT_OPAQUE;
    simple_set_release_info(&drawable->release_info, (intptr_t)self);
    drawable->type            = QXL_DRAW_COPY;
    drawable->surfaces_dest[0] = -1;
    drawable->surfaces_dest[1] = -1;
    drawable->surfaces_dest[2] = -1;

    drawable->u.copy.rop_descriptor  = SPICE_ROPD_OP_PUT;
    drawable->u.copy.src_bitmap      = (intptr_t)image;
    drawable->u.copy.src_area.right  = bw;
    drawable->u.copy.src_area.bottom = bh;

    QXL_SET_IMAGE_ID(image, QXL_IMAGE_GROUP_DEVICE, unique++);
    image->descriptor.type   = SPICE_IMAGE_TYPE_BITMAP;
    image->bitmap.flags      = QXL_BITMAP_DIRECT | QXL_BITMAP_TOP_DOWN;
    image->bitmap.stride     = bw * 4;
    image->descriptor.width  = image->bitmap.x = bw;
    image->descriptor.height = image->bitmap.y = bh;
    image->bitmap.data = (intptr_t) cairo_image_surface_get_data (cairo_surface);
    image->bitmap.palette = 0;
    image->bitmap.format = SPICE_BITMAP_FMT_32BIT;

    set_cmd(&priv->ext, QXL_CMD_DRAW, (intptr_t)drawable);
}

// TODO properties
SsgQXLDrawCopyCommand *ssg_qxl_draw_copy_command_new (guint32 surface_id, guint32 x, guint32 y, cairo_surface_t *cairo_surface, guint32 width, guint32 height)
{
    SsgQXLDrawCopyCommand *self = g_object_new (SSG_TYPE_QXL_DRAW_COPY_COMMAND, NULL);
    SsgQXLDrawCopyCommandPrivate *priv = ssg_qxl_draw_copy_command_get_instance_private (self);

    cairo_surface_reference (cairo_surface);

    QXLRect bbox = {
        .left = x,
        .right = x + width,
        .top = y,
        .bottom = y + height
    };

    build_qxl_draw_copy_command(self, surface_id, bbox, cairo_surface);

    return self;

}


