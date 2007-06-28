/*
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>
#include <string.h>
#include <libebook/e-contact.h>

/* Get a list of the specified attributes from a contact */
GList *
hito_vcard_get_named_attributes (EVCard *contact, const char *name)
{
  GList *attrs = NULL, *l;

  g_return_val_if_fail (E_IS_VCARD (contact), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (l = e_vcard_get_attributes (E_VCARD (contact)); l; l = l->next)
  {
    EVCardAttribute *attr;
    const char *n;

    attr = (EVCardAttribute *) l->data;
    n = e_vcard_attribute_get_name (attr);

    if (strcmp (n, name) == 0)
      attrs = g_list_prepend (attrs, attr);
  }

  return g_list_reverse (attrs);
}


/*
 * Convenience function to set the type parmeter of a vcard attribute
 *
 * attr: the attribute to modify
 * type: semicolan seperated list of values
 *
 */
void
hito_vcard_attribute_set_type (EVCardAttribute *attr, gchar *type)
{
  GList *params;
  EVCardAttributeParam *p = NULL;

  /* look for the TYPE parameter */
  for (params = e_vcard_attribute_get_params (attr); params;
      params = g_list_next (params))
  {
    if (!strcmp (e_vcard_attribute_param_get_name (params->data), "TYPE"))
    {
      p = params->data;
      break;
    }
  }

  /* if we didn't find the TYPE parameter, so create it now */
  if (p == NULL)
  {
    p = e_vcard_attribute_param_new ("TYPE");
    e_vcard_attribute_add_param (attr, p);
  }

  /* remove the current values */
  e_vcard_attribute_param_remove_values (p);

  gint i;
  gchar **values = g_strsplit (type, ";", -1);

  for (i = 0; (values[i]); i++)
  {
    e_vcard_attribute_param_add_value (p, values[i]);
  }

  g_strfreev (values);
}

/*
 * Convenience function to get the type property of a vcard attribute
 * Returns the 
 */
gchar*
hito_vcard_attribute_get_type (EVCardAttribute *attr)
{
  GList *list, *l;
  gchar *result = NULL;
  list = e_vcard_attribute_get_param (attr, "TYPE");

  for (l = list; l; l = g_list_next (l))
  {
    if (result)
    {
      gchar *old_result = result;
      result = g_strconcat (l->data, ";", old_result, NULL);
      g_free (old_result);
    }
    else
    {
      result = g_strdup (l->data);
    }
  }

  return result;
}

/* returns true b is a subset of a, where a and b are semi-colon seperated
 * lists
 */
gboolean
hito_vcard_attribute_compare_value_strings (gchar *a, gchar *b)
{
  gchar **alist, **blist;
  gboolean result = FALSE;
  int i, j;

  /* make sure a and b are not NULL */
  if (!(a && b))
    return FALSE;

  alist = g_strsplit (a, ";", -1);
  blist = g_strsplit (b, ";", -1);

  /* check each element of blist exists in alist */
  for (i = 0; blist[i]; i++)
  {
    gboolean exists = FALSE;
    for (j = 0; alist[j]; j++)
    {
      if (!strcmp (alist[j], blist[i]))
      {
        exists = TRUE;
        break;
      }
    }
    /* if any of the items from b don't exist in a, we return false */
    if (!exists)
    {
      result = FALSE;
      break;
    }
    else
    {
      result = TRUE;
    }
  }

  g_strfreev (alist);
  g_strfreev (blist);

  return result;
}

/*
 * load the attribute value, returning a newly allocated semicolon seperated
 * string for multivalue attributes
 */
gchar*
vcard_attribute_get_value_string (EVCardAttribute *attr)
{
  gchar *attr_value = NULL;
  GList *l;
  l = e_vcard_attribute_get_values (attr);
  if (l)
  {
    attr_value = g_strdup (l->data);

    while ((l = g_list_next (l)))
    {
      gchar *old = attr_value;
      attr_value = g_strdup_printf ("%s; %s", old, (gchar*) l->data);
      g_free (old);
    }
  }
  return attr_value;
}

/*
 * Strip empty attributes from a vcard
 * Returns: the number of attributes left on the card
 */
gint
hito_vcard_strip_empty_attributes (EVCard *card)
{
  GList *attrs, *values;
  gboolean remove;
  EVCardAttribute *attribute;
  gint count = 0;

  attrs = e_vcard_get_attributes (card);
  while (attrs)
  {
    count++;
    attribute = attrs->data;
    remove = TRUE;
    values = e_vcard_attribute_get_values (attrs->data);

    while (values)
    {
      if (g_utf8_strlen (values->data, -1) > 0)
      {
        remove = FALSE;
        break;
      }
      values = g_list_next (values);
    }

    attrs = g_list_next (attrs);
    if (remove)
    {
      e_vcard_remove_attribute (card, attribute);
      count--;
    }
  }
  return count;
}


