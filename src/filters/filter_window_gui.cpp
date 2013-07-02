/* $Id: filter_window_gui.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filter_window_gui.cpp The base for filter windows and some other related functions. */

#include "../stdafx.h"
#include "../window_func.h"
#include "../station_gui.h"
#include "../viewport_func.h"
#include "../vehicle_base.h"
#include "../vehicle_gui_base.h"
#include "../company_base.h"
#include "../zoning.h"
#include "filter_lists.h"

#include "../widgets/filter_widget.h"

#include "table/strings.h"

enum FilterWindowTabs{
	FWT_BEGIN = 0,
	FWT_TOWNS = FWT_BEGIN,
	FWT_COMPANIES,
	FWT_CARGO,                         ///< Used as cargo production on some windows.
	FWT_CARGO_ACCEPTANCE,
	FWT_STATION_FACILITIES,
	FWT_STATIONS,
	FWT_ORDERLIST,
	FWT_VEHICLE_GROUP_PROPERTIES,
	FWT_CATCHMENT_AREA_PROPERTIES,
	FWT_TOWN_PROPERTIES,

	FWT_STATE,
	FWT_END,
};

/** Base class for managing the filter windows. */
struct FilterWindowBase : Window, FilterLists {
	const uint mask;       ///< Mask indicating which lists are to be considered (stations, towns, etc.)
	const uint tiny_step;  ///< Step height for the elements list.
	Scrollbar *vscroll;    ///< The scrollbar.
	FilterWindowTabs tab;  ///< Now showing this list.
	uint state_lines;      ///< Number of elements of the current list.

	/**
	 * On new filterlists, set where the active filter is; if null, create a new active filter.
	 * @param active If not null, set passed FilterActive as the active elements.
	 */
	FilterWindowBase(WindowDesc *desc, uint mask, FilterActive *active) : Window(desc), mask(mask), tiny_step(FONT_HEIGHT_NORMAL + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM)
	{
		if (active == NULL) {
			this->active = new FilterActive();
		} else {
			this->active = active;
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		if (widget == FW_LIST) {
			resize->height = this->tiny_step;
			size->height = 5 * this->tiny_step;
		}
	}

	/**
	 * We should hide stations when:
	 * 	1) catchment window
	 * 	2) and on active elements list
	 * 	3) and no special selection of stations is made.
	 * @return true if we should hide stations on active list.
	 */
	inline bool HideStationsForCatchment() const {
		return this->window_class == WC_CATCHMENT_AREA_WINDOW && this->tab == FWT_STATE && !_stations_modified;
	}

	/**
	 * Draw a list of FilterElements
	 * @param list_type list type of items to draw
	 * @param r is the rectangle where to draw
	 * @param y is the height where to draw
	 * @param n is the number of already drawn items
	 * @param initial_list_pos the initial item to draw (default is 0)
	 * @param only_active if we are drawing the state of a filter (the items that are active)
	 * @param skip_lines the number of lines to skip (used when drawing the state of a filter)
	 */
	void DrawList(const ListType list_type, const Rect &r, int &y, uint &n, uint initial_list_pos, bool only_active, uint &skip_lines) const
	{
		const bool rtl = _current_text_dir == TD_RTL;
		const int sqr_width = FONT_HEIGHT_NORMAL - 2;
		const int left = r.left + WD_FRAMERECT_LEFT;
		const int right = r.right - WD_FRAMERECT_RIGHT;
		const int left_rect = rtl ? right - sqr_width : left;
		const int right_rect = rtl ? right : left + sqr_width;
		const int margin_left = rtl ? left : left + sqr_width + 4;
		const int margin_right = rtl ? right - sqr_width - 4 : right;
		const GUIListForFilter &list = only_active ? this->active->lists[list_type] : this->lists[list_type];
		int x;

		if (list_type == LT_STATIONS && this->HideStationsForCatchment()) return;

		for (uint i = initial_list_pos; i < list.Length() && n < this->vscroll->GetCapacity(); i++) {
			const FilterElement &element_is = list[i];
			if (skip_lines > 0) {
				skip_lines--;
				continue;
			}

			StringID str;

			switch (list_type) {
				case LT_TOWNS: {
					const Town *t = Town::GetIfValid(element_is.GetElement());
					str = STR_FILTER_TOWN;
					SetDParam(0, t->index);
					SetDParam(1, t->cache.population);
					break;
				}

				case LT_CARGO: {
					const CargoSpec *cs = CargoSpec::Get(element_is.GetElement());
					GfxFillRect(left_rect, y + 1, right_rect, y + sqr_width, cs->rating_colour);
					switch (this->window_class) {
						case WC_CATCHMENT_AREA_WINDOW:
						case WC_INDUSTRY_FILTER:
						case WC_STATION_FILTER:
							str = STR_FILTER_CARGO_PRODUCTION;
							SetDParam(0, cs->name);
							break;
						case WC_VEHICLE_GROUP_FILTER:
							str = cs->name;
							break;
						default: NOT_REACHED();
					}
					break;
				}

				case LT_CARGO_ACCEPTANCE: {
					const CargoSpec *cs = CargoSpec::Get(element_is.GetElement());
					GfxFillRect(left_rect, y + 1, right_rect, y + sqr_width, cs->rating_colour);
					str = STR_FILTER_CARGO_ACCEPTANCE;
					SetDParam(0, cs->name);
					break;
				}

				case LT_COMPANIES: {
					const Company *c = Company::GetIfValid(element_is.GetElement());
					DrawFrameRect(left_rect, y + 1, right_rect, y + sqr_width, (Colours)c->colour, element_is.GetFrameFlags());
					str = STR_COMPANY_NAME;
					SetDParam(0, c->index);
					break;
				}

				case LT_STATION_FACILITIES: {
					str = STR_FILTER_STATION_FACILITIES_TRAIN + element_is.GetElement();
					break;
				}

				case LT_STATIONS: {
					const Station *st = Station::GetIfValid(element_is.GetElement());
					if (st->owner < MAX_COMPANIES) DrawFrameRect(left_rect, y + 1, right_rect, y + sqr_width, (Colours)Company::GetIfValid(st->owner)->colour, element_is.GetFrameFlags());

					str = STR_FILTER_LIST_STATION;
					SetDParam(0, st->index);
					SetDParam(1, st->facilities);
					break;
				}

				case LT_ORDERLIST:
					str = STR_ORDERLIST_TYPE_INVALID + element_is.GetElement();
					break;

				case LT_VEHICLE_GROUP_PROPERTIES:
					str = STR_FILTER_PROPERTIES_STOPPED + element_is.GetElement();
					break;

				case LT_CATCHMENT_AREA_PROPERTIES:
					str = STR_FILTER_PROPERTIES_SHOW_ALL_UNCAUGHT_TILES + element_is.GetElement();
					break;

				case LT_TOWN_PROPERTIES:
					str = STR_FILTER_PROPERTIES_LOCAL_COMPANY_HAS_STATUE + element_is.GetElement();
					break;

				default: NOT_REACHED();
			}

			x = DrawString(margin_left, margin_right, y, str,  element_is.GetColourString());
			DrawString(left_rect, right_rect, y, element_is.GetMark(), element_is.GetColourString(), SA_CENTER);

			if (list_type == LT_STATIONS) {
				const Station *st = Station::GetIfValid(element_is.GetElement());
				StationsWndShowStationRating(st, left, right, x, FONT_HEIGHT_NORMAL + 2, y);
			}
			n++;
			y += this->tiny_step;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		if (widget != FW_LIST) return;

		uint lines_to_skip = 0;
		uint n = 0;                        // number of items shown
		int y = r.top + WD_FRAMERECT_TOP;  // vertical position for next item to be drawn

		switch (this->tab) {
			case FWT_TOWNS:
			case FWT_COMPANIES:
			case FWT_CARGO:
			case FWT_CARGO_ACCEPTANCE:
			case FWT_STATION_FACILITIES:
			case FWT_STATIONS:
			case FWT_ORDERLIST:
			case FWT_VEHICLE_GROUP_PROPERTIES:
			case FWT_CATCHMENT_AREA_PROPERTIES:
			case FWT_TOWN_PROPERTIES:
				this->DrawList((ListType)this->tab, r, y, n, this->vscroll->GetPosition(), false, lines_to_skip);
				break;

			case FWT_STATE:
				lines_to_skip = this->vscroll->GetPosition();
				for (uint i = LT_BEGIN; i < LT_END; i++) {
					this->DrawList((ListType)i, r, y, n, 0, true, lines_to_skip);
				}
				break;

			default: NOT_REACHED();
		}
	}


	/** Repaint vehicle details window. */
	virtual void OnPaint()
	{
		switch(this->tab){
			case FWT_TOWNS:
			case FWT_COMPANIES:
			case FWT_CARGO:
			case FWT_CARGO_ACCEPTANCE:
			case FWT_STATION_FACILITIES:
			case FWT_STATIONS:
			case FWT_ORDERLIST:
			case FWT_VEHICLE_GROUP_PROPERTIES:
			case FWT_CATCHMENT_AREA_PROPERTIES:
			case FWT_TOWN_PROPERTIES:
				this->vscroll->SetCount(this->lists[this->tab - FWT_BEGIN].Length());
				break;
			case FWT_STATE:
				this->vscroll->SetCount(state_lines);
				break;
			default: NOT_REACHED();
		}

		this->SetWidgetsDisabledState(this->window_class == WC_CATCHMENT_AREA_WINDOW && _stations_modified,
				FW_WIDGET_TOWNS,
				FW_WIDGET_COMPANIES,
				FW_WIDGET_CARGO,
				FW_WIDGET_CARGO_ACCEPTANCE,
				FW_WIDGET_STATION_FACILITIES,
				WIDGET_LIST_END);
		this->SetWidgetsDisabledState(false,
				FW_WIDGET_STATIONS,
				FW_WIDGET_ORDERS,
				FW_WIDGET_VEHICLE_GROUP_PROPERTIES,
				FW_WIDGET_CATCHMENT_AREA_PROPERTIES,
				FW_WIDGET_TOWN_PROPERTIES,
				FW_WIDGET_STATE,
				WIDGET_LIST_END);
		DisableWidget(FW_WIDGET_TOWNS + this->tab);

		this->DrawWidgets();
	}


	/**
	 * @todo admin, owner and sync
	 */
	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case FW_LIST: {
				uint id_v = this->vscroll->GetScrolledRowFromWidget(pt.y, this, FW_LIST);
				if (this->tab != FWT_STATE && id_v >= this->lists[this->tab].Length()) return;
				if (_ctrl_pressed) {
					switch (this->tab) {
						case FWT_TOWNS: {
							Town *t = Town::GetIfValid(this->lists[FWT_TOWNS][id_v].GetElement());
							if (t != NULL) ScrollMainWindowToTile(t->xy);
							break;
						}
						case FWT_STATIONS: {
							Station *st = Station::GetIfValid(this->lists[FWT_STATIONS][id_v].GetElement());
							ScrollMainWindowToTile(st->xy);
							break;
						}
						case FWT_STATE: {
							this->active->CTRLClickOnActive(id_v);
							break;
						}
						default: break;
					}
					return;
				}

				if (this->tab == FWT_STATE) return;
				this->FilterLists::OnClick((ListType)this->tab, this->lists[this->tab][id_v]);
				this->InvalidateFilterData((ListType)this->tab, id_v);
				this->SetDirty();
				break;
			}

			case FW_WIDGET_STATE:
				this->active->SortAll();
				state_lines = this->active->Count();
				this->tab = FWT_STATE;
				if (this->HideStationsForCatchment()) state_lines -= this->active->lists[LT_STATIONS].Length();
				this->SetDirty();
				break;

			case FW_WIDGET_TOWNS:
			case FW_WIDGET_COMPANIES:
			case FW_WIDGET_CARGO:
			case FW_WIDGET_CARGO_ACCEPTANCE:
			case FW_WIDGET_STATION_FACILITIES:
			case FW_WIDGET_STATIONS:
			case FW_WIDGET_ORDERS:
			case FW_WIDGET_VEHICLE_GROUP_PROPERTIES:
			case FW_WIDGET_CATCHMENT_AREA_PROPERTIES:
			case FW_WIDGET_TOWN_PROPERTIES:
				this->tab = (FilterWindowTabs)(widget - FW_WIDGET_TOWNS);
				this->SetDirty();
				break;

			case FW_WIDGET_RESET:
				this->Reset();
				if (this->tab == FWT_STATE) state_lines = this->active->Count();
				this->OnInvalidateData(4);
				this->SetDirty();
				break;

		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, FW_LIST);
		this->GetWidget<NWidgetCore>(FW_LIST)->widget_data = (this->vscroll->GetCapacity() << MAT_ROW_START) + (1 << MAT_COL_START);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data usually 0 for building, 2 for updating filter state, 4 for parent rebuilding
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;

		switch (data) {
			case 0:
				this->Build(this->mask);
				break;
			case 2:
				/* FALL THROUGH and rebuild list */
				/* This is the default update of filter,
				 * except for those filters that can be altered from outside the window
				 * (CTRL+Click a station -> exterior change of catchment area list) */
			case 4:
				/* On change of filter state, rebuild list of parent window */
				parent->InvalidateData(0);

				/* Mark the window we are updating */
				parent->SetWhiteBorder();
				parent->SetDirty();
				break;
		}

		this->SetDirty();
	}

	/**
	 * Virtual function to invalidate data of filters; each filterwindow has its own way to invalidate.
	 * @param list_type the type of element that is changed.
	 * @param id the id of the element changed.
	 */
	virtual void InvalidateFilterData(ListType list_type, uint id)
	{
		this->OnInvalidateData(4);
	}
};

/**
 * Find and return all opened filterlists that involve companies, stations...
 */
SmallVector<FilterLists*, 32> FindAllFilterLists(ListType type)
{
	SmallVector<FilterLists*, 32> lists;
	Window *w;
	FOR_ALL_WINDOWS_FROM_BACK(w) {
		if (w->window_class >= WC_VEHICLE_GROUP_FILTER && w->window_class <= WC_STATION_FILTER) {
			if (HasBit(((FilterWindowBase*)w)->mask, type)) *lists.Append() = (FilterWindowBase*)w;
		}
	}
	return lists;
}

/**
 * Update all filters that involve stations after a station given by id has been deleted or created
 * @note if created, add it after setting its name; if deleted, remove it from post_destructor associated
 */
void StationUpdateFilters(const StationID station)
{
	/* Find all FilterLists contained on windows that involve stations */
	SmallVector<FilterLists*, 32>  lists = FindAllFilterLists(LT_STATIONS);

	if (Station::IsValidID(station)) {
		/* Add the station */

		/* On _ca_controller */
		if (_ca_controller.FilterTestCA(Station::Get(station))) {
			*_ca_controller.lists[LT_STATIONS].Append() = FilterElement(station, IS_POSITIVE);
			UpdateCALayer(station);
		}

		/* On the opened windows */
		for (uint i = lists.Length(); i--;) {
			if (lists[i]->list_owner != INVALID_COMPANY && Station::Get(station)->owner != lists[i]->list_owner) continue;
			*lists[i]->lists[LT_STATIONS].Append() = FilterElement(station);
		}
	} else {
		/* Remove the station */

		/* Remove from catchment area controller */
		if (_ca_controller.lists[LT_STATIONS].FindAndErase(FilterElement(station))) {
			if (_stations_modified && _ca_controller.lists[LT_STATIONS].Length() == 0) _stations_modified = false;
		}

		/* Remove from the opened windows */
		for (uint i = lists.Length(); i--;) {
			/* Erase from list and its active part */
			lists[i]->lists[LT_STATIONS].FindAndErase(FilterElement(station));
			lists[i]->active->lists[LT_STATIONS].FindAndErase(FilterElement(station));
		}
	}

	for (uint i = lists.Length(); i--;) lists[i]->Sort(LT_STATIONS);

	InvalidateWindowClassesData(WC_CATCHMENT_AREA_WINDOW, 2);
	InvalidateWindowClassesData(WC_VEHICLE_GROUP_FILTER, 2);
}

/**
 * Update all filters that involve a company after this is created or deleted
 * @note if created, add it after setting its name; if deleted, remove it from post_destructor associated
 * @see StationUpdateFilters
 */
void CompanyUpdateFilters(const CompanyID company)
{
	SmallVector<FilterLists*, 32>  lists = FindAllFilterLists(LT_COMPANIES);

	if (Company::IsValidID(company)) {
		for (uint i = lists.Length(); i--;) {
			*lists[i]->lists[LT_COMPANIES].Append() = FilterElement(company);
		}
	} else {
		for (uint i = lists.Length(); i--;) {
			lists[i]->lists[LT_COMPANIES].FindAndErase(FilterElement(company));
			lists[i]->active->lists[LT_COMPANIES].FindAndErase(FilterElement(company));
		}

		if (_ca_controller.lists[LT_COMPANIES].FindAndErase(FilterElement(company)) && !_stations_modified) {
			UpdateCALayer(LT_END, 0);
		}
	}

	for (uint i = lists.Length(); i--;) lists[i]->Sort(LT_COMPANIES);

	SetWindowDirty(WC_CATCHMENT_AREA_WINDOW, 0);
	InvalidateWindowClassesData(WC_TOWN_FILTER, 2);
}

/**
 * Update all filters that involve a town after this is created or deleted
 * @note if created, add it after setting its name; if deleted, remove it from post_destructor associated
 * @see StationUpdateFilters
 */
void TownUpdateFilters(const TownID town)
{
	SmallVector<FilterLists*, 32>  lists = FindAllFilterLists(LT_TOWNS);

	if (Town::IsValidID(town)) {
		for (uint i = lists.Length(); i--;) {
			*lists[i]->lists[LT_TOWNS].Append() = FilterElement(town);
		}
	} else {
		/* Towns cannot be deleted ingame; do not call TownUpdateFilters on those cases */
		/* Or write the code that should go here */
		NOT_REACHED();
	}

	for (uint i = lists.Length(); i--;) lists[i]->Sort(LT_TOWNS);

	SetWindowDirty(WC_CATCHMENT_AREA_WINDOW, 0);
	SetWindowClassesDirty(WC_INDUSTRY_FILTER);
	SetWindowClassesDirty(WC_STATION_FILTER);
	SetWindowClassesDirty(WC_VEHICLE_GROUP_FILTER);
}

#include "filterwindow_vehiclesgroups.cpp"
#include "filterwindow_industries.cpp"
#include "filterwindow_towns.cpp"
#include "filterwindow_stations.cpp"
#include "filterwindow_catchmentarea.cpp"

/**
 * Find the FilterActive associated to a list
 * @param window_class window class of the window that contains the filter
 * @param window_number window number of the window that contains the filter
 * @return the FilterActive or NULL if no associated filter is opened
 */
const FilterActive *FindFilterData(WindowClass window_class, WindowNumber window_number)
{
	const FilterWindowBase *filter_window = (FilterWindowBase*)FindWindowById(window_class, window_number);
	return filter_window == NULL ? NULL : filter_window->active;
}

/**
 * Open a new filter window
 * @param parent_window (NULL for catchment area)
 * @param window_number window number for the new window (usually same as parent)
 */
void ShowFilterWindow(Window *parent_window, WindowNumber new_window_number)
{
	if (parent_window == NULL) {
		switch (new_window_number) {
			case 0:
				AllocateWindowDescFront<CatchmentAreaWindow>(&_ca_layer_window, new_window_number);
				return;
			default: break;
		}
		NOT_REACHED();
	}

	Window *new_window;

	switch (parent_window->window_class) {
		case WC_TRAINS_LIST:
		case WC_ROADVEH_LIST:
		case WC_SHIPS_LIST:
		case WC_AIRCRAFT_LIST:
			new_window = AllocateWindowDescFront<VehicleGroupFilter>(&_nested_vehicle_group_filter_desc, new_window_number);
			break;
		case WC_INDUSTRY_DIRECTORY:
			new_window = AllocateWindowDescFront<IndustryFilter>(&_nested_industries_filter_desc, new_window_number);
			break;
		case WC_TOWN_DIRECTORY:
			new_window = AllocateWindowDescFront<TownFilter>(&_nested_towns_filter_desc, new_window_number);
			break;
		case WC_STATION_LIST:
			new_window = AllocateWindowDescFront<StationFilter>(&_nested_stations_filter_desc, new_window_number);
			break;
		default: NOT_REACHED();
	}

	if (new_window != NULL) new_window->parent = parent_window;
}