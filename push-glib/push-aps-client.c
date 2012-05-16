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
   PushApsClientMode mode;
   GTlsCertificate *tls_certificate;
   GError *tls_error;
   gchar *ssl_cert_file;
   gchar *ssl_key_file;
   GIOStream *feedback_stream;
   GIOStream *gateway_stream;
   GHashTable *results;
};

enum
{
   PROP_0,
   PROP_MODE,
   PROP_SSL_CERT_FILE,
   PROP_SSL_KEY_FILE,
   PROP_TLS_CERTIFICATE,
   LAST_PROP
};

enum
{
   IDENTITY_REMOVED,
   LAST_SIGNAL
};

static GParamSpec *gParamSpecs[LAST_PROP];
static guint       gSignals[LAST_SIGNAL];

static void push_aps_client_try_load_tls (PushApsClient *client);

static void
push_aps_client_connect_gateway_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
   GSimpleAsyncResult *simple = user_data;
   GSocketConnection *conn;
   GSocketClient *socket_client = (GSocketClient *)object;
   PushApsClient *client;
   GError *error = NULL;

   ENTRY;

   g_assert(G_IS_SOCKET_CLIENT(socket_client));
   g_assert(G_IS_SIMPLE_ASYNC_RESULT(simple));

   if (!(conn = g_socket_client_connect_to_host_finish(socket_client,
                                                       result,
                                                       &error))) {
      g_simple_async_result_take_error(simple, error);
   } else {
      client = PUSH_APS_CLIENT(g_async_result_get_source_object(user_data));
      g_clear_object(&client->priv->gateway_stream);
      client->priv->gateway_stream = G_IO_STREAM(conn);
      g_object_unref(client);
   }

   g_simple_async_result_set_op_res_gboolean(simple, !!conn);
   g_simple_async_result_complete_in_idle(simple);
   g_object_unref(simple);

   EXIT;
}

static void
push_aps_client_connect_event_cb (GSocketClient      *socket_client,
                                  GSocketClientEvent  event,
                                  GSocketConnectable *connectable,
                                  GIOStream          *connection,
                                  PushApsClient      *client)
{
   ENTRY;

   g_assert(G_IS_SOCKET_CLIENT(socket_client));
   g_assert(PUSH_IS_APS_CLIENT(client));

   if (event == G_SOCKET_CLIENT_TLS_HANDSHAKING) {
      g_tls_connection_set_certificate(G_TLS_CONNECTION(connection),
                                       client->priv->tls_certificate);
   }

   EXIT;
}

void
push_aps_client_connect_async (PushApsClient       *client,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data)
{
   PushApsClientPrivate *priv;
   GSimpleAsyncResult *simple;
   GSocketClient *socket_client;
   const gchar *host;

   ENTRY;

   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));

   priv = client->priv;

   if (priv->tls_error) {
      g_simple_async_report_gerror_in_idle(G_OBJECT(client),
                                           callback,
                                           user_data,
                                           priv->tls_error);
      EXIT;
   }

   if (!priv->tls_certificate) {
      g_simple_async_report_error_in_idle(G_OBJECT(client),
                                          callback,
                                          user_data,
                                          PUSH_APS_CLIENT_ERROR,
                                          PUSH_APS_CLIENT_ERROR_TLS_NOT_AVAILABLE,
                                          _("TLS has not yet been configured."));
      EXIT;
   }

   if (priv->gateway_stream) {
      g_simple_async_report_error_in_idle(G_OBJECT(client),
                                          callback,
                                          user_data,
                                          PUSH_APS_CLIENT_ERROR,
                                          PUSH_APS_CLIENT_ERROR_ALREADY_CONNECTED,
                                          _("The client is already connected."));
      EXIT;
   }

   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      push_aps_client_connect_async);

   socket_client = g_object_new(G_TYPE_SOCKET_CLIENT,
                                "family", G_SOCKET_FAMILY_IPV4,
                                "protocol", G_SOCKET_PROTOCOL_TCP,
                                "timeout", 60,
                                "tls", TRUE,
                                "type", G_SOCKET_TYPE_STREAM,
                                NULL);
   g_signal_connect(socket_client,
                    "event",
                    G_CALLBACK(push_aps_client_connect_event_cb),
                    client);
   host = (priv->mode == PUSH_APS_CLIENT_PRODUCTION) ?
          "gateway.push.apple.com" :
          "gateway.sandbox.push.apple.com";
   g_socket_client_connect_to_host_async(socket_client,
                                         host,
                                         2195,
                                         cancellable,
                                         push_aps_client_connect_gateway_cb,
                                         simple);
   g_object_unref(socket_client);

   EXIT;
}

gboolean
push_aps_client_connect_finish (PushApsClient  *client,
                                GAsyncResult   *result,
                                GError        **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   ENTRY;

   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   RETURN(ret);
}

static GByteArray *
push_aps_client_encode (PushApsClient *client,
                        const gchar   *device_token,
                        const gchar   *message)
{
   GByteArray *ret = NULL;
   guint16 b16;
   guchar *data;
   guint8 b8;
   gsize len;

   ENTRY;

   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), NULL);
   g_return_val_if_fail(device_token, NULL);
   g_return_val_if_fail(message, NULL);

   ret = g_byte_array_sized_new(64);

   b8 = 1;
   g_byte_array_append(ret, &b8, 1);

   data = g_base64_decode(device_token, &len);
   b16 = len;
   b16 = GUINT16_TO_LE(b16);
   g_byte_array_append(ret, (guint8 *)&b16, 2);
   g_byte_array_append(ret, (guint8 *)data, len);
   g_free(data);

   b16 = strlen(message);
   b16 = GUINT16_TO_LE(b16);
   g_byte_array_append(ret, (guint8 *)&b16, 2);
   g_byte_array_append(ret, (guint8 *)message, strlen(message));

   RETURN(ret);
}

/**
 * push_aps_client_deliver_async:
 * @client: A #PushApsClient.
 * @identity: A #PushApsIdentity.
 * @message: A #PushApsMessage.
 * @cancellable: (allow-none: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to execute upon completion.
 * @user_data: User data for @callback.
 *
 * Asynchronously requests that @message be delivered to @identity.
 * The message is serialized and sent via the Apple push notification
 * gateway who performs the actual delivery to the identified device.
 *
 * @callback MUST call push_aps_client_deliver_finish() with the
 * provided #GAsyncResult.
 */
void
push_aps_client_deliver_async (PushApsClient       *client,
                               PushApsIdentity     *identity,
                               PushApsMessage      *message,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data)
{
   PushApsClientPrivate *priv;
   GSimpleAsyncResult *simple;
   GOutputStream *stream;
   GByteArray *buffer;
   GError *error = NULL;
   gsize bytes_written;

   ENTRY;

   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   g_return_if_fail(PUSH_IS_APS_IDENTITY(identity));
   g_return_if_fail(PUSH_IS_APS_MESSAGE(message));
   g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));
   g_return_if_fail(callback);

   priv = client->priv;

   if (priv->tls_error) {
      g_simple_async_report_gerror_in_idle(G_OBJECT(client),
                                           callback,
                                           user_data,
                                           priv->tls_error);
      EXIT;
   }

   if (!priv->gateway_stream) {
      g_simple_async_report_error_in_idle(G_OBJECT(client),
                                          callback,
                                          user_data,
                                          PUSH_APS_CLIENT_ERROR,
                                          PUSH_APS_CLIENT_ERROR_NOT_CONNECTED,
                                          _("Not currently connected to "
                                            "APS gateway."));
      EXIT;
   }

   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      push_aps_client_deliver_async);
   g_simple_async_result_set_check_cancellable(simple, cancellable);

   buffer = push_aps_client_encode(client,
                                   push_aps_identity_get_device_token(identity),
                                   push_aps_message_get_json(message));

   stream = g_io_stream_get_output_stream(priv->gateway_stream);
   if (!g_output_stream_write_all(stream,
                                  buffer->data,
                                  buffer->len,
                                  &bytes_written,
                                  NULL, /* priv->shutdown, */
                                  &error)) {
      /* TODO: push_aps_client_fail(client); */
   }

   g_byte_array_free(buffer, TRUE);
   g_simple_async_result_complete_in_idle(simple);
   g_object_unref(simple);

   EXIT;
}

/**
 * push_aps_client_deliver_finish:
 * @client: A #PushApsClient.
 * @result: A #GAsyncResult.
 * @error: (out) (allow-none): A location for a #GError, or %NULL.
 *
 * Completes an asynchronous request to push_aps_client_deliver_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 */
gboolean
push_aps_client_deliver_finish (PushApsClient  *client,
                                GAsyncResult   *result,
                                GError        **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   ENTRY;

   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   RETURN(ret);
}

PushApsClientMode
push_aps_client_get_mode (PushApsClient *client)
{
   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), 0);
   return client->priv->mode;
}

static void
push_aps_client_set_mode (PushApsClient     *client,
                          PushApsClientMode  mode)
{
   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   g_return_if_fail((mode == PUSH_APS_CLIENT_PRODUCTION) ||
                    (mode == PUSH_APS_CLIENT_SANDBOX));
   client->priv->mode = mode;
}

/**
 * push_aps_client_get_ssl_cert_file:
 * @client: (in): A #PushApsClient.
 *
 * Fetches the "ssl-cert-file" property. This is a path to the file
 * containing the TLS certificate.
 *
 * Returns: A string which should not be modified or freed.
 */
const gchar *
push_aps_client_get_ssl_cert_file (PushApsClient *client)
{
   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), NULL);
   return client->priv->ssl_cert_file;
}

/**
 * push_aps_client_get_ssl_key_file:
 * @client: (in): A #PushApsClient.
 *
 * Fetches the "ssl-key-file" property. This is a path to the file
 * containing the TLS private key.
 *
 * Returns: A string which should not be modified or freed.
 */
const gchar *
push_aps_client_get_ssl_key_file (PushApsClient *client)
{
   g_return_val_if_fail(PUSH_IS_APS_CLIENT(client), NULL);
   return client->priv->ssl_key_file;
}

/**
 * push_aps_client_get_tls_certificate:
 * @client: (in): A #PushApsClient.
 *
 * Fetches the #GTlsCertificate used to communicate with the
 * Apple Push Notification gateway.
 *
 * Returns: (transfer none): A #GTlsCertificate.
 */
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
   push_aps_client_try_load_tls(client);
   EXIT;
}

static void
push_aps_client_set_ssl_key_file (PushApsClient *client,
                                  const gchar   *ssl_key_file)
{
   ENTRY;
   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   client->priv->ssl_key_file = g_strdup(ssl_key_file);
   push_aps_client_try_load_tls(client);
   EXIT;
}

static void
push_aps_client_set_tls_certificate (PushApsClient   *client,
                                     GTlsCertificate *tls_certificate)
{
   ENTRY;
   g_return_if_fail(PUSH_IS_APS_CLIENT(client));
   if (tls_certificate) {
      client->priv->tls_certificate = g_object_ref(tls_certificate);
   }
   EXIT;
}

static void
push_aps_client_try_load_tls (PushApsClient *client)
{
   PushApsClientPrivate *priv;
   GTlsCertificate *tls;
   GError *error = NULL;

   g_return_if_fail(PUSH_IS_APS_CLIENT(client));

   priv = client->priv;

   if (priv->ssl_cert_file && priv->ssl_key_file) {
      if (!(tls = g_tls_certificate_new_from_files(priv->ssl_cert_file,
                                                   priv->ssl_key_file,
                                                   &error))) {
         g_clear_error(&priv->tls_error);
         g_warning("TLS Certificate Error: %s", error->message);
         priv->tls_error = error;
      } else {
         push_aps_client_set_tls_certificate(client, tls);
         g_object_unref(tls);
      }
   }
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

   g_clear_error(&priv->tls_error);

   g_hash_table_unref(priv->results);
   priv->results = NULL;

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
   case PROP_MODE:
      g_value_set_enum(value, push_aps_client_get_mode(client));
      break;
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
   case PROP_MODE:
      push_aps_client_set_mode(client, g_value_get_enum(value));
      break;
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

   gParamSpecs[PROP_MODE] =
      g_param_spec_enum("mode",
                        _("Mode"),
                        _("The mode of the client."),
                        PUSH_TYPE_APS_CLIENT_MODE,
                        PUSH_APS_CLIENT_PRODUCTION,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
   g_object_class_install_property(object_class, PROP_MODE,
                                   gParamSpecs[PROP_MODE]);

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

   gSignals[IDENTITY_REMOVED] = g_signal_new("identity-removed",
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
   client->priv->mode = PUSH_APS_CLIENT_PRODUCTION;
   client->priv->results =
      g_hash_table_new_full(g_int_hash, g_int_equal,
                            g_free, g_object_unref);
   EXIT;
}

GType
push_aps_client_mode_get_type (void)
{
   static GType type_id;
   static gsize initialized = FALSE;
   static GEnumValue values[] = {
      { PUSH_APS_CLIENT_PRODUCTION,
        "PUSH_APS_CLIENT_PRODUCTION",
        "PRODUCTION" },
      { PUSH_APS_CLIENT_SANDBOX,
        "PUSH_APS_CLIENT_SANDBOX",
        "SANDBOX" },
      { 0 }
   };

   if (g_once_init_enter(&initialized)) {
      type_id = g_enum_register_static("PushApsClientMode", values);
      g_once_init_leave(&initialized, TRUE);
   }

   return type_id;
}

GQuark
push_aps_client_error_quark (void)
{
   return g_quark_from_static_string("push-aps-client-error-quark");
}
