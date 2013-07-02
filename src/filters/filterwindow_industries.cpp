/* $Id: filterwindow_industries.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filterwindow_industries.cpp */

#include "../widgets/industry_widget.h"
#include "../widgets/filter_widget.h"

const uint mask_industries = 1 << LT_TOWNS |
				1 << LT_CARGO |
				1 << LT_CARGO_ACCEPTANCE;


static const NWidgetPart _nested_industries_filter_widgets[] = {
	NWidget(NWID_HORIZONTAL), // Window header
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN, FW_WIDGET_CAPTION),
				SetDataTip(STR_FILTER_CAPTION_INDUSTRIES, STR_NULL),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_TOWNS), SetMinimalSize(96, 12),
				SetDataTip(STR_FILTER_TAB_TOWNS, STR_FILTER_TAB_TOWNS_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_CARGO), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO_PRODUCTION, STR_FILTER_TAB_CARGO_PRODUCTION_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, FW_WIDGET_CARGO_ACCEPTANCE), SetMinimalSize(99, 12),
				SetDataTip(STR_FILTER_TAB_CARGO_ACCEPTANCE, STR_FILTER_TAB_CARGO_ACCEPTANCE_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
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

struct IndustryFilter : FilterWindowBase {
	public:
	/** Initialize a newly created vehicle/groups filter window */
	IndustryFilter(WindowDesc *desc, WindowNumber window_number) : FilterWindowBase(desc, mask_industries, NULL)
	{
		tab = FWT_TOWNS;
		this->CreateNestedTree(desc);
		this->vscroll = this->GetScrollbar(FW_SCROLLBAR);
		this->FinishInitNested(window_number);

		this->owner = INVALID_COMPANY;
		this->list_owner = this->owner;
		this->OnInvalidateData(0);
	}

	~IndustryFilter() {
		this->parent->GetWidget<NWidgetCore>(WID_ID_FILTER)->SetLowered(false);
		this->parent->OnInvalidateData();
		this->parent->SetDirty();
	}
};

static WindowDesc _nested_industries_filter_desc(
	WDP_AUTO, "gen_filter_window", 475, 246,
	WC_INDUSTRY_FILTER, WC_NONE,
	0,
	_nested_industries_filter_widgets, lengthof(_nested_industries_filter_widgets)
);