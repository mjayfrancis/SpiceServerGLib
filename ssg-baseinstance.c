#include "ssg-baseinstance.h"

G_DEFINE_ABSTRACT_TYPE (SsgBaseInstance, ssg_baseinstance, G_TYPE_OBJECT)

static void
ssg_baseinstance_init (SsgBaseInstance *self)
{
}

static void
ssg_baseinstance_class_init (SsgBaseInstanceClass *object_class)
{
    object_class->helper_get_spice_baseinstance = NULL;
}


SpiceBaseInstance* ssg_baseinstance_get_spice_baseinstance (SsgBaseInstance *self)
{
    return SSG_BASEINSTANCE_GET_CLASS(self)->helper_get_spice_baseinstance(self);
}
