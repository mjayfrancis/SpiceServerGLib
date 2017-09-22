#include "ssg-qxl-draw-alpha-blend-command.h"

#include "ssg-qxl-image.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>


typedef struct _SsgQXLDrawAlphaBlendCommandPrivate {
    QXLCommandExt ext;
    QXLDrawable drawable;
    SsgQXLImage *image;
} SsgQXLDrawAlphaBlendCommandPrivate;

#define SSG_TYPE_QXL_DRAW_ALPHA_BLEND_COMMAND ssg_qxl_draw_alpha_blend_command_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLDrawAlphaBlendCommand, ssg_qxl_draw_alpha_blend_command, SSG_TYPE_QXL_COMMAND)

static void
ssg_qxl_draw_alpha_blend_command_finalize (GObject *object)
{
    SsgQXLDrawAlphaBlendCommandPrivate *priv = ssg_qxl_draw_alpha_blend_command_get_instance_private (SSG_QXL_DRAW_ALPHA_BLEND_COMMAND (object));

    g_object_unref(priv->image);
}

static QXLCommandExt *
helper_get_command_ext(SsgQXLDrawAlphaBlendCommand *command)
{
    SsgQXLDrawAlphaBlendCommandPrivate *priv = ssg_qxl_draw_alpha_blend_command_get_instance_private (command);

    return &priv->ext;
}

static void
ssg_qxl_draw_alpha_blend_command_class_init (SsgQXLDrawAlphaBlendCommandClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->finalize = ssg_qxl_draw_alpha_blend_command_finalize;

    SsgQXLCommandClass *ssg_qxl_command_class = SSG_QXL_COMMAND_CLASS (_class);
    ssg_qxl_command_class->helper_get_command_ext = helper_get_command_ext;
}

static void
ssg_qxl_draw_alpha_blend_command_init (SsgQXLDrawAlphaBlendCommand *object)
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


static void build_qxl_draw_alpha_blend_command(SsgQXLDrawAlphaBlendCommand *self, SsgQXLImage *source_image, QXLRect source_bbox, uint32_t surface_id,
                                                        QXLRect dest_bbox)
{
    SsgQXLDrawAlphaBlendCommandPrivate *priv = ssg_qxl_draw_alpha_blend_command_get_instance_private (self);

    QXLDrawable *drawable = &priv->drawable;

    QXLImage *qxl_image = ssg_qxl_image_get_qxl_image(source_image);

    drawable->surface_id      = surface_id;

    drawable->bbox            = dest_bbox;
    drawable->clip.type       = SPICE_CLIP_TYPE_NONE;
    drawable->effect          = QXL_EFFECT_BLEND;
    simple_set_release_info(&drawable->release_info, (intptr_t)self);
    drawable->type            = QXL_DRAW_ALPHA_BLEND;
    drawable->surfaces_dest[0] = -1;
    drawable->surfaces_dest[1] = -1;
    drawable->surfaces_dest[2] = -1;

    drawable->u.alpha_blend.alpha = 255;
    drawable->u.alpha_blend.src_bitmap      = (intptr_t)qxl_image;
    drawable->u.alpha_blend.src_area = source_bbox;

    if (SSG_IS_QXL_SURFACE_IMAGE(source_image)) {
        drawable->surfaces_dest[0] = qxl_image->surface_image.surface_id;
        drawable->surfaces_rects[0] = source_bbox;
    }

    set_cmd(&priv->ext, QXL_CMD_DRAW, (intptr_t)drawable);
}

// TODO properties
SsgQXLDrawAlphaBlendCommand *ssg_qxl_draw_alpha_blend_command_new (SsgQXLImage *source_image,
        guint32 source_x, guint32 source_y, guint32 source_width, guint32 source_height,
        guint32 dest_surface_id,
        guint32 dest_x, guint32 dest_y, guint32 dest_width, guint32 dest_height)
{
    SsgQXLDrawAlphaBlendCommand *self = g_object_new (SSG_TYPE_QXL_DRAW_ALPHA_BLEND_COMMAND, NULL);
    SsgQXLDrawAlphaBlendCommandPrivate *priv = ssg_qxl_draw_alpha_blend_command_get_instance_private (self);

    priv->image = source_image;
    g_object_ref(source_image);

    QXLRect source_bbox = {
        .left = source_x,
        .right = source_x + source_width,
        .top = source_y,
        .bottom = source_y + source_height
    };

    QXLRect dest_bbox = {
        .left = dest_x,
        .right = dest_x + dest_width,
        .top = dest_y,
        .bottom = dest_y + dest_height
    };

    build_qxl_draw_alpha_blend_command(self, source_image, source_bbox, dest_surface_id, dest_bbox);

    return self;

}


