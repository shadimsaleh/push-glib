/* push-aps-client.h
 *
 * Copyright (C) 2012 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PUSH_APS_CLIENT_H
#define PUSH_APS_CLIENT_H

#include <gio/gio.h>

#include "push-aps-identity.h"
#include "push-aps-message.h"

G_BEGIN_DECLS

#define PUSH_TYPE_APS_CLIENT            (push_aps_client_get_type())
#define PUSH_TYPE_APS_CLIENT_MODE       (push_aps_client_mode_get_type())
#define PUSH_APS_CLIENT_ERROR           (push_aps_client_error_quark())
#define PUSH_APS_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PUSH_TYPE_APS_CLIENT, PushApsClient))
#define PUSH_APS_CLIENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PUSH_TYPE_APS_CLIENT, PushApsClient const))
#define PUSH_APS_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PUSH_TYPE_APS_CLIENT, PushApsClientClass))
#define PUSH_IS_APS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PUSH_TYPE_APS_CLIENT))
#define PUSH_IS_APS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PUSH_TYPE_APS_CLIENT))
#define PUSH_APS_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PUSH_TYPE_APS_CLIENT, PushApsClientClass))

typedef struct _PushApsClient        PushApsClient;
typedef struct _PushApsClientClass   PushApsClientClass;
typedef struct _PushApsClientPrivate PushApsClientPrivate;
typedef enum   _PushApsClientError   PushApsClientError;
typedef enum   _PushApsClientMode    PushApsClientMode;

enum _PushApsClientError
{
   PUSH_APS_CLIENT_ERROR_NOT_CONNECTED = 1,
   PUSH_APS_CLIENT_ERROR_ALREADY_CONNECTED,
   PUSH_APS_CLIENT_ERROR_TLS_NOT_AVAILABLE,
};

enum _PushApsClientMode
{
   PUSH_APS_CLIENT_PRODUCTION = 1,
   PUSH_APS_CLIENT_SANDBOX    = 2,
};

struct _PushApsClient
{
   GObject parent;

   /*< private >*/
   PushApsClientPrivate *priv;
};

struct _PushApsClientClass
{
   GObjectClass parent_class;
};

void     push_aps_client_connect_async  (PushApsClient        *client,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
gboolean push_aps_client_connect_finish (PushApsClient        *client,
                                         GAsyncResult         *result,
                                         GError              **error);
void     push_aps_client_deliver_async  (PushApsClient        *client,
                                         PushApsIdentity      *identity,
                                         PushApsMessage       *message,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
gboolean push_aps_client_deliver_finish (PushApsClient        *client,
                                         GAsyncResult         *result,
                                         GError              **error);
GQuark   push_aps_client_error_quark    (void) G_GNUC_CONST;
GType    push_aps_client_get_type       (void) G_GNUC_CONST;
GType    push_aps_client_mode_get_type  (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PUSH_APS_CLIENT_H */
