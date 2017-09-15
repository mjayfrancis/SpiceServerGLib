#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <spice.h>

#include "ssg-qxl-command.h"


G_DEFINE_ABSTRACT_TYPE (SsgQXLCommand, ssg_qxl_command, G_TYPE_OBJECT)

static void
ssg_qxl_command_init (SsgQXLCommand *self)
{
}

static void
ssg_qxl_command_class_init (SsgQXLCommandClass *object_class)
{
    object_class->helper_get_command_ext = NULL;
}

/**
 * ssg_qxl_command_get_command_ext: (skip)
 */
QXLCommandExt* ssg_qxl_command_get_command_ext (SsgQXLCommand *self)
{
    return SSG_QXL_COMMAND_GET_CLASS(self)->helper_get_command_ext(self);
}
