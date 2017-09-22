#ifndef __SPICESERVERGLIB_QXLSURFACECREATECOMMAND_H__
#define __SPICESERVERGLIB_QXLSURFACECREATECOMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

#include "ssg-qxl-command.h"

G_BEGIN_DECLS

#define SSG_TYPE_QXL_SURFACE_CREATE_COMMAND ssg_qxl_surface_create_command_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLSurfaceCreateCommand, ssg_qxl_surface_create_command, SSG, QXL_SURFACE_CREATE_COMMAND, SsgQXLCommand)

typedef struct _SsgQXLSurfaceCreateCommand SsgQXLSurfaceCreateCommand;
typedef struct _SsgQXLSurfaceCreateCommandPrivate SsgQXLSurfaceCreateCommandPrivate;

struct _SsgQXLSurfaceCreateCommand {
        GObject parent;
};

struct _SsgQXLSurfaceCreateCommandClass {
        GObjectClass parent;
};

SsgQXLSurfaceCreateCommand *ssg_qxl_surface_create_command_new (int surface_id, cairo_surface_t *cairo_surface);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLSURFACECREATECOMMAND_H__ */
