#ifndef __SPICESERVERGLIB_QXLDRAWALPHABLENDCOMMAND_H__
#define __SPICESERVERGLIB_QXLDRAWALPHABLENDCOMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

#include "ssg-qxl-command.h"
#include "ssg-qxl-image.h"

G_BEGIN_DECLS

#define SSG_TYPE_QXL_DRAW_ALPHA_BLEND_COMMAND ssg_qxl_draw_alpha_blend_command_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLDrawAlphaBlendCommand, ssg_qxl_draw_alpha_blend_command, SSG, QXL_DRAW_ALPHA_BLEND_COMMAND, SsgQXLCommand)

typedef struct _SsgQXLDrawAlphaBlendCommand SsgQXLDrawAlphaBlendCommand;
typedef struct _SsgQXLDrawAlphaBlendCommandPrivate SsgQXLDrawAlphaBlendCommandPrivate;

struct _SsgQXLDrawAlphaBlendCommand {
        GObject parent;
};

struct _SsgQXLDrawAlphaBlendCommandClass {
        GObjectClass parent;
};

SsgQXLDrawAlphaBlendCommand *ssg_qxl_draw_alpha_blend_command_new (SsgQXLImage *source_image,
        guint32 source_x, guint32 source_y, guint32 source_width, guint32 source_height,
        guint32 dest_surface_id,
        guint32 dest_x, guint32 dest_y, guint32 dest_width, guint32 dest_height);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLDRAWALPHABLENDCOMMAND_H__ */
