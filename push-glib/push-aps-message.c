/* push-aps-message.c
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

#include "push-aps-message.h"
#include "push-debug.h"

G_DEFINE_TYPE(PushApsMessage, push_aps_message, G_TYPE_OBJECT)

struct _PushApsMessagePrivate
{
   GHashTable *extra;
   gchar *alert;
   guint badge;
   gchar *sound;
};

enum
{
   PROP_0,
   PROP_ALERT,
   PROP_BADGE,
   PROP_SOUND,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

void
push_aps_message_add_extra (PushApsMessage *message,
                            const gchar    *key,
                            JsonNode       *value)
{
   PushApsMessagePrivate *priv;

   ENTRY;

   g_return_if_fail(PUSH_IS_APS_MESSAGE(message));
   g_return_if_fail(key);
   g_return_if_fail(value);

   priv = message->priv;

   if (!priv->extra) {
      priv->extra =
         g_hash_table_new_full(g_str_hash, g_str_equal,
                               g_free, (GDestroyNotify)json_node_free);
   }

   g_hash_table_insert(priv->extra, g_strdup(key), json_node_copy(value));

   EXIT;
}

const gchar *
push_aps_message_get_alert (PushApsMessage *message)
{
   g_return_val_if_fail(PUSH_IS_APS_MESSAGE(message), NULL);
   return message->priv->alert;
}

guint
push_aps_message_get_badge (PushApsMessage *message)
{
   g_return_val_if_fail(PUSH_IS_APS_MESSAGE(message), 0);
   return message->priv->badge;
}

const gchar *
push_aps_message_get_sound (PushApsMessage *message)
{
   g_return_val_if_fail(PUSH_IS_APS_MESSAGE(message), NULL);
   return message->priv->sound;
}

void
push_aps_message_set_alert (PushApsMessage *message,
                            const gchar    *alert)
{
   g_return_if_fail(PUSH_IS_APS_MESSAGE(message));
   g_free(message->priv->alert);
   message->priv->alert = g_strdup(alert);
   g_object_notify_by_pspec(G_OBJECT(message), gParamSpecs[PROP_ALERT]);
}

void
push_aps_message_set_badge (PushApsMessage *message,
                            guint           badge)
{
   g_return_if_fail(PUSH_IS_APS_MESSAGE(message));
   message->priv->badge = badge;
   g_object_notify_by_pspec(G_OBJECT(message), gParamSpecs[PROP_BADGE]);
}

void
push_aps_message_set_sound (PushApsMessage *message,
                            const gchar    *sound)
{
   g_return_if_fail(PUSH_IS_APS_MESSAGE(message));
   g_free(message->priv->sound);
   message->priv->sound = g_strdup(sound);
   g_object_notify_by_pspec(G_OBJECT(message), gParamSpecs[PROP_SOUND]);
}

static void
push_aps_message_finalize (GObject *object)
{
   PushApsMessagePrivate *priv;

   ENTRY;

   priv = PUSH_APS_MESSAGE(object)->priv;

   g_free(priv->alert);
   priv->alert = NULL;

   g_free(priv->sound);
   priv->sound = NULL;

   if (priv->extra) {
      g_hash_table_unref(priv->extra);
      priv->extra = NULL;
   }

   G_OBJECT_CLASS(push_aps_message_parent_class)->finalize(object);

   EXIT;
}

static void
push_aps_message_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
   PushApsMessage *message = PUSH_APS_MESSAGE(object);

   switch (prop_id) {
   case PROP_ALERT:
      g_value_set_string(value, push_aps_message_get_alert(message));
      break;
   case PROP_BADGE:
      g_value_set_uint(value, push_aps_message_get_badge(message));
      break;
   case PROP_SOUND:
      g_value_set_string(value, push_aps_message_get_sound(message));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_aps_message_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
   PushApsMessage *message = PUSH_APS_MESSAGE(object);

   switch (prop_id) {
   case PROP_ALERT:
      push_aps_message_set_alert(message, g_value_get_string(value));
      break;
   case PROP_BADGE:
      push_aps_message_set_badge(message, g_value_get_uint(value));
      break;
   case PROP_SOUND:
      push_aps_message_set_sound(message, g_value_get_string(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_aps_message_class_init (PushApsMessageClass *klass)
{
   GObjectClass *object_class;

   ENTRY;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = push_aps_message_finalize;
   object_class->get_property = push_aps_message_get_property;
   object_class->set_property = push_aps_message_set_property;
   g_type_class_add_private(object_class, sizeof(PushApsMessagePrivate));

   gParamSpecs[PROP_ALERT] =
      g_param_spec_string("alert",
                          _("Alert"),
                          _("The alert text to display."),
                          NULL,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_ALERT,
                                   gParamSpecs[PROP_ALERT]);

   gParamSpecs[PROP_BADGE] =
      g_param_spec_uint("badge",
                        _("Badge"),
                        _("The badget number or zero to unset."),
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_BADGE,
                                   gParamSpecs[PROP_BADGE]);

   gParamSpecs[PROP_SOUND] =
      g_param_spec_string("sound",
                          _("Sound"),
                          _("The sound to play."),
                          NULL,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_SOUND,
                                   gParamSpecs[PROP_SOUND]);

   EXIT;
}

static void
push_aps_message_init (PushApsMessage *message)
{
   message->priv = G_TYPE_INSTANCE_GET_PRIVATE(message,
                                               PUSH_TYPE_APS_MESSAGE,
                                               PushApsMessagePrivate);
}
