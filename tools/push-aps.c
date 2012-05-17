/* push-aps.c
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
#include <push-glib/push-glib.h>
#include <stdlib.h>

static gchar         *gAlert;
static guint          gBadge;
static gchar         *gCertFile;
static gchar         *gKeyFile;
static gchar        **gDeviceTokens;
static gboolean       gSandbox;
static gchar         *gSound;
static GMainLoop     *gMainLoop;
static guint          gToSend;
static GOptionEntry   gEntries[] = {
   { "alert", 'a', 0, G_OPTION_ARG_STRING, &gAlert,
     N_("The alert text to display.") },
   { "badge", 'b', 0, G_OPTION_ARG_INT, &gBadge,
     N_("The badge number to display (0 to reset).") },
   { "cert-file", 'f', 0, G_OPTION_ARG_FILENAME, &gCertFile,
     N_("The certificate file for the APS TLS client.") },
   { "device-token", 'd', 0, G_OPTION_ARG_STRING_ARRAY, &gDeviceTokens,
     N_("The device token to deliver to (multiple okay).") },
   { "key-file", 'k', 0, G_OPTION_ARG_FILENAME, &gKeyFile,
     N_("The key file for the APS TLS client.") },
   { "sandbox", 's', 0, G_OPTION_ARG_NONE, &gSandbox,
     N_("Use the APS sandbox for delivery.") },
   { "sound", 'n', 0, G_OPTION_ARG_STRING, &gSound,
     N_("The sound file to play.") },
   { 0 }
};

static void
deliver_cb (GObject      *object,
            GAsyncResult *result,
            gpointer      user_data)
{
   PushApsClient *client = (PushApsClient *)object;
   GError *error = NULL;

   if (!push_aps_client_deliver_finish(client, result, &error)) {
      g_printerr("ERROR: %s\n", error->message);
      g_error_free(error);
   } else {
      g_print("Notification sent.\n");
   }

   if (--gToSend == 0) {
      g_main_loop_quit(gMainLoop);
   }
}

static void
connect_cb (GObject      *object,
            GAsyncResult *result,
            gpointer      user_data)
{
   PushApsIdentity *identity;
   PushApsMessage *message;
   PushApsClient *client = (PushApsClient *)object;
   GError *error = NULL;
   guint i;

   g_assert(PUSH_IS_APS_CLIENT(client));
   g_assert(G_IS_ASYNC_RESULT(result));

   if (!push_aps_client_connect_finish(client, result, &error)) {
      g_printerr("ERROR: %s\n", error->message);
      g_error_free(error);
      g_main_loop_quit(gMainLoop);
      return;
   }

   message = g_object_new(PUSH_TYPE_APS_MESSAGE,
                          "alert", gAlert,
                          "badge", gBadge,
                          "sound", gSound,
                          NULL);

   gToSend = g_strv_length(gDeviceTokens);

   for (i = 0; gDeviceTokens[i]; i++) {
      identity = push_aps_identity_new(gDeviceTokens[i]);
      push_aps_client_deliver_async(client, identity, message, NULL,
                                    deliver_cb, NULL);
      g_object_unref(identity);
   }

   g_object_unref(message);
}

gint
main (gint   argc,
      gchar *argv[])
{
   PushApsClientMode mode;
   GOptionContext *context;
   PushApsClient *client;
   GError *error = NULL;

   g_set_prgname("push-aps");
   g_set_application_name(_("Push APS"));

   context = g_option_context_new(_("- Send APS notifications."));
   g_option_context_add_main_entries(context, gEntries, NULL);
   if (!g_option_context_parse(context, &argc, &argv, &error)) {
      g_printerr("%s\n", error->message);
      g_error_free(error);
      return EXIT_FAILURE;
   }

   if (!gDeviceTokens || !gDeviceTokens[0]) {
      g_printerr("%s\n", _("Please provide at least one device token."));
      return EXIT_FAILURE;
   }

   g_type_init();

   gMainLoop = g_main_loop_new(NULL, FALSE);

   mode = gSandbox ? PUSH_APS_CLIENT_SANDBOX : PUSH_APS_CLIENT_PRODUCTION;
   client = g_object_new(PUSH_TYPE_APS_CLIENT,
                         "mode", mode,
                         "ssl-cert-file", gCertFile,
                         "ssl-key-file", gKeyFile,
                         NULL);
   push_aps_client_connect_async(client,
                                 NULL,
                                 connect_cb,
                                 NULL);

   g_main_loop_run(gMainLoop);

   g_object_unref(client);

   return EXIT_SUCCESS;
}
