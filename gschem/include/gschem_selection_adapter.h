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
/*!
 * \file gschem_selection_adapter.h
 *
 * \brief
 */

#define GSCHEM_TYPE_SELECTION_ADAPTER           (gschem_selection_adapter_get_type())
#define GSCHEM_SELECTION_ADAPTER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_SELECTION_ADAPTER, GschemSelectionAdapter))
#define GSCHEM_SELECTION_ADAPTER_CLASS(klasse)  (G_TYPE_CHECK_CLASS_CAST ((klasse), GSCHEM_TYPE_SELECTION_ADAPTER, GschemSelectionAdapterClass))
#define GSCHEM_IS_SELECTION_ADAPTER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_SELECTION_ADAPTER))
#define GSCHEM_SELECTION_ADAPTER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSCHEM_TYPE_SELECTION_ADAPTER, GschemSelectionAdapterClass))

typedef struct _GschemSelectionAdapterClass GschemSelectionAdapterClass;
typedef struct _GschemSelectionAdapter GschemSelectionAdapter;

struct _GschemSelectionAdapterClass
{
  GObjectClass parent_class;
};

struct _GschemSelectionAdapter
{
  GObject parent;

  SELECTION *selection;
  TOPLEVEL *toplevel;
};

GType
gschem_selection_get_type();

int
gschem_selection_adapter_get_cap_style (GschemSelectionAdapter *adapter);

int
gschem_selection_adapter_get_dash_length (GschemSelectionAdapter *adapter);

int
gschem_selection_adapter_get_dash_space (GschemSelectionAdapter *adapter);

int
gschem_selection_adapter_get_line_type (GschemSelectionAdapter *adapter);

int
gschem_selection_adapter_get_line_width (GschemSelectionAdapter *adapter);

SELECTION *
gschem_selection_adapter_get_selection (GschemSelectionAdapter *adapter);

TOPLEVEL*
gschem_selection_adapter_get_toplevel (GschemSelectionAdapter *adapter);

GschemSelectionAdapter*
gschem_selection_adapter_new ();

void
gschem_selection_adapter_set_cap_style (GschemSelectionAdapter *adapter, int cap_style);

void
gschem_selection_adapter_set_dash_length (GschemSelectionAdapter *adapter, int dash_length);

void
gschem_selection_adapter_set_dash_space (GschemSelectionAdapter *adapter, int dash_space);

void
gschem_selection_adapter_set_line_type (GschemSelectionAdapter *adapter, int line_type);

void
gschem_selection_adapter_set_line_width (GschemSelectionAdapter *adapter, int line_width);

void
gschem_selection_adapter_set_selection (GschemSelectionAdapter *adapter, SELECTION *selection);

void
gschem_selection_adapter_set_toplevel (GschemSelectionAdapter *adapter, TOPLEVEL *toplevel);