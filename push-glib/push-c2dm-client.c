/* push-c2dm-client.c
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

#include <glib/gi18n.h>

#include "push-c2dm-client.h"
#include "push-debug.h"

#define PUSH_C2DM_CLIENT_URL "https://android.apis.google.com/c2dm/send"

G_DEFINE_TYPE(PushC2dmClient, push_c2dm_client, SOUP_TYPE_SESSION_ASYNC)

struct _PushC2dmClientPrivate
{
   gchar *auth_token;
};

enum
{
   PROP_0,
   PROP_AUTH_TOKEN,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

const gchar *
push_c2dm_client_get_auth_token (PushC2dmClient *client)
{
   g_return_val_if_fail(PUSH_IS_C2DM_CLIENT(client), NULL);
   return client->priv->auth_token;
}

void
push_c2dm_client_set_auth_token (PushC2dmClient *client,
                                 const gchar    *auth_token)
{
   ENTRY;
   g_return_if_fail(PUSH_IS_C2DM_CLIENT(client));
   g_free(client->priv->auth_token);
   client->priv->auth_token = g_strdup(auth_token);
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_AUTH_TOKEN]);
   EXIT;
}

static void
push_c2dm_client_message_cb (SoupSession *session,
                             SoupMessage *message,
                             gpointer     user_data)
{
   GSimpleAsyncResult *simple = user_data;
   PushC2dmClient *client = (PushC2dmClient *)session;

   ENTRY;

   g_return_if_fail(PUSH_IS_C2DM_CLIENT(client));
   g_return_if_fail(SOUP_IS_MESSAGE(message));
   g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple));

   if (SOUP_STATUS_IS_SUCCESSFUL(message->status_code)) {
      g_simple_async_result_set_op_res_gboolean(simple, TRUE);
   } else {
      g_simple_async_result_set_error(simple,
                                      PUSH_C2DM_CLIENT_ERROR,
                                      PUSH_C2DM_CLIENT_ERROR_REQUEST_FAILED,
                                      _("C2DM request failed with code: %u"),
                                      message->status_code);
   }

   g_simple_async_result_complete_in_idle(simple);
   g_object_unref(simple);

   EXIT;
}

void
push_c2dm_client_deliver_async (PushC2dmClient      *client,
                                PushC2dmIdentity    *identity,
                                PushC2dmMessage     *message,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
   GSimpleAsyncResult *simple;
   const gchar *registration_id;
   SoupMessage *request;
   GHashTable *params;

   ENTRY;

   g_return_if_fail(PUSH_IS_C2DM_CLIENT(client));
   g_return_if_fail(PUSH_IS_C2DM_IDENTITY(identity));
   g_return_if_fail(PUSH_IS_C2DM_MESSAGE(message));
   g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
   g_return_if_fail(callback);

   registration_id = push_c2dm_identity_get_registration_id(identity);
   params = push_c2dm_message_build_params(message);
   g_hash_table_insert(params,
                       g_strdup("registration_id"),
                       g_strdup(registration_id));
   request = soup_form_request_new_from_hash(SOUP_METHOD_POST,
                                             PUSH_C2DM_CLIENT_URL,
                                             params);
   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      push_c2dm_client_deliver_async);
   g_simple_async_result_set_check_cancellable(simple, cancellable);
   soup_session_queue_message(SOUP_SESSION(client),
                              request,
                              push_c2dm_client_message_cb,
                              simple);
   g_hash_table_unref(params);

   EXIT;
}

gboolean
push_c2dm_client_deliver_finish (PushC2dmClient  *client,
                                 GAsyncResult    *result,
                                 GError         **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   ENTRY;

   g_return_val_if_fail(PUSH_IS_C2DM_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   RETURN(ret);
}

static void
push_c2dm_client_finalize (GObject *object)
{
   PushC2dmClientPrivate *priv;

   ENTRY;

   priv = PUSH_C2DM_CLIENT(object)->priv;

   g_free(priv->auth_token);

   G_OBJECT_CLASS(push_c2dm_client_parent_class)->finalize(object);

   EXIT;
}

static void
push_c2dm_client_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
   PushC2dmClient *client = PUSH_C2DM_CLIENT(object);

   switch (prop_id) {
   case PROP_AUTH_TOKEN:
      g_value_set_string(value, push_c2dm_client_get_auth_token(client));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_c2dm_client_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
   PushC2dmClient *client = PUSH_C2DM_CLIENT(object);

   switch (prop_id) {
   case PROP_AUTH_TOKEN:
      push_c2dm_client_set_auth_token(client, g_value_get_string(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_c2dm_client_class_init (PushC2dmClientClass *klass)
{
   GObjectClass *object_class;

   ENTRY;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = push_c2dm_client_finalize;
   object_class->get_property = push_c2dm_client_get_property;
   object_class->set_property = push_c2dm_client_set_property;
   g_type_class_add_private(object_class, sizeof(PushC2dmClientPrivate));

   gParamSpecs[PROP_AUTH_TOKEN] =
      g_param_spec_string("auth-token",
                          _("Auth Token"),
                          _("The authentication token to use with C2DM."),
                          NULL,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_AUTH_TOKEN,
                                   gParamSpecs[PROP_AUTH_TOKEN]);

   EXIT;
}

static void
push_c2dm_client_init (PushC2dmClient *client)
{
   ENTRY;
   client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client,
                                              PUSH_TYPE_C2DM_CLIENT,
                                              PushC2dmClientPrivate);
   EXIT;
}

GQuark
push_c2dm_client_error_quark (void)
{
   return g_quark_from_static_string("psuh-c2dm-client-error-quark");
}
