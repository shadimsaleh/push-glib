/* push-c2dm-message.c
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
#include <libsoup/soup.h>

#include "push-c2dm-message.h"

G_DEFINE_TYPE(PushC2dmMessage, push_c2dm_message, G_TYPE_OBJECT)

struct _PushC2dmMessagePrivate
{
   SoupMessage *message;
   gchar *collapse_key;
   gboolean delay_while_idle;
};

enum
{
   PROP_0,
   PROP_COLLAPSE_KEY,
   PROP_DELAY_WHILE_IDLE,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

/**
 * push_c2dm_message_new:
 *
 * Creates a new instance of #PushC2dmMessage.
 *
 * Returns: (transfer full): A newly allocated #PushC2dmMessage.
 */
PushC2dmMessage *
push_c2dm_message_new (void)
{
   return g_object_new(PUSH_TYPE_C2DM_MESSAGE, NULL);
}

/**
 * push_c2dm_message_add_param:
 * @message: (in): A #PushC2dmMessage.
 * @param: (in): The key for the parameter.
 * @value: (in): The value for @key.
 *
 * This adds an additional key/value pair to be delivered in the C2DM message.
 * This is sent as "data.<key>" to the C2DM service.
 */
void
push_c2dm_message_add_param (PushC2dmMessage *message,
                             const gchar     *param,
                             const gchar     *value)
{
   gchar *data_key;

   g_return_if_fail(PUSH_IS_C2DM_MESSAGE(message));
   g_return_if_fail(param);

   data_key = g_strdup_printf("data.%s", param);
   soup_message_headers_append(message->priv->message->request_headers,
                               data_key, value);
   g_free(data_key);
}

/**
 * push_c2dm_message_get_collapse_key:
 * @message: (in): A #PushC2dmMessage.
 *
 * Fetches the "collapse-key" property.
 *
 * Returns: A string which should not be modified or freed.
 */
const gchar *
push_c2dm_message_get_collapse_key (PushC2dmMessage *message)
{
   g_return_val_if_fail(PUSH_IS_C2DM_MESSAGE(message), NULL);
   return message->priv->collapse_key;
}

/**
 * push_c2dm_message_set_collapse_key:
 * @message: (in): A #PushC2dmMessage.
 * @collapse_key: (in): A string containing the collapse-key.
 *
 * Sets the "collapse-key" property to use for the message. This is used
 * by the C2DM service to collapse multiple messages into a single message
 * when communicating with a device. This could happen if the device was
 * offline or messages are sent too fast.
 */
void
push_c2dm_message_set_collapse_key (PushC2dmMessage *message,
                                    const gchar     *collapse_key)
{
   g_return_if_fail(PUSH_IS_C2DM_MESSAGE(message));
   g_free(message->priv->collapse_key);
   message->priv->collapse_key = g_strdup(collapse_key);
   g_object_notify_by_pspec(G_OBJECT(message),
                            gParamSpecs[PROP_COLLAPSE_KEY]);
}

/**
 * push_c2dm_message_get_delay_while_idle:
 * @message: (in): A #PushC2dmMessage.
 *
 * Fetches the "delay-while-idle" property.
 *
 * Returns: A #gboolean.
 */
gboolean
push_c2dm_message_get_delay_while_idle (PushC2dmMessage *message)
{
   g_return_val_if_fail(PUSH_IS_C2DM_MESSAGE(message), FALSE);
   return message->priv->delay_while_idle;
}

/**
 * push_c2dm_message_set_delay_while_idle:
 * @message: (in): A #PushC2dmMessage.
 * @delay_while_idle: (in): Indicates c2dm should delay if device is idle.
 *
 * Sets the "delay-while-idle" property. If this is %FALSE, then the
 * device should receive the message immediately, regardless of if it
 * is in an idle state. If this is true, the device will wait to wake
 * up to receive the message.
 */
void
push_c2dm_message_set_delay_while_idle (PushC2dmMessage *message,
                                        gboolean         delay_while_idle)
{
   g_return_if_fail(PUSH_IS_C2DM_MESSAGE(message));
   message->priv->delay_while_idle = delay_while_idle;
   g_object_notify_by_pspec(G_OBJECT(message),
                            gParamSpecs[PROP_DELAY_WHILE_IDLE]);
}

static void
push_c2dm_message_finalize (GObject *object)
{
   PushC2dmMessagePrivate *priv = PUSH_C2DM_MESSAGE(object)->priv;
   g_clear_object(&priv->message);
   g_free(priv->collapse_key);
   G_OBJECT_CLASS(push_c2dm_message_parent_class)->finalize(object);
}

static void
push_c2dm_message_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
   PushC2dmMessage *message = PUSH_C2DM_MESSAGE(object);

   switch (prop_id) {
   case PROP_COLLAPSE_KEY:
      g_value_set_string(value,
                         push_c2dm_message_get_collapse_key(message));
      break;
   case PROP_DELAY_WHILE_IDLE:
      g_value_set_boolean(value,
                          push_c2dm_message_get_delay_while_idle(message));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_c2dm_message_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
   PushC2dmMessage *message = PUSH_C2DM_MESSAGE(object);

   switch (prop_id) {
   case PROP_COLLAPSE_KEY:
      push_c2dm_message_set_collapse_key(message,
                                         g_value_get_string(value));
      break;
   case PROP_DELAY_WHILE_IDLE:
      push_c2dm_message_set_delay_while_idle(message,
                                             g_value_get_boolean(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
push_c2dm_message_class_init (PushC2dmMessageClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = push_c2dm_message_finalize;
   object_class->get_property = push_c2dm_message_get_property;
   object_class->set_property = push_c2dm_message_set_property;
   g_type_class_add_private(object_class, sizeof(PushC2dmMessagePrivate));

   /**
    * PushC2dmMessage:collapse-key:
    *
    * The "collapse-key" property represents a key that the C2DM service can
    * use to collapse multiple messages into a single message. This may happen
    * if a device is offline while multiple messages have been sent. It also
    * might also happen if you send messages faster than they deem
    * appropriate.
    */
   gParamSpecs[PROP_COLLAPSE_KEY] =
      g_param_spec_string("collapse-key",
                          _("Collapse Key"),
                          _("A key for the C2DM to collapse queued messages."),
                          NULL,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_COLLAPSE_KEY,
                                   gParamSpecs[PROP_COLLAPSE_KEY]);

   /**
    * PushC2dmMessage:delay-while-idle:
    *
    * The "delay-while-idle" property indicates that the message should NOT
    * be immediately delivered to a device if it is in an idle state. This
    * is generally a good thing to set to %TRUE so that you don't waste
    * power on the end devices for spurious wakeups.
    */
   gParamSpecs[PROP_DELAY_WHILE_IDLE] =
      g_param_spec_boolean("delay-while-idle",
                          _("Delay While Idle"),
                          _("If the message should be delivered while device "
                            "is in an idle state."),
                          FALSE,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_DELAY_WHILE_IDLE,
                                   gParamSpecs[PROP_DELAY_WHILE_IDLE]);
}

static void
push_c2dm_message_init (PushC2dmMessage *message)
{
   message->priv = G_TYPE_INSTANCE_GET_PRIVATE(message,
                                               PUSH_TYPE_C2DM_MESSAGE,
                                               PushC2dmMessagePrivate);
   message->priv->message =
      soup_message_new("POST",
                       "https://android.apis.google.com/c2dm/send");
}
