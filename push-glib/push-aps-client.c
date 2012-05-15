/* push-aps-client.c
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

#include "push-aps-client.h"
#include "push-debug.h"

G_DEFINE_TYPE(PushApsClient, push_aps_client, G_TYPE_OBJECT)

struct _PushApsClientPrivate
{
   gchar *ssl_cert_file;
   gchar *ssl_key_file;
   GTlsCertificate *tls_certificate;
};

enum
{
   PROP_0,
   PROP_SSL_CERT_FILE,
   PROP_SSL_KEY_FILE,
   PROP_TLS_CERTIFICATE,
   LAST_PROP
};

enum
{
   DEVICE_REMOVED,
   LAST_SIGNAL
};

static GParamSpec *gParamSpecs[LAST_PROP];
static guint       gSignals[LAST_SIGNAL];

const gchar *
push_aps_client_get_ssl_cert_file (PushApsClient *client)
{
   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), NULL);
   return client->priv->ssl_cert_file;
}

const gchar *
push_aps_client_get_ssl_key_file (PushApsClient *client)
{
   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), NULL);
   return client->priv->ssl_key_file;
}

GTlsCertificate *
push_aps_client_get_tls_certificate (PushApsClient *client)
{
   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), NULL);
   return client->priv->tls_certificate;
}

static void
push_aps_client_set_ssl_cert_file (PushApsClient *client,
                                   const gchar   *ssl_cert_file)
{
   ENTRY;
   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   client->priv->ssl_cert_file = g_strdup(ssl_cert_file);
   EXIT;
}

static void
push_aps_client_set_ssl_key_file (PushApsClient *client,
                                  const gchar   *ssl_key_file)
{
   ENTRY;
   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   client->priv->ssl_key_file = g_strdup(ssl_key_file);
   EXIT;
}

static void
push_aps_client_set_tls_certificate (PushApsClient   *client,
                                     GTlsCertificate *tls_certificate)
{
   ENTRY;
   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   client->priv->tls_certificate =
      tls_certificate ? g_object_ref(tls_certificate) : NULL;
   EXIT;
}

static void
push_aps_client_finalize (GObject *object)
{
   PushApsClientPrivate *priv;

   ENTRY;

   priv = PUSH_APS_CLIENT(object)->priv;

   g_free(priv->ssl_cert_file);
   priv->ssl_cert_file = NULL;

   g_free(priv->ssl_key_file);
   priv->ssl_key_file = NULL;

   g_clear_object(&priv->tls_certificate);

   G_OBJECT_CLASS(push_aps_client_parent_class)->finalize(object);

   EXIT;
}

static void
push_aps_client_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
   PushApsClient *client = PUSH_APS_CLIENT(object);

   switch (prop_id) {
   case PROP_SSL_CERT_FILE:
      g_value_set_string(value, push_aps_client_get_ssl_cert_file(client));
      break;
   case PROP_SSL_KEY_FILE:
      g_value_set_string(value, push_aps_client_get_ssl_key_file(client));
      break;
   case PROP_TLS_CERTIFICATE:
      g_value_set_object(value, push_aps_client_get_tls_certificate(client));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_aps_client_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
   PushApsClient *client = PUSH_APS_CLIENT(object);

   switch (prop_id) {
   case PROP_SSL_CERT_FILE:
      push_aps_client_set_ssl_cert_file(client, g_value_get_string(value));
      break;
   case PROP_SSL_KEY_FILE:
      push_aps_client_set_ssl_key_file(client, g_value_get_string(value));
      break;
   case PROP_TLS_CERTIFICATE:
      push_aps_client_set_tls_certificate(client, g_value_get_object(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_aps_client_class_init (PushApsClientClass *klass)
{
   GObjectClass *object_class;

   ENTRY;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = push_aps_client_finalize;
   object_class->get_property = push_aps_client_get_property;
   object_class->set_property = push_aps_client_set_property;
   g_type_class_add_private(object_class, sizeof(PushApsClientPrivate));

   gParamSpecs[PROP_SSL_CERT_FILE] =
      g_param_spec_string("ssl-cert-file",
                          _("SSL Certificate File"),
                          _("Path to SSL certificate file."),
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
   g_object_class_install_property(object_class, PROP_SSL_CERT_FILE,
                                   gParamSpecs[PROP_SSL_CERT_FILE]);

   gParamSpecs[PROP_SSL_KEY_FILE] =
      g_param_spec_string("ssl-key-file",
                          _("SSL Key File"),
                          _("Path to SSL key file."),
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
   g_object_class_install_property(object_class, PROP_SSL_KEY_FILE,
                                   gParamSpecs[PROP_SSL_KEY_FILE]);

   gParamSpecs[PROP_TLS_CERTIFICATE] =
      g_param_spec_object("tls-certificate",
                          _("TLS Certificate"),
                          _("A TLS certificate to communicate with."),
                          G_TYPE_TLS_CERTIFICATE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
   g_object_class_install_property(object_class, PROP_TLS_CERTIFICATE,
                                   gParamSpecs[PROP_TLS_CERTIFICATE]);

   gSignals[DEVICE_REMOVED] = g_signal_new("device-removed",
                                           PUSH_TYPE_APS_CLIENT,
                                           G_SIGNAL_RUN_FIRST,
                                           0,
                                           NULL,
                                           NULL,
                                           g_cclosure_marshal_VOID__OBJECT,
                                           G_TYPE_NONE,
                                           1,
                                           PUSH_TYPE_APS_IDENTITY);

   EXIT;
}

static void
push_aps_client_init (PushApsClient *client)
{
   ENTRY;
   client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client,
                                              PUSH_TYPE_APS_CLIENT,
                                              PushApsClientPrivate);
   EXIT;
}
