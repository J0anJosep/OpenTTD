/* $Id: filterwindow_vehiclesgroups.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filterwindow_vehiclesgroups.cpp */

#include "../widgets/vehicle_widget.h"
#include "../widgets/group_widget.h"
#include "../widgets/filter_widget.h"

const uint mask_vehicle_group = 1 << LT_TOWNS |
				1 << LT_CARGO |
				1 << LT_STATIONS |
				1 << LT_ORDERLIST |
				1 << LT_VEHICLE_GROUP_PROPERTIES;

static const NWidgetPart _nested_vehicle_group_filter_widgets[] = {
	NWidget(NWID_HORIZONTAL), // Window header
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, FW_WIDGET_CAPTION),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_TOWNS), SetMinimalSize(96, 12),
				SetDataTip(STR_FILTER_TAB_TOWNS, STR_FILTER_TAB_TOWNS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_CARGO), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO, STR_FILTER_TAB_CARGO_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_STATIONS), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_STATIONS, STR_FILTER_TAB_STATIONS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_ORDERS), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_ORDERS, STR_FILTER_TAB_ORDERS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, FW_WIDGET_VEHICLE_GROUP_PROPERTIES), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_VEHICLE_GROUP_PROPERTIES, STR_FILTER_TAB_VEHICLE_GROUP_PROPERTIES_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
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

struct VehicleGroupFilter : FilterWindowBase {
	private:
	VehicleListIdentifier vli;

	public:
	/** Initialize a newly created vehicle/groups filter window */
	VehicleGroupFilter(WindowDesc *desc, WindowNumber window_number) : FilterWindowBase(desc, mask_vehicle_group, NULL)
	{
		tab = FWT_TOWNS;
		this->CreateNestedTree(desc);
		this->vscroll = this->GetScrollbar(FW_SCROLLBAR);
		this->FinishInitNested(window_number);

		this->vli.Unpack(window_number);
		if (this->vli.company != OWNER_NONE) this->owner = this->vli.company;
		this->list_owner = this->owner;
		this->GetWidget<NWidgetCore>(FW_WIDGET_CAPTION)->widget_data = STR_FILTER_CAPTION_STANDARD + this->vli.type;
		this->OnInvalidateData(0);
	}

	~VehicleGroupFilter() {
		switch (this->vli.type) {
			case VL_GROUPS_WINDOW:
				this->parent->GetWidget<NWidgetCore>(HasBit(this->window_number, 20) ? WID_GL_FILTER_GROUPS : WID_GL_FILTER_VEHICLES)->SetLowered(false);
				break;
			default:
				this->parent->GetWidget<NWidgetCore>(WID_VL_FILTER)->SetLowered(false);
				break;
		}
		this->parent->OnInvalidateData();
		this->parent->SetDirty();
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == FW_WIDGET_CAPTION) {
			SetDParam(0, STR_REPLACE_VEHICLE_TRAIN + this->vli.vtype);
			switch (this->vli.type) {
				case VL_SHARED_ORDERS:
				case VL_STANDARD:
					break;

				case VL_GROUP_LIST:
					switch (this->vli.index) {
						case ALL_GROUP:
							SetDParam(1, STR_GROUP_ALL_TRAINS + this->vli.vtype);
							break;
						case DEFAULT_GROUP:
							SetDParam(1, STR_GROUP_DEFAULT_TRAINS + this->vli.vtype);
							break;
						default:
							SetDParam(1, STR_GROUP_NAME);
							SetDParam(2, this->vli.index);
					}
					break;

				case VL_GROUPS_WINDOW:
					SetDParam(1, HasBit(window_number, 20) ? STR_DUAL_GROUPS : STR_DUAL_VEHICLES);
					break;

				case VL_STATION_LIST: // Station/Waypoint Name
					SetDParam(1, Station::IsExpected(BaseStation::Get(this->vli.index)) ? STR_STATION_NAME : STR_WAYPOINT_NAME);
					SetDParam(2, this->vli.index);
					break;

				case VL_DEPOT_LIST:
					SetDParam(1, STR_DEPOT_CAPTION);
					SetDParam(2, this->vli.vtype);
					SetDParam(3, this->vli.index);
					break;

				default: NOT_REACHED();
			}
		}
	}
};

static WindowDesc _nested_vehicle_group_filter_desc(
	WDP_AUTO, "gen_filter_window", 475, 246,
	WC_VEHICLE_GROUP_FILTER, WC_NONE,
	0,
	_nested_vehicle_group_filter_widgets, lengthof(_nested_vehicle_group_filter_widgets)
);