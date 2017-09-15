#ifndef __SPICESERVERGLIB_BASEINSTANCE_H__
#define __SPICESERVERGLIB_BASEINSTANCE_H__

#include <spice.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SSG_TYPE_BASEINSTANCE ssg_baseinstance_get_type()
G_DECLARE_DERIVABLE_TYPE(SsgBaseInstance, ssg_baseinstance, SSG, BASEINSTANCE, GObject)

typedef struct _SsgBaseInstancePrivate SsgBaseInstancePrivate;

struct _SsgBaseInstanceClass {
    GObjectClass parent;
    SpiceBaseInstance* (*helper_get_spice_baseinstance) (SsgBaseInstance *base_instance);
};

/**
 * ssg_baseinstance_get_spice_baseinstance: (skip)
 */
SpiceBaseInstance* ssg_baseinstance_get_spice_baseinstance(SsgBaseInstance *base_instance);

G_END_DECLS


#endif /* __SPICESERVERGLIB_BASEINSTANCE_H__ */
