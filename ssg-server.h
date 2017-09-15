#ifndef __SPICESERVERGLIB_SERVER_H__
#define __SPICESERVERGLIB_SERVER_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <cairo.h>

#include "ssg-baseinstance.h"
#include "ssg-enum-types.h"

G_BEGIN_DECLS

#define SSG_TYPE_SERVER ssg_server_get_type()
G_DECLARE_FINAL_TYPE (SsgServer, ssg_server, SSG, SERVER, GObject)

typedef struct _SsgServer		SsgServer;
typedef struct _SsgServerPrivate	SsgServerPrivate;

struct _SsgServer {
    GObject parent;
};

struct _SsgServerClass {
    GObjectClass parent;
};

gint ssg_server_set_ticket(SsgServer *server, const gchar *passwd, gint lifetime, gboolean fail_if_connected, gboolean disconnect_if_connected);
gint ssg_server_add_client(SsgServer *server, gint socket, gboolean skip_auth);
gint ssg_server_add_ssl_client(SsgServer *server, gint socket, gboolean skip_auth);
void ssg_server_vm_start(SsgServer *server);
void ssg_server_vm_stop(SsgServer *server);
gint ssg_server_add_interface(SsgServer *server, SsgBaseInstance *instance);
gint ssg_server_remove_interface(SsgServer *server, SsgBaseInstance *instance);
void ssg_server_destroy(SsgServer *server);

G_END_DECLS

#endif /* __SPICESERVERGLIB_SERVER_H__ */
