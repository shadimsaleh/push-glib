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

G_DEFINE_TYPE(PushApsClient, push_aps_client, G_TYPE_OBJECT)

struct _PushApsClientPrivate
{
   gpointer dummy;
};

enum
{
   PROP_0,
   LAST_PROP
};

//static GParamSpec *gParamSpecs[LAST_PROP];

static void
push_aps_client_finalize (GObject *object)
{
   G_OBJECT_CLASS(push_aps_client_parent_class)->finalize(object);
}

static void
push_aps_client_class_init (PushApsClientClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = push_aps_client_finalize;
   g_type_class_add_private(object_class, sizeof(PushApsClientPrivate));
}

static void
push_aps_client_init (PushApsClient *client)
{
   client->priv =
      G_TYPE_INSTANCE_GET_PRIVATE(client,
                                  PUSH_TYPE_APS_CLIENT,
                                  PushApsClientPrivate);
}
