/* gtd-storage-goa.c
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtd-storage-goa.h"
#include "gtd-storage.h"
#include "gtd-task-list.h"

#include <glib/gi18n.h>

struct _GtdStorageGoa
{
  GtdStorage          parent;

  GoaObject          *goa_object;
  GIcon              *icon;
  gchar              *parent_source;
};

G_DEFINE_TYPE (GtdStorageGoa, gtd_storage_goa, GTD_TYPE_STORAGE)

enum {
  PROP_0,
  PROP_GOA_OBJECT,
  PROP_PARENT,
  LAST_PROP
};

static GIcon*
gtd_storage_goa_get_icon (GtdStorage *storage)
{
  g_return_val_if_fail (GTD_IS_STORAGE_GOA (storage), NULL);

  return GTD_STORAGE_GOA (storage)->icon;
}

static GtdTaskList*
gtd_storage_goa_create_list (GtdStorage  *storage,
                             const gchar *name)
{
  ESourceExtension *extension;
  GtdStorageGoa *self;
  GoaCalendar *calendar;
  GtdTaskList *task_list;
  GoaAccount *account;
  ESource *source;
  GError *error;

  g_return_val_if_fail (GTD_IS_STORAGE_GOA (storage), NULL);

  self = GTD_STORAGE_GOA (storage);
  error = NULL;
  account = goa_object_peek_account (self->goa_object);
  calendar = goa_object_peek_calendar (self->goa_object);
  source = e_source_new (NULL,
                         NULL,
                         &error);

  if (error)
    {
      g_warning ("%s: %s: %s",
                 G_STRFUNC,
                 _("Error creating new task list"),
                 error->message);

      g_clear_error (&error);
      return NULL;
    }

  /* Some properties */
  e_source_set_display_name (source, name);

  /* FIXME: while Evolution-Data-Server doesn't support creating remote
   * sources that are children of the GOA source, we'll have to make it
   * a plain webdav source.
   */
  e_source_set_parent (source, "webdav-stub");

  /* Mark it as a TASKLIST */
  extension = e_source_get_extension (source, E_SOURCE_EXTENSION_TASK_LIST);
  e_source_backend_set_backend_name (E_SOURCE_BACKEND (extension), "webdav");

  /* Make it a WebDAV source */
  extension = e_source_get_extension (source, E_SOURCE_EXTENSION_WEBDAV_BACKEND);
  e_source_webdav_set_display_name (E_SOURCE_WEBDAV (extension), gtd_storage_get_name (storage));

  /* Setup authentication */
  extension = e_source_get_extension (source, E_SOURCE_EXTENSION_AUTHENTICATION);
  e_source_authentication_set_user (E_SOURCE_AUTHENTICATION (extension), goa_account_get_identity (account));
  e_source_authentication_set_host (E_SOURCE_AUTHENTICATION (extension), goa_calendar_get_uri (calendar));

  /* Create task list */
  task_list = gtd_task_list_new (source, gtd_storage_get_name (storage));

  return task_list;
}

static gint
gtd_storage_goa_compare (GtdStorage *a,
                         GtdStorage *b)
{
  GoaAccount *account_a;
  GoaAccount *account_b;
  gint retval;

  /*
   * If any of the compared storages is not a GOA storage, we have
   * no means to properly compare them. Simply return 0 and let the
   * parent class' call decide what to do.
   */
  if (!GTD_IS_STORAGE_GOA (a) || !GTD_IS_STORAGE_GOA (b))
    return 0;

  account_a = gtd_storage_goa_get_account (GTD_STORAGE_GOA (a));
  account_b = gtd_storage_goa_get_account (GTD_STORAGE_GOA (b));

  retval = g_strcmp0 (goa_account_get_provider_type (account_b), goa_account_get_provider_type (account_a));

  if (retval != 0)
    return retval;

  retval = g_strcmp0 (gtd_storage_get_id (b), gtd_storage_get_id (a));

  return retval;
}

static void
gtd_storage_goa_finalize (GObject *object)
{
  GtdStorageGoa *self = (GtdStorageGoa *)object;

  g_clear_object (&self->goa_object);
  g_clear_object (&self->icon);
  g_clear_pointer (&self->parent_source, g_free);

  G_OBJECT_CLASS (gtd_storage_goa_parent_class)->finalize (object);
}

static void
gtd_storage_goa_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GtdStorageGoa *self = GTD_STORAGE_GOA (object);

  switch (prop_id)
    {
    case PROP_GOA_OBJECT:
      g_value_set_object (value, self->goa_object);
      break;

    case PROP_PARENT:
      g_value_set_string (value, gtd_storage_goa_get_parent (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_storage_goa_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GtdStorageGoa *self = GTD_STORAGE_GOA (object);

  switch (prop_id)
    {
    case PROP_GOA_OBJECT:
      gtd_storage_goa_set_object (self, g_value_get_object (value));
      break;

    case PROP_PARENT:
      gtd_storage_goa_set_parent (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_storage_goa_class_init (GtdStorageGoaClass *klass)
{
  GtdStorageClass *storage_class = GTD_STORAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtd_storage_goa_finalize;
  object_class->get_property = gtd_storage_goa_get_property;
  object_class->set_property = gtd_storage_goa_set_property;

  storage_class->get_icon = gtd_storage_goa_get_icon;
  storage_class->compare = gtd_storage_goa_compare;
  storage_class->create_list = gtd_storage_goa_create_list;

  /**
   * GtdStorageGoa::account:
   *
   * The #GoaAccount of this storage.
   */
  g_object_class_install_property (
        object_class,
        PROP_GOA_OBJECT,
        g_param_spec_object ("goa-object",
                             _("GoaObject of the storage"),
                             _("The GoaObject this storage location represents."),
                             GOA_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * GtdStorageGoa::parent:
   *
   * The parent source of this storage.
   */
  g_object_class_install_property (
        object_class,
        PROP_PARENT,
        g_param_spec_string ("parent",
                             _("Parent of the storage"),
                             _("The parent source identifier of the storage location."),
                             NULL,
                             G_PARAM_READWRITE));
}

static void
gtd_storage_goa_init (GtdStorageGoa *self)
{
}

/**
 * gtd_storage_goa_new:
 * @object: the #GoaObject related to this storage
 *
 * Creates a new #GtdStorageGoa.
 *
 * Returns: (transfer full): a newly allocated #GtdStorageGoa
 */
GtdStorage*
gtd_storage_goa_new (GoaObject *object)
{
  return g_object_new (GTD_TYPE_STORAGE_GOA,
                           "goa-object", object,
                           NULL);;
}

/**
 * gtd_storage_goa_get_account:
 * @goa_storage: a #GtdStorageGoa
 *
 * Retrieves the #GoaAccount of @goa_storage.
 *
 * Returns: (transfer none): a #GoaAccount.
 */
GoaAccount*
gtd_storage_goa_get_account (GtdStorageGoa *goa_storage)
{
  g_return_val_if_fail (GTD_IS_STORAGE_GOA (goa_storage), NULL);

  return goa_storage->goa_object ? goa_object_peek_account (goa_storage->goa_object) : NULL;
}

/**
 * gtd_storage_goa_get_object:
 * @goa_storage: a #GtdStorageGoa
 *
 * Retrieves the internal #GoaObject of @goa_storage.
 *
 * Returns: (transger none): a #GoaObject
 */
GoaObject*
gtd_storage_goa_get_object (GtdStorageGoa *goa_storage)
{
  g_return_val_if_fail (GTD_IS_STORAGE_GOA (goa_storage), NULL);

  return goa_storage->goa_object;
}

/**
 * gtd_storage_goa_set_object:
 * @goa_storage: a #GtdStorageGoa
 * @object: a #GoaObject
 *
 * Sets the #GoaObject of @goa_storage.
 *
 * Returns:
 */
void
gtd_storage_goa_set_object (GtdStorageGoa *goa_storage,
                            GoaObject     *object)
{
  g_return_if_fail (GTD_IS_STORAGE_GOA (goa_storage));

  if (goa_storage->goa_object != object)
    {
      g_set_object (&goa_storage->goa_object, object);
      g_object_notify (G_OBJECT (goa_storage), "goa-object");

      if (goa_storage->goa_object)
        {
          GoaAccount *account;
          gchar *icon_name;

          /* Setup account */
          account = goa_object_peek_account (goa_storage->goa_object);
          icon_name = g_strdup_printf ("goa-account-%s", goa_account_get_provider_type (account));

          gtd_storage_set_id (GTD_STORAGE (goa_storage), goa_account_get_id (account));
          gtd_storage_set_name (GTD_STORAGE (goa_storage), goa_account_get_identity (account));
          gtd_storage_set_provider (GTD_STORAGE (goa_storage), goa_account_get_provider_name (account));

          g_set_object (&goa_storage->icon, g_themed_icon_new (icon_name));
          g_object_notify (G_OBJECT (goa_storage), "icon");

          g_free (icon_name);
        }
    }
}

/**
 * gtd_storage_goa_get_parent:
 * @goa_storage: a #GtdStorageGoa
 *
 * Retrieve @goa_storage's parent #ESource UID.
 *
 * Returns: (transfer none): the parent #ESource's UID
 */
const gchar*
gtd_storage_goa_get_parent (GtdStorageGoa *goa_storage)
{
  g_return_val_if_fail (GTD_IS_STORAGE_GOA (goa_storage), NULL);

  return goa_storage->parent_source;
}

/**
 * gtd_storage_goa_set_parent:
 * @goa_storage: a #GtdStorageGoa
 * @parent: the parent #ESource UID of @goa_storage
 *
 * Sets the parent #ESource UID of @goa_storage.
 *
 * Returns:
 */
void
gtd_storage_goa_set_parent (GtdStorageGoa *goa_storage,
                            const gchar   *parent)
{
  g_return_if_fail (GTD_IS_STORAGE_GOA (goa_storage));

  if (g_strcmp0 (goa_storage->parent_source, parent) != 0)
    {
      g_clear_pointer (&goa_storage->parent_source, g_free);

      goa_storage->parent_source = g_strdup (parent);

      g_object_notify (G_OBJECT (goa_storage), "parent");
    }
}