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
