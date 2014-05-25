/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file vehicle_gui_base.h Functions/classes shared between the different vehicle list GUIs. */

#ifndef VEHICLE_GUI_BASE_H
#define VEHICLE_GUI_BASE_H

#include "sortlist_type.h"
#include "vehiclelist.h"
#include "group.h"
#include "window_gui.h"
#include "widgets/dropdown_type.h"

enum ShowingList {
	VLS_VEHICLES,
	VLS_GROUPS,
};

struct GroupAndIndent {
	const Group* g;
	int indent;
	bool active;
	bool has_active_descendant;

	GroupAndIndent(Group *g, bool active);
};

typedef GUIList<const Vehicle*> GUIVehicleList;
typedef GUIList<GroupAndIndent> GUIGroupList;

struct BaseVehicleListWindow : public Window {
	GUIVehicleList vehicles;  ///< The list of vehicles
	GUIGroupList groups;      ///< List of groups
	ShowingList show;         ///< Which list to show (except for groups window)
	Listing *sorting;         ///< Pointer to the vehicle type related sorting.
	byte unitnumber_digits;   ///< The number of digits of the highest unit number
	Scrollbar *vscroll;
	VehicleListIdentifier vli; ///< Identifier of the vehicle list we want to currently show.

	enum ActionDropdownItem {
		ADI_REPLACE,
		ADI_SERVICE,
		ADI_DEPOT,
		ADI_ADD_SHARED,
		ADI_REMOVE_ALL,
		ADI_DEFAULT_GROUP_NAME,
	};

	enum ActionDropdownItemGroups {
		ADIG_BEGIN = 0,
		ADIG_GROUP_MANAGE_BUILD_ORDERS_SIMPLE = ADIG_BEGIN,
		ADIG_GROUP_MANAGE_BUILD_ORDERS_STATIONS,
		ADIG_GROUP_MANAGE_BUILD_CARGO,
		ADIG_GROUP_MANAGE_BUILD_1ST_ENGINE_CLASS_CARGO,
		ADIG_GROUP_MANAGE_BUILD_1ST_ENGINE,
		ADIG_GROUP_MANAGE_BUILD_CARGO_ORDERS,
		ADIG_GROUP_MANAGE_DEFAULT_NAMES,
		ADIG_BEGIN_ONLY_LOCAL,
		ADIG_UPDATE_CARGO = ADIG_BEGIN_ONLY_LOCAL,

		ADIG_END
	};

	static const StringID vehicle_depot_name[];

	static const StringID vehicle_sorter_names[];
	static GUIVehicleList::SortFunction * const vehicle_sorter_funcs[];
	static const StringID group_sorter_names[];
	static GUIGroupList::SortFunction * const group_sorter_funcs[];

	BaseVehicleListWindow(WindowDesc *desc, WindowNumber wno) : Window(desc), vli(wno)
	{
		this->vehicles.SetSortFuncs(this->vehicle_sorter_funcs);
	}

	void DrawVehicleListItems(VehicleID selected_vehicle, int line_height, const Rect &r) const;
	void DrawGroupListItems(int line_height, const Rect &r) const;
	void SortVehicleList();
	void SortGroupList();
	bool BuildGroupList(bool group_window = 0);
	void BuildVehicleList();
	Dimension GetActionDropdownSize(bool show_autoreplace, bool show_group);
	DropDownList *BuildActionDropdownList(const bool show_autoreplace, const bool groups = false) const;
};

uint GetVehicleListHeight(VehicleType type, uint divisor = 1);

struct Sorting {
	Listing aircraft;
	Listing roadveh;
	Listing ship;
	Listing train;
};

extern Sorting _sorting;

#endif /* VEHICLE_GUI_BASE_H */
