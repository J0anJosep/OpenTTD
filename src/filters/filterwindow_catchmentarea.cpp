/* $Id: filter_window_gui.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filterwindow_catchmentarea.cpp */

#include "../widgets/filter_widget.h"

const uint mask_catchment_area = 1 << LT_TOWNS |
				1 << LT_COMPANIES |
				1 << LT_CARGO |
				1 << LT_CARGO_ACCEPTANCE |
				1 << LT_STATION_FACILITIES |
				1 << LT_STATIONS |
				1 << LT_CATCHMENT_AREA_PROPERTIES;

static const NWidgetPart _nested_ca_layer_filter_widgets[] = {
	NWidget(NWID_HORIZONTAL), // Window header
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN, FW_WIDGET_CAPTION),
				SetDataTip(STR_FILTER_CAPTION_CATCHMENT_AREA, STR_NULL),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_TOWNS), SetMinimalSize(96, 12),
				SetDataTip(STR_FILTER_TAB_TOWNS, STR_FILTER_TAB_TOWNS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_COMPANIES), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_COMPANIES, STR_FILTER_TAB_COMPANIES_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_CARGO), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO_PRODUCTION, STR_FILTER_TAB_CARGO_PRODUCTION_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_CARGO_ACCEPTANCE), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO_ACCEPTANCE, STR_FILTER_TAB_CARGO_ACCEPTANCE_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_STATION_FACILITIES), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_STATION_FACILITIES, STR_FILTER_TAB_STATION_FACILITIES_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PANEL, COLOUR_BROWN), SetFill(1, 1), SetResize(0,1), EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_STATIONS), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_STATIONS, STR_FILTER_TAB_STATIONS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_CATCHMENT_AREA_PROPERTIES),
				SetMinimalSize(99, 12), SetDataTip(STR_FILTER_TAB_CATCHMENT_AREA_PROPERTIES, STR_FILTER_TAB_ZONING_PROPERTIES_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PANEL, COLOUR_BROWN), SetFill(1, 1), SetResize(0,1), EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_STATE), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_STATE, STR_FILTER_TAB_STATE_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_RESET), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_RESET, STR_FILTER_TAB_RESET_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_MATRIX, COLOUR_BROWN, FW_LIST), SetMinimalSize(188, 0), SetMatrixDataTip(1, 0, STR_FILTER_LIST_TOOLTIP),
				SetFill(1, 0), SetResize(1, 1), SetScrollbar(FW_SCROLLBAR),
			NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_BROWN, FW_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_BROWN),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

struct CatchmentAreaWindow : FilterWindowBase {

	public:
	/** Initialize a newly created catchment area window. */
	CatchmentAreaWindow(WindowDesc *desc, WindowNumber window_number) : FilterWindowBase(desc, mask_catchment_area, &_ca_controller)
	{
		tab = _stations_modified ? FWT_STATIONS : FWT_TOWNS;
		this->CreateNestedTree(desc);
		this->vscroll = this->GetScrollbar(FW_SCROLLBAR);
		this->FinishInitNested(window_number);

		this->owner = INVALID_COMPANY;
		this->list_owner = this->owner;

		this->OnInvalidateData(0);
	}

	~CatchmentAreaWindow() { this->active = NULL; }

	/**
	 * Some data on this window has become invalid.
	 * @param data 0 build lists, 2 update filter, 4 invalidate data for parent
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		switch (data) {
			case 0:
				this->Build(this->mask);
				/* FALL THROUGH and get Active stations. */
			case 2:
				this->GetFromActive();
				/* On Zoning Window, if we modify stations, unselect owners, cargos and select stations. */
				if (_stations_modified && this->tab >= FWT_TOWNS && this->tab <= FWT_CARGO_ACCEPTANCE)
					this->tab = FWT_STATIONS;
				break;
			case 4:
				InvalidateFilterData(LT_END, 0);
				break;
			default: NOT_REACHED();
		}

		if (this->tab == FWT_STATE) {
			this->active->SortAll();
			state_lines = this->active->Count();
		}
	}

	virtual void InvalidateFilterData(ListType list_type, uint id)
	{
		/* On Zoning Window, if we modify stations, unselect owners, cargos and select stations. */
		if (_stations_modified && this->tab >= FWT_TOWNS && this->tab <= FWT_CARGO_ACCEPTANCE)
			this->tab = FWT_STATIONS;

		if (list_type == LT_STATIONS) id = this->lists[LT_STATIONS][id].GetElement();
		UpdateCALayer(list_type, id);
	}
};

static WindowDesc _ca_layer_window(
	WDP_AUTO, "gen_filter_window", 475, 246,
	WC_CATCHMENT_AREA_WINDOW, WC_NONE,
	0,
	_nested_ca_layer_filter_widgets, lengthof(_nested_ca_layer_filter_widgets)
);