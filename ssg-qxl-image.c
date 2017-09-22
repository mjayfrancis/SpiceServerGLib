#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>

#include "ssg-qxl-image.h"

/* SsgQXLImage */

typedef struct _SsgQXLImagePrivate {
    QXLImage image;
} SsgQXLImagePrivate;


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (SsgQXLImage, ssg_qxl_image, G_TYPE_OBJECT)

static void
ssg_qxl_image_class_init (SsgQXLImageClass *object_class)
{

}

static void
ssg_qxl_image_init (SsgQXLImage *self)
{

}

/**
 * ssg_qxl_image_get_qxl_image: (skip)
 * Returns: (transfer none):
 */
QXLImage *ssg_qxl_image_get_qxl_image(SsgQXLImage *self)
{
    SsgQXLImagePrivate *priv = ssg_qxl_image_get_instance_private (SSG_QXL_IMAGE (self));
    return &priv->image;
}

/* SsgQXLBitmapImage */

typedef struct _SsgQXLBitmapImagePrivate {
    cairo_surface_t *cairo_surface;
} SsgQXLBitmapImagePrivate;

#define SSG_TYPE_QXL_BITMAP_IMAGE ssg_qxl_bitmap_image_get_type()
G_DEFINE_TYPE_WITH_PRIVATE (SsgQXLBitmapImage, ssg_qxl_bitmap_image, SSG_TYPE_QXL_IMAGE)

static void
ssg_qxl_bitmap_image_constructed (GObject *object)
{
    G_OBJECT_CLASS(ssg_qxl_bitmap_image_parent_class)->constructed(object);
}

static void
ssg_qxl_bitmap_image_finalize (GObject *object)
{
    SsgQXLBitmapImagePrivate *priv = ssg_qxl_bitmap_image_get_instance_private (SSG_QXL_BITMAP_IMAGE (object));

    cairo_surface_destroy (priv->cairo_surface);
}

static void
ssg_qxl_bitmap_image_class_init (SsgQXLBitmapImageClass *_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(_class);

    object_class->constructed = ssg_qxl_bitmap_image_constructed;
    object_class->finalize = ssg_qxl_bitmap_image_finalize;
}

static void
ssg_qxl_bitmap_image_init (SsgQXLBitmapImage *object)
{

}

static int unique = 1;

guint8 spice_bitmap_format_for_cairo_surface(cairo_surface_t *cairo_surface)
{
    switch (cairo_image_surface_get_format(cairo_surface)) {
        case CAIRO_FORMAT_ARGB32:
            return SPICE_BITMAP_FMT_RGBA;
        case CAIRO_FORMAT_RGB24:
            return SPICE_BITMAP_FMT_24BIT;
        case CAIRO_FORMAT_A8:
            return SPICE_BITMAP_FMT_8BIT_A;
        case CAIRO_FORMAT_A1:
            switch (SPICE_ENDIAN) {
                case SPICE_ENDIAN_LITTLE:
                    return SPICE_BITMAP_FMT_1BIT_LE;
                case SPICE_ENDIAN_BIG:
                    return SPICE_BITMAP_FMT_1BIT_BE;
            }
            return SPICE_BITMAP_FMT_INVALID;
        case CAIRO_FORMAT_RGB16_565:
            return SPICE_BITMAP_FMT_16BIT;
        case CAIRO_FORMAT_RGB30:
            // Fall through - not supported
        default:
            return SPICE_BITMAP_FMT_INVALID;
    }
}

// TODO properties
SsgQXLBitmapImage *ssg_qxl_bitmap_image_new (cairo_surface_t *cairo_surface)
{
    SsgQXLBitmapImage *self = g_object_new (SSG_TYPE_QXL_BITMAP_IMAGE, NULL);
    SsgQXLImagePrivate *parent_priv = ssg_qxl_image_get_instance_private (SSG_QXL_IMAGE(self));
    SsgQXLBitmapImagePrivate *priv = ssg_qxl_bitmap_image_get_instance_private (self);
    QXLImage *image = &parent_priv->image;

    priv->cairo_surface = cairo_surface;

    cairo_surface_reference (cairo_surface);

    QXL_SET_IMAGE_ID(image, QXL_IMAGE_GROUP_DEVICE, unique++);
    image->descriptor.type   = SPICE_IMAGE_TYPE_BITMAP;
    image->bitmap.flags      = QXL_BITMAP_DIRECT | QXL_BITMAP_TOP_DOWN;
    image->bitmap.stride     = cairo_image_surface_get_stride(cairo_surface);
    image->descriptor.width  = image->bitmap.x = cairo_image_surface_get_width(cairo_surface);
    image->descriptor.height = image->bitmap.y = cairo_image_surface_get_height(cairo_surface);
    image->bitmap.data       = (intptr_t)cairo_image_surface_get_data (cairo_surface);
    image->bitmap.format     = spice_bitmap_format_for_cairo_surface(cairo_surface);

    return self;

}

/* SsgQXLSurfaceImage */

#define SSG_TYPE_QXL_SURFACE_IMAGE ssg_qxl_surface_image_get_type()
G_DEFINE_TYPE (SsgQXLSurfaceImage, ssg_qxl_surface_image, SSG_TYPE_QXL_IMAGE)

static void
ssg_qxl_surface_image_class_init (SsgQXLSurfaceImageClass *_class)
{
    // Nothing to do
}

static void
ssg_qxl_surface_image_init (SsgQXLSurfaceImage *object)
{
    // Nothing to do
}

// TODO properties
SsgQXLSurfaceImage *ssg_qxl_surface_image_new (guint surface_id)
{
    SsgQXLSurfaceImage *self = g_object_new (SSG_TYPE_QXL_SURFACE_IMAGE, NULL);
    SsgQXLImagePrivate *parent_priv = ssg_qxl_image_get_instance_private (SSG_QXL_IMAGE(self));
    QXLImage *image = &parent_priv->image;

    image->descriptor.type          = SPICE_IMAGE_TYPE_SURFACE;
    image->surface_image.surface_id = surface_id;

    return self;

}

