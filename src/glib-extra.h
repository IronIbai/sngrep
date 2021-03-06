/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file glib-extra.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for glib containers
 *
 */

#ifndef __SNGREP_GLIB_EXTRA_H
#define __SNGREP_GLIB_EXTRA_H

#include <glib.h>

#define g_ptr_array_len(array) (array->len)
#define g_ptr_array_empty(array) (array->len == 0)
#define g_ptr_array_first(array) g_ptr_array_index(array, 0)
#define g_ptr_array_last(array)  g_ptr_array_index(array, array->len-1)
#define g_ptr_array_set(array, index, item)  (array->pdata[index] = item)

/**
 * @brief Make a deep concat from one Double-Linked list to another
 *
 * @param dst Destination list. May already have values or be NULL
 * @param src Source list. May be empty or be NULL
 *
 * @return Destination list with source nodes appended
 */
GList *
g_list_concat_deep(GList *dst, GList *src);

GPtrArray *
g_ptr_array_copy(GPtrArray *origin);

GPtrArray *
g_ptr_array_copy_filtered(GPtrArray *origin, GEqualFunc filter_func, gpointer filter_data);

gint
g_ptr_array_data_index(GPtrArray *array, gconstpointer data);

gpointer
g_ptr_array_next(GPtrArray *array, gconstpointer data);

gpointer
g_ptr_array_prev(GPtrArray *array, gconstpointer data);

void
g_ptr_array_add_array(GPtrArray *array, GPtrArray *items);

void
g_ptr_array_remove_array(GPtrArray *array, GPtrArray *items);

void
g_ptr_array_remove_all(GPtrArray *array);

#if !GLIB_CHECK_VERSION(2, 54, 0)
gboolean
g_ptr_array_find(GPtrArray *haystack, gconstpointer needle, guint *index);

gboolean
g_ptr_array_find_with_equal_func(GPtrArray *haystack, gconstpointer needle, GEqualFunc equal_func, guint *index);
#endif

#endif //__SNGREP_GLIB_EXTRA_H
