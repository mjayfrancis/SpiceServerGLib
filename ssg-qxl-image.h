#ifndef __SPICESERVERGLIB_QXLIMAGE_H__
#define __SPICESERVERGLIB_QXLIMAGE_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include <spice.h>

G_BEGIN_DECLS

/* SsgQXLImage */

#define SSG_TYPE_QXL_IMAGE ssg_qxl_image_get_type()
G_DECLARE_DERIVABLE_TYPE (SsgQXLImage, ssg_qxl_image, SSG, QXL_IMAGE, GObject)

typedef struct _SsgQXLImagePrivate SsgQXLImagePrivate;

struct _SsgQXLImageClass {
        GObjectClass parent;
};

QXLImage *ssg_qxl_image_get_qxl_image(SsgQXLImage *self);

/* SsgQXLBitmapImage */

#define SSG_TYPE_QXL_BITMAP_IMAGE ssg_qxl_bitmap_image_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLBitmapImage, ssg_qxl_bitmap_image, SSG, QXL_BITMAP_IMAGE, SsgQXLImage)

typedef struct _SsgQXLBitmapImage SsgQXLBitmapImage;
typedef struct _SsgQXLBitmapImagePrivate SsgQXLBitmapImagePrivate;

struct _SsgQXLBitmapImage {
        GObject parent;
};

struct _SsgQXLBitmapImageClass {
        GObjectClass parent;
};

SsgQXLBitmapImage *ssg_qxl_bitmap_image_new (cairo_surface_t *cairo_surface);

/* SsgQXLSurfaceImage */

#define SSG_TYPE_QXL_SURFACE_IMAGE ssg_qxl_surface_image_get_type()
G_DECLARE_FINAL_TYPE (SsgQXLSurfaceImage, ssg_qxl_surface_image, SSG, QXL_SURFACE_IMAGE, SsgQXLImage)

typedef struct _SsgQXLSurfaceImage SsgQXLSurfaceImage;
typedef struct _SsgQXLSurfaceImagePrivate SsgQXLSurfaceImagePrivate;

struct _SsgQXLSurfaceImage {
        GObject parent;
};

struct _SsgQXLSurfaceImageClass {
        GObjectClass parent;
};

SsgQXLSurfaceImage *ssg_qxl_surface_image_new (guint surface_id);

G_END_DECLS

#endif /* __SPICESERVERGLIB_QXLIMAGE_H__ */
