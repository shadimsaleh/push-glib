/* push-c2dm.c
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

static gchar         *gAuthToken;
static gchar         *gCollapseKey;
static gboolean       gDelayWhileIdle;
static gchar        **gRegistrationIds;
static GMainLoop     *gMainLoop;
static guint          gToSend;
static GOptionEntry   gEntries[] = {
   { "auth-token", 'a', 0, G_OPTION_ARG_STRING, &gAuthToken,
     N_("The authentication token to provide to C2DM.") },
   { "collapse-key", 'c', 0, G_OPTION_ARG_STRING, &gCollapseKey,
     N_("The collapse_key to send C2DM for consolidating messages.") },
   { "delay-while-idle", 'd', 0, G_OPTION_ARG_NONE, &gDelayWhileIdle,
     N_("If the message should not be delivered until the device awakes.") },
   { "registration-id", 'r', 0, G_OPTION_ARG_STRING_ARRAY, &gRegistrationIds,
     N_("One or more registration_id to notify of message.") },
   { 0 }
};

static void
deliver_cb (GObject      *object,
            GAsyncResult *result,
            gpointer      user_data)
{
   PushC2dmClient *client = (PushC2dmClient *)object;
   GError *error = NULL;

   if (!push_c2dm_client_deliver_finish(client, result, &error)) {
      g_printerr("ERROR: %s\n", error->message);
      g_error_free(error);
   } else {
      g_print("Notification sent.\n");
   }

   if (--gToSend == 0) {
      g_main_loop_quit(gMainLoop);
   }
}

gint
main (gint   argc,
      gchar *argv[])
{
   PushC2dmIdentity *identity;
   PushC2dmMessage *message;
   GOptionContext *context;
   PushC2dmClient *client;
   GError *error = NULL;
   guint i;

   g_set_prgname("push-c2dm");
   g_set_application_name(_("Push C2DM"));

   context = g_option_context_new(_("- Send C2DM notifications."));
   g_option_context_add_main_entries(context, gEntries, NULL);
   if (!g_option_context_parse(context, &argc, &argv, &error)) {
      g_printerr("%s\n", error->message);
      g_error_free(error);
      return EXIT_FAILURE;
   }

   if (!gRegistrationIds || !gRegistrationIds[0]) {
      g_printerr("%s\n", _("Please provide at least one registration-id."));
      return EXIT_FAILURE;
   }

   g_type_init();

   gMainLoop = g_main_loop_new(NULL, FALSE);

   client = g_object_new(PUSH_TYPE_C2DM_CLIENT,
                         "auth-token", gAuthToken,
                         NULL);

   message = g_object_new(PUSH_TYPE_C2DM_MESSAGE,
                          "collapse-key", gCollapseKey,
                          "delay-while-idle", gDelayWhileIdle,
                          NULL);

   gToSend = g_strv_length(gRegistrationIds);

   for (i = 0; gRegistrationIds[i]; i++) {
      identity = g_object_new(PUSH_TYPE_C2DM_IDENTITY,
                              "registration-id", gRegistrationIds[i],
                              NULL);
      push_c2dm_client_deliver_async(client,
                                     identity,
                                     message,
                                     NULL,
                                     deliver_cb,
                                     NULL);
      g_object_unref(identity);
   }

   g_main_loop_run(gMainLoop);

   g_object_unref(client);

   return EXIT_SUCCESS;
}
