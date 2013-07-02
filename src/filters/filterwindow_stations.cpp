/* $Id: filterwindow_stations.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filterwindow_stations.cpp */

#include "../widgets/station_widget.h"
#include "../widgets/filter_widget.h"

const uint mask_stations = 1 << LT_TOWNS |
				1 << LT_CARGO |
				1 << LT_CARGO_ACCEPTANCE |
				1 << LT_STATION_FACILITIES;

static const NWidgetPart _nested_stations_filter_widgets[] = {
	NWidget(NWID_HORIZONTAL), // Window header
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, FW_WIDGET_CAPTION),
				SetDataTip(STR_FILTER_CAPTION_STATION, STR_NULL),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_TOWNS), SetMinimalSize(96, 12),
				SetDataTip(STR_FILTER_TAB_TOWNS, STR_FILTER_TAB_TOWNS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_CARGO), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO_PRODUCTION, STR_FILTER_TAB_CARGO_PRODUCTION_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_CARGO_ACCEPTANCE), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO_ACCEPTANCE, STR_FILTER_TAB_CARGO_ACCEPTANCE_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_STATION_FACILITIES), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_STATION_FACILITIES, STR_FILTER_TAB_STATION_FACILITIES_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PANEL, COLOUR_GREY), SetFill(1, 1), SetResize(0,1), EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_STATE), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_STATE, STR_FILTER_TAB_STATE_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_RESET), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_RESET, STR_FILTER_TAB_RESET_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_MATRIX, COLOUR_GREY, FW_LIST), SetMinimalSize(188, 0), SetMatrixDataTip(1, 0, STR_FILTER_LIST_TOOLTIP),
				SetFill(1, 0), SetResize(1, 1), SetScrollbar(FW_SCROLLBAR),
			NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_GREY, FW_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_GREY),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

struct StationFilter : FilterWindowBase {
	public:
	/** Initialize a newly created vehicle/groups filter window */
	StationFilter(WindowDesc *desc, WindowNumber window_number) : FilterWindowBase(desc, mask_stations, NULL)
	{
		tab = FWT_CARGO;
		this->CreateNestedTree(desc);
		this->vscroll = this->GetScrollbar(FW_SCROLLBAR);
		this->FinishInitNested(window_number);

		this->owner = (CompanyID)window_number;
		this->list_owner = this->owner;
		this->OnInvalidateData(0);
	}

	~StationFilter() {
		this->parent->GetWidget<NWidgetCore>(WID_STL_FILTER)->SetLowered(false);
		this->parent->OnInvalidateData(-1);
		this->parent->OnInvalidateData(0);
		this->parent->SetDirty();
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == FW_WIDGET_CAPTION) SetDParam(0, this->window_number);
	}

	virtual void InvalidateFilterData(ListType list_type, uint id)
	{
		if (list_type == LT_CARGO) {
			/* We need to invalidate cargo_filter on parent */
			parent->OnInvalidateData(-1);
		}
		this->OnInvalidateData(4);
	}
};

static WindowDesc _nested_stations_filter_desc(
	WDP_AUTO, "gen_filter_window", 475, 246,
	WC_STATION_FILTER, WC_NONE,
	0,
	_nested_stations_filter_widgets, lengthof(_nested_stations_filter_widgets)
);