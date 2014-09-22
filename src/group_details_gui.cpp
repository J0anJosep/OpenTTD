/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file group_details_gui.cpp The group details gui. */

#include "stdafx.h"
#include "strings_func.h"
#include "widgets/dropdown_func.h"

#include "company_func.h"
#include "vehicle_base.h"
#include "vehicle_gui.h"
#include "vehiclelist.h"
#include "gui.h"
#include "autoreplace_gui.h"
#include "group.h"
#include "timetable.h"
#include "core/sort_func.hpp"

#include "widgets/group_widget.h"

typedef int CDECL SortersForOrderLists(const OrderList * const *a, const OrderList * const *b);

/** The tabs in the group info window */
enum GroupDetailsWindowTabs {
	GI_TAB_GENERAL = 0,     ///< Tab with number of vehicles, profit, lenghts, max velocity,...
	GI_TAB_TIMETABLE,       ///< Tab with timetable information
	GI_TAB_CARGO,           ///< Tab with cargo carried by the vehicles
};

assert_compile(GIW_WIDGET_DETAILS_GENERAL       == GIW_WIDGET_DETAILS_GENERAL + GI_TAB_GENERAL  );
assert_compile(GIW_WIDGET_DETAILS_TIMETABLE     == GIW_WIDGET_DETAILS_GENERAL + GI_TAB_TIMETABLE);
assert_compile(GIW_WIDGET_DETAILS_CARGO         == GIW_WIDGET_DETAILS_GENERAL + GI_TAB_CARGO    );


/**
 * Make a vertical row of buttons,
 * starting at widget #GIW_WIDGET_DETAILS_LAT_BEGIN till #GIW_WIDGET_DETAILS_LAT_END.
 * @param biggest_index Pointer to store biggest used widget number of the buttons.
 * @return Vertical row.
 */
static NWidgetBase *GroupDetailsLateralWidgets(int *biggest_index)
{
	NWidgetVertical *container = new NWidgetVertical();
	for (uint i = GIW_WIDGET_DETAILS_LAT_BEGIN; i < GIW_WIDGET_DETAILS_LAT_END; i++) {
		/* after cargo button and before list button, add an empty panel for resizing */
		if (i == GIW_WIDGET_DETAILS_LIST) {
			NWidgetBackground *panel = new NWidgetBackground(WWT_PANEL, COLOUR_GREY, - 1);
			panel->SetResize(1, 1);
			panel->SetFill(1, 1);
			container->Add(panel);
		}
		NWidgetLeaf *nwi = new NWidgetLeaf(WWT_PUSHTXTBTN, COLOUR_GREY, i, STR_GROUP_DETAILS_GENERAL + i - GIW_WIDGET_DETAILS_LAT_BEGIN, STR_GROUP_DETAILS_GENERAL_TOOLTIP + i - GIW_WIDGET_DETAILS_LAT_BEGIN);
		nwi->SetMinimalSize(50, 12);
		nwi->SetResize(0, 0);
		nwi->SetFill(1, 0);
		container->Add(nwi);
	}
	*biggest_index = max(*biggest_index, GIW_WIDGET_DETAILS_LAT_END - 1);
	return container;
}


/** Group details widgets. */
static const NWidgetPart _nested_group_details_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, GIW_WIDGET_CAPTION), SetDataTip(STR_VEHICLE_DETAILS_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),

	NWidget(NWID_HORIZONTAL),
		NWidgetFunction(GroupDetailsLateralWidgets),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_PANEL, COLOUR_GREY, GIW_WIDGET_DETAILS), SetResize(1, 0), SetFill(1, 0), EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_MATRIX, COLOUR_GREY, GIW_WIDGET_LIST), SetMatrixDataTip(1, 0, 0), SetFill(1, 0), SetResize(1, 0), SetScrollbar(GIW_WIDGET_LIST_SCROLLBAR),
				NWidget(NWID_VSCROLLBAR, COLOUR_GREY, GIW_WIDGET_LIST_SCROLLBAR),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PANEL, COLOUR_GREY), SetFill(1, 1), SetResize(1,1), EndContainer(),
				NWidget(WWT_DROPDOWN, COLOUR_GREY, GIW_WIDGET_ORDERLISTS_SORTER_DROPDOWN), SetResize(1, 0), SetMinimalSize(50, 12), SetDataTip(0x0, STR_GROUP_DETAILS_ORDERLISTS_SORTER_DROPDOWN_TOOLTIP),
				NWidget(WWT_DROPDOWN, COLOUR_GREY, GIW_WIDGET_DETAILS_DROPDOWN), SetResize(1, 0), SetMinimalSize(50, 12), SetDataTip(0x0, STR_GROUP_DETAILS_MANAGE_GROUP_TOOLTIP),
				NWidget(WWT_RESIZEBOX, COLOUR_GREY),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

/** Class for managing the group info window. */
struct GroupDetailsWindow : Window {
	GroupDetailsWindowTabs tab;     ///< which tab is displayed
	Scrollbar *vscroll;
	const GroupStatistics *stat;   ///< the group statistics this window is displaying
	uint tiny_step_height;
	VehicleListIdentifier vli;

	static const StringID schedules_sorter_names[];
	static const SortersForOrderLists * schedules_sorter_funcs[];

	enum GroupDropdownList{
		ADI_REPLACE,
		ADI_UPDATE_CARGO,
	};

	DropDownList *BuildGroupDropdownList()
	{
		DropDownList *list = new DropDownList();
		if (_local_company == this->vli.company) {
			*list->Append() = new DropDownListStringItem(STR_VEHICLE_LIST_REPLACE_VEHICLES, ADI_REPLACE, false);
		}
		*list->Append() = new DropDownListStringItem(STR_GROUP_DETAILS_UPDATE_CARGO, ADI_UPDATE_CARGO, false);
		return list;
	}

	DropDownList *BuildOrderListsSorter()
	{
		DropDownList *list = new DropDownList();
		for (uint i = 0; schedules_sorter_names[i] != INVALID_STRING_ID; i++) {
			*list->Append() = new DropDownListStringItem(schedules_sorter_names[i], i, false);
		}
		return list;
	}

	/** Initialize a newly created group details window */
	GroupDetailsWindow(WindowDesc *desc, WindowNumber index) : Window(desc)
	{
		bool valid_vli = vli.UnpackIfValid(index);
		assert(valid_vli);
		Dimension dim = GetStringBoundingBox(STR_JUST_NOTHING);
		this->tiny_step_height = GetMinSizing(NWST_STEP, dim.height + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
		stat = &GroupStatistics::Get(this->vli.company, this->vli.index, this->vli.vtype);
		this->CreateNestedTree(desc);
		this->tab = GI_TAB_GENERAL;
		this->GetWidget<NWidgetCore>(GIW_WIDGET_CAPTION)->widget_data = STR_GROUP_DETAILS_CAPTION;
		this->GetWidget<NWidgetCore>(GIW_WIDGET_LIST)->SetMinimalSize(0, 4 * this->tiny_step_height);
		this->vscroll = this->GetScrollbar(GIW_WIDGET_LIST_SCROLLBAR);
		this->FinishInitNested(index);
		this->owner = this->vli.company;
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == GIW_WIDGET_CAPTION){
			SetDParam(0, STR_REPLACE_VEHICLE_TRAIN + this->vli.vtype);
			switch(this->vli.index) {
				case ALL_GROUP:
					SetDParam(1, STR_GROUP_ALL_TRAINS + this->vli.vtype);
					break;
				case DEFAULT_GROUP:
					SetDParam(1, STR_GROUP_DEFAULT_TRAINS + this->vli.vtype);
					break;
				default:
					SetDParam(1, STR_GROUP_NAME);
					SetDParam(2, this->vli.index);
					break;
			}
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case GIW_WIDGET_DETAILS: {
				/* Set width; string using most width is usually STR_GROUP_DETAILS_GENERAL_PROFIT */
				for (uint i = 0; i < 2; i++) SetDParam(i, UINT32_MAX);
				Dimension dim = GetStringBoundingBox(STR_GROUP_DETAILS_GENERAL_PROFIT);
				size->width = dim.width + WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
				size->height = this->tiny_step_height;
				break;
			}
			case GIW_WIDGET_LIST: {
				size->height =  Ceil(5 * GetMinSizing(NWST_BUTTON, this->tiny_step_height), this->tiny_step_height);
				resize->height = this->tiny_step_height;
				break;
			}
		}
	}

	inline void DrawDetails1stLine(const Rect &r) const
	{
		int y = Center(r.top, this->tiny_step_height);
		int left = r.left + WD_FRAMERECT_LEFT;
		int right = r.right - WD_FRAMERECT_RIGHT;

		switch (tab) {
			case GI_TAB_GENERAL:
				SetDParam(0, stat->num_vehicle);
				SetDParam(1, stat->num_profit_vehicle);
				DrawString(left, right, y, STR_GROUP_DETAILS_GENERAL_NUMBER);
				break;

			case GI_TAB_TIMETABLE:
				DrawString(left, right, y, STR_GROUP_DETAILS_TIMETABLE_CAPTION);
				break;

			case GI_TAB_CARGO:
				DrawString(left, right, y,  STR_GROUP_DETAILS_TOTAL_CARGO);
				break;
		}
	}

	inline void DrawDetailsList(const Rect &r) const
	{
		int y = Center(r.top, this->tiny_step_height);
		int left = r.left + WD_FRAMERECT_LEFT;
		int right = r.right - WD_FRAMERECT_RIGHT;
		switch (tab) {
			case GI_TAB_GENERAL:
				if (stat->num_profit_vehicle > 0) {
					SetDParam(0, stat->profit_last_year);
					SetDParam(1, stat->profit_last_year / stat->num_profit_vehicle);
					DrawString(left, right, y, STR_GROUP_DETAILS_GENERAL_PROFIT);
					y += this->tiny_step_height;
					SetDParam(0, stat->min_profit_vehicle >> 8);
					DrawString(left, right, y, STR_GROUP_DETAILS_GENERAL_MINIMUM_PROFIT);
				}
				break;

			case GI_TAB_TIMETABLE: {
				int max = min(this->vscroll->GetPosition() + this->vscroll->GetCapacity(), stat->order_lists.Length());
				for (int i = this->vscroll->GetPosition(); i < max; ++i) {
					const OrderList *list = stat->order_lists[i];
					SetDParam(0, STR_ORDERLIST_TYPE_INVALID + list->GetOrderListType());
					SetDParam(1, list->GetNumVehicles());
					SetDParam(2, list->GetNumOrders());
					if (list->IsCompleteTimetable() && list->GetTimetableDurationIncomplete() != 0) {
						SetTimetableParams(3, 4, list->GetTimetableTotalDuration());
						SetDParam(5, STR_GROUP_DETAILS_TIMETABLE_DETAILS_BRACKETS);
						SetDParam(6, list->GetRelativeUnpunctuality());
					} else {
						SetDParam(3, STR_ORDERLIST_TYPE_MARK_INCOMPLETE);
						SetDParam(5, STR_EMPTY);
					}
					DrawString(left, right, y, STR_GROUP_DETAILS_TIMETABLE_DETAILS, list->GetOrderListTypeColour());
					y += this->tiny_step_height;
				}
				break;
			}

			case GI_TAB_CARGO: {
				int skip = this->vscroll->GetPosition();
				int skipped = 0;
				CargoID i = 0;
				for (; i < NUM_CARGO && skipped < skip; i++) {
					if (stat->DoesGroupCarryCargoType(i)) {
						skipped++;
					}
				}
				if (stat->num_vehicle == 0) break;
				skipped = 0;
				for (; i < NUM_CARGO && skipped < this->vscroll->GetCapacity(); i++) {
					if (stat->DoesGroupCarryCargoType(i)) {
						skipped++;
						SetDParam(0, i);
						SetDParam(1, stat->act_cargo[i]);
						SetDParam(2, i);
						SetDParam(3, stat->max_cargo[i]);
						DrawString(left, right, y, STR_VEHICLE_DETAILS_TRAIN_TOTAL_CAPACITY);
						y += this->tiny_step_height;
					}
				}
				break;
			}

			default:
				break;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		switch (widget) {
			case GIW_WIDGET_DETAILS:
				this->DrawDetails1stLine(r);
				break;

			case GIW_WIDGET_LIST:
				this->DrawDetailsList(r);
				break;
		}
	}

	virtual void OnPaint()
	{
		this->SetWidgetsDisabledState(this->stat->num_vehicle == 0,
			GIW_WIDGET_DETAILS_GENERAL,
			GIW_WIDGET_DETAILS_TIMETABLE,
			GIW_WIDGET_DETAILS_CARGO,
			GIW_WIDGET_DETAILS_LIST,
			GIW_WIDGET_DETAILS_VEHICLE,
			GIW_WIDGET_DETAILS_ORDERS_TT,
			GIW_WIDGET_DETAILS_DROPDOWN,
			WIDGET_LIST_END);

		this->SetWidgetDisabledState(GIW_WIDGET_ORDERLISTS_SORTER_DROPDOWN, this->stat->order_lists.Length() < 2);

		DisableWidget(GIW_WIDGET_DETAILS_GENERAL + tab);

		/* Update scrollbar length */
		uint lines = 0;
		switch (tab) {
			case GI_TAB_CARGO:
				for (CargoID i = NUM_CARGO; i--;) {
					if (this->stat->DoesGroupCarryCargoType(i)) lines++;
				}
				break;
			case GI_TAB_TIMETABLE:
				lines = this->stat->order_lists.Length();
				break;
			default:
				break;
		}
		this->vscroll->SetCount(lines);

		/* Set text of sort by dropdown */
		this->GetWidget<NWidgetCore>(GIW_WIDGET_ORDERLISTS_SORTER_DROPDOWN)->widget_data = STR_GROUP_DETAILS_ORDERLISTS_SORTER_DROPDOWN;
		this->GetWidget<NWidgetCore>(GIW_WIDGET_DETAILS_DROPDOWN)->widget_data = STR_GROUP_DETAILS_MANAGE_GROUP;

		this->DrawWidgets();
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case GIW_WIDGET_DETAILS_GENERAL:
			case GIW_WIDGET_DETAILS_TIMETABLE:
			case GIW_WIDGET_DETAILS_CARGO:
				this->RaiseWidget(GIW_WIDGET_DETAILS_GENERAL + tab);
				tab = (GroupDetailsWindowTabs)(widget - GIW_WIDGET_DETAILS_GENERAL);
				this->LowerWidget(widget);
				break;

			case GIW_WIDGET_DETAILS_LIST:
				ShowGroupVehicleListWindow(this->vli.company, this->vli.vtype, this->vli.index);
				return;

			case GIW_WIDGET_DETAILS_VEHICLE:
			case GIW_WIDGET_DETAILS_ORDERS_TT: {
				if (stat->order_lists.Length() == 0) return;
				DropDownList *list = stat->BuildSharedOrdersDropdown();

				if (list != NULL) {
  					ShowDropDownList(this, list, 0, widget, 0, true);
				} else {
					const Vehicle *v = stat->order_lists[0]->GetFirstSharedVehicle();
					if (v == NULL) return;
					if (widget == GIW_WIDGET_DETAILS_VEHICLE) {
						ShowVehicleViewWindow(v);
					} else {
						if (_ctrl_pressed) {
							ShowTimetableWindow(v);
						} else {
							ShowOrdersWindow(v);
						}
					}
				}
				return;
			}

			case GIW_WIDGET_LIST: {
				if (tab != GI_TAB_TIMETABLE) return;
				uint id_list = this->vscroll->GetScrolledRowFromWidget(pt.y, this, GIW_WIDGET_LIST);
				if (id_list >= this->stat->order_lists.Length()) return;
				Vehicle *v = this->stat->order_lists[id_list]->GetFirstSharedVehicle();
				if (v == NULL) return;
				if (_ctrl_pressed) {
					ShowTimetableWindow(v);
				} else {
					ShowOrdersWindow(v);
				}
				return;
			}

			case GIW_WIDGET_ORDERLISTS_SORTER_DROPDOWN: {
				DropDownList *list = this->BuildOrderListsSorter();
				ShowDropDownList(this, list, 0, GIW_WIDGET_ORDERLISTS_SORTER_DROPDOWN, 0, true);
				break;
			}

			case GIW_WIDGET_DETAILS_DROPDOWN: {
				DropDownList *list = this->BuildGroupDropdownList();
				ShowDropDownList(this, list, 0, GIW_WIDGET_DETAILS_DROPDOWN, 0, true);
				break;
			}
		}
		this->SetDirty();
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		switch (widget) {
			case GIW_WIDGET_DETAILS_VEHICLE:
			case GIW_WIDGET_DETAILS_ORDERS_TT: {
				if (widget == GIW_WIDGET_DETAILS_VEHICLE) {
					ShowVehicleViewWindow(Vehicle::GetIfValid(index));
				} else {
					if (_ctrl_pressed) {
						ShowTimetableWindow(Vehicle::GetIfValid(index));
					} else {
						ShowOrdersWindow(Vehicle::GetIfValid(index));
					}
				}
				break;
			}

			case GIW_WIDGET_DETAILS_DROPDOWN:
				switch (index) {
					case ADI_REPLACE:
						ShowReplaceGroupVehicleWindow(this->vli.index, this->vli.vtype);
						break;
					case ADI_UPDATE_CARGO:
						GroupStatistics::UpdateCargo(this->vli.company, this->vli.index, this->vli.vtype);
						this->SetDirty();
						break;
				}
				break;
			case GIW_WIDGET_ORDERLISTS_SORTER_DROPDOWN:
				OrderListList *orders = const_cast<OrderListList *>(&this->stat->order_lists);
				GSortT(orders->Begin(), orders->Length(), *schedules_sorter_funcs[index]);
				this->SetDirty();
				break;
		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, GIW_WIDGET_LIST);
		this->GetWidget<NWidgetCore>(GIW_WIDGET_LIST)->widget_data = (this->vscroll->GetCapacity() << MAT_ROW_START) + (1 << MAT_COL_START);
	}
};

/** Sort order_lists by their type */
static int CDECL OrdersTypeSorter(const OrderList * const *a, const OrderList * const *b)
{
	return (*a)->GetOrderListType() - (*b)->GetOrderListType();
}

/** Sort order_lists by number of vehicles */
static int CDECL NumberOfVehiclesSorter(const OrderList * const *a, const OrderList * const *b)
{
	return (*b)->GetNumVehicles() - (*a)->GetNumVehicles();
}

/** Sort order_lists by number of orders */
static int CDECL NumberOfOrdersSorter(const OrderList * const *a, const OrderList * const *b)
{
	return (*b)->GetNumOrders() - (*a)->GetNumOrders();
}

/** Sort order_lists by timetable duration */
static int CDECL TimetableDurationSorter(const OrderList * const *a, const OrderList * const *b)
{
	bool is_a_complete, is_b_complete;
	is_a_complete = (*a)->IsCompleteTimetable();
	is_b_complete = (*b)->IsCompleteTimetable();
	if (!is_a_complete && is_b_complete) return 1;
	if (!is_b_complete && is_a_complete) return -1;

	return (*b)->GetTimetableDurationIncomplete() - (*a)->GetTimetableDurationIncomplete();
}

/** Sort order_lists by timetable duration */
static int CDECL TimetableDelaySorter(const OrderList * const *a, const OrderList * const *b)
{
	return (*b)->GetMaxUnpunctuality(true) - (*a)->GetMaxUnpunctuality(true);
}


/** Sort order_lists by timetable duration */
static int CDECL TimetableUnpunctualitySorter(const OrderList * const *a, const OrderList * const *b)
{
	return (*b)->GetMaxUnpunctuality(false) - (*a)->GetMaxUnpunctuality(false);
}

/** Sort order_lists by timetable duration */
static int CDECL TimetableRelativeUnpunctualitySorter(const OrderList * const *a, const OrderList * const *b)
{
	if (!(*a)->IsCompleteTimetable() || !(*b)->IsCompleteTimetable()) return TimetableDurationSorter(a,b);
	if ((*a)->GetTimetableDurationIncomplete() == 0 || (*b)->GetTimetableDurationIncomplete() == 0) return TimetableDurationSorter(a,b);
	return (*b)->GetRelativeUnpunctuality() - (*a)->GetRelativeUnpunctuality();
}
const StringID GroupDetailsWindow::schedules_sorter_names[] =  {
	STR_SORT_BY_ORDERS_TYPE,
	STR_SORT_BY_NUMBER_OF_VEHICLES,
	STR_SORT_BY_NUMBER_OF_ORDERS,
	STR_SORT_BY_TIMETABLE_DURATION,
	STR_SORT_BY_TIMETABLE_DELAY,
	STR_SORT_BY_TIMETABLE_UNPUNCTUALITY,
	STR_SORT_BY_RELATIVE_UNPUNCTUALITY,
	INVALID_STRING_ID
};

const SortersForOrderLists * GroupDetailsWindow::schedules_sorter_funcs[] = {
	&OrdersTypeSorter,
	&NumberOfVehiclesSorter,
	&NumberOfOrdersSorter,
	&TimetableDurationSorter,
	&TimetableDelaySorter,
	&TimetableUnpunctualitySorter,
	&TimetableRelativeUnpunctualitySorter,
};

static WindowDesc _group_details(
	WDP_AUTO, "group_details", 350, 100,
	WC_GROUP_DETAILS, WC_NONE,
	0,
	_nested_group_details_widgets, lengthof(_nested_group_details_widgets)
);

/**
 * Shows the group info window of the given group.
 * @param window_number is a packed vli
 */
void ShowGroupDetailsWindow(const WindowNumber window_number)
{
	AllocateWindowDescFront<GroupDetailsWindow>(&_group_details, window_number);
}
