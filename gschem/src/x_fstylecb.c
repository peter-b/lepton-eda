/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2010 gEDA Contributors (see ChangeLog for details)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
/*! \file x_fstylecb.c
 *
 *  \brief A GtkComboBox for gschem fill styles.
 */
#include <config.h>
#include <version.h>
#include <missing.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif




/*! \brief The columns in the GtkListStore
 */
enum
{
    COLUMN_NAME,
    COLUMN_INDEX,
    COLUMN_USE_WIDTH,
    COLUMN_USE1,
    COLUMN_USE2,
    COLUMN_COUNT
};



/*! \brief Stores the list of fill styles for use in GtkComboBox
 */
static GtkListStore* fstyle_list_store = NULL;



static GtkListStore* create_fstyle_list_store();



/*! \brief Create the GtkListStore for fill styles.
 */
static GtkListStore*
create_fstyle_list_store ()
{
  GtkTreeIter iter;
  GtkListStore *store;

  store = gtk_list_store_new (COLUMN_COUNT,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Hollow"),
    COLUMN_INDEX,     FILLING_HOLLOW,
    COLUMN_USE_WIDTH, FALSE,
    COLUMN_USE1,      FALSE,
    COLUMN_USE2,      FALSE,
    -1
    );

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Filled"),
    COLUMN_INDEX,     FILLING_FILL,
    COLUMN_USE_WIDTH, FALSE,
    COLUMN_USE1,      FALSE,
    COLUMN_USE2,      FALSE,
    -1
    );

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Mesh"),
    COLUMN_INDEX,     FILLING_MESH,
    COLUMN_USE_WIDTH, TRUE,
    COLUMN_USE1,      TRUE,
    COLUMN_USE2,      TRUE,
    -1
    );

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Hatch"),
    COLUMN_INDEX,     FILLING_HATCH,
    COLUMN_USE_WIDTH, TRUE,
    COLUMN_USE1,      TRUE,
    COLUMN_USE2,      FALSE,
    -1
    );

  return store;
}



/*! \brief Create a ComboBox with the gschem fill styles.
 *
 *  \return GtkWidget
 */
GtkWidget*
x_fstylecb_new ()
{
  GtkComboBox *combo;
  GtkCellLayout *layout;
  GtkCellRenderer *text_cell;

  if (fstyle_list_store == NULL) {
    fstyle_list_store = create_fstyle_list_store ();
  }

  combo = GTK_COMBO_BOX (gtk_combo_box_new_with_model (GTK_TREE_MODEL (fstyle_list_store)));
  layout = GTK_CELL_LAYOUT (combo); /* For convenience */

  /* Renders the name of the fill style */
  text_cell = GTK_CELL_RENDERER (gtk_cell_renderer_text_new());
  g_object_set (text_cell, "xpad", 5, NULL);
  gtk_cell_layout_pack_start (layout, text_cell, TRUE);
  gtk_cell_layout_add_attribute (layout, text_cell, "text", COLUMN_NAME);

  return GTK_WIDGET (combo);
}



/*! \brief Get the currently selected fill style index
 *
 *  \param [in,out] widget The fill style combo box
 *  \return The currently selected fill style index
 */
int
x_fstylecb_get_index (GtkWidget *widget)
{
  int index = -1;
  GtkTreeIter iter;
  GValue value = {0};

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
    gtk_tree_model_get_value (GTK_TREE_MODEL (fstyle_list_store), &iter, COLUMN_INDEX, &value);
    index = g_value_get_int (&value);
    g_value_unset (&value);
  }

  return index;
}



/*! \brief The currently selected fill style uses line width
 *
 *  \param [in,out] widget The fill style combo box
 *  \return The currently selected fill style uses line width
 */
gboolean
x_fstylecb_get_use_width (GtkWidget *widget)
{
  gboolean use_width = FALSE;
  GtkTreeIter iter;
  GValue value = {0};

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
    gtk_tree_model_get_value (GTK_TREE_MODEL (fstyle_list_store), &iter, COLUMN_USE_WIDTH, &value);
    use_width = g_value_get_boolean (&value);
    g_value_unset (&value);
  }

  return use_width;
}



/*! \brief The currently selected fill style uses angle1, pitch1
 *
 *  \param [in,out] widget The fill style combo box
 *  \return The currently selected fill style uses angle1, pitch1
 */
gboolean
x_fstylecb_get_use1 (GtkWidget *widget)
{
  gboolean use1 = FALSE;
  GtkTreeIter iter;
  GValue value = {0};

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
    gtk_tree_model_get_value (GTK_TREE_MODEL (fstyle_list_store), &iter, COLUMN_USE1, &value);
    use1 = g_value_get_boolean (&value);
    g_value_unset (&value);
  }

  return use1;
}



/*! \brief The currently selected fill style uses angle2, pitch2
 *
 *  \param [in,out] widget The fill style combo box
 *  \return The currently selected fill style uses angle2, pitch2
 */
gboolean
x_fstylecb_get_use2 (GtkWidget *widget)
{
  gboolean use2 = FALSE;
  GtkTreeIter iter;
  GValue value = {0};

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
    gtk_tree_model_get_value (GTK_TREE_MODEL (fstyle_list_store), &iter, COLUMN_USE2, &value);
    use2 = g_value_get_boolean (&value);
    g_value_unset (&value);
  }

  return use2;
}



/*! \brief Select the given fill style index
 *
 *  \param [in,out] widget The fill style combo box
 *  \param [in]     style  The fill style index to select
 */
void
x_fstylecb_set_index (GtkWidget *widget, int style)
{
  GtkTreeIter iter;
  gboolean success;
  GValue value = {0};

  g_return_if_fail (fstyle_list_store != NULL);

  success = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (fstyle_list_store), &iter);
  while (success) {
    gtk_tree_model_get_value (GTK_TREE_MODEL (fstyle_list_store), &iter, COLUMN_INDEX, &value);
    if (g_value_get_int (&value) == style) {
      g_value_unset (&value);
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX(widget), &iter);
      break;
    }
    g_value_unset (&value);
    success = gtk_tree_model_iter_next (GTK_TREE_MODEL(fstyle_list_store), &iter);
  }
}