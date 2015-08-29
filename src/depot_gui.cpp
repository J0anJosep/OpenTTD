/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file depot_gui.cpp The GUI for depots. */

#include "stdafx.h"
#include "train.h"
#include "roadveh.h"
#include "ship.h"
#include "aircraft.h"
#include "gui.h"
#include "textbuf_gui.h"
#include "viewport_func.h"
#include "command_func.h"
#include "depot_base.h"
#include "spritecache.h"
#include "strings_func.h"
#include "vehicle_func.h"
#include "company_func.h"
#include "tilehighlight_func.h"
#include "window_gui.h"
#include "vehiclelist.h"
#include "order_backup.h"
#include "zoom_func.h"
#include "core/geometry_func.hpp"

#include "widgets/depot_widget.h"
#include "widgets/station_widget.h"

#include "table/strings.h"

#include "safeguards.h"

/*
 * Since all depot window sizes aren't the same, we need to modify sizes a little.
 * It's done with the following arrays of widget indexes. Each of them tells if a widget side should be moved and in what direction.
 * How long they should be moved and for what window types are controlled in ShowDepotWindow()
 */

/** Nested widget definition for train depots. */
static const NWidgetPart _nested_train_depot_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_D_CAPTION), SetDataTip(STR_DEPOT_CAPTION, STR_NULL),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_MATRIX, COLOUR_GREY, WID_D_MATRIX), SetResize(1, 1), SetScrollbar(WID_D_V_SCROLL),
			NWidget(NWID_SELECTION, INVALID_COLOUR, WID_D_SHOW_H_SCROLL),
				NWidget(NWID_HSCROLLBAR, COLOUR_GREY, WID_D_H_SCROLL),
			EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_D_SELL), SetDataTip(0x0, STR_NULL), SetResize(0, 1), SetFill(0, 1),
			NWidget(NWID_SELECTION, INVALID_COLOUR, WID_D_SHOW_SELL_CHAIN),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_D_SELL_CHAIN), SetDataTip(SPR_SELL_CHAIN_TRAIN, STR_DEPOT_DRAG_WHOLE_TRAIN_TO_SELL_TOOLTIP), SetResize(0, 1), SetFill(0, 1),
			EndContainer(),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_D_SELL_ALL), SetResize(0, 1), SetFill(0, 1), SetDataTip(0x0, STR_NULL),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_D_AUTOREPLACE), SetResize(0, 1), SetFill(0, 1), SetDataTip(0x0, STR_NULL),
		EndContainer(),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_D_V_SCROLL),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_D_BUILD), SetDataTip(0x0, STR_NULL), SetFill(1, 1), SetResize(1, 0),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_D_CLONE), SetDataTip(0x0, STR_NULL), SetFill(1, 1), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_D_LOCATION), SetDataTip(STR_BUTTON_LOCATION, STR_NULL), SetFill(1, 1), SetResize(1, 0),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_D_SHOW_RENAME), // rename button
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_D_RENAME), SetDataTip(STR_BUTTON_RENAME, STR_DEPOT_RENAME_TOOLTIP), SetFill(1, 1), SetResize(1, 0),
		EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_D_VEHICLE_LIST), SetDataTip(0x0, STR_NULL), SetFill(0, 1),
		NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_D_STOP_ALL), SetDataTip(SPR_FLAG_VEH_STOPPED, STR_NULL), SetFill(0, 1),
		NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_D_START_ALL), SetDataTip(SPR_FLAG_VEH_RUNNING, STR_NULL), SetFill(0, 1),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};

static WindowDesc _train_depot_desc(
	WDP_AUTO, "depot_train", 362, 123,
	WC_VEHICLE_DEPOT, WC_NONE,
	0,
	_nested_train_depot_widgets, lengthof(_nested_train_depot_widgets)
);

static WindowDesc _road_depot_desc(
	WDP_AUTO, "depot_roadveh", 316, 97,
	WC_VEHICLE_DEPOT, WC_NONE,
	0,
	_nested_train_depot_widgets, lengthof(_nested_train_depot_widgets)
);

static WindowDesc _ship_depot_desc(
	WDP_AUTO, "depot_ship", 306, 99,
	WC_VEHICLE_DEPOT, WC_NONE,
	0,
	_nested_train_depot_widgets, lengthof(_nested_train_depot_widgets)
);

static WindowDesc _aircraft_depot_desc(
	WDP_AUTO, "depot_aircraft", 332, 99,
	WC_VEHICLE_DEPOT, WC_NONE,
	0,
	_nested_train_depot_widgets, lengthof(_nested_train_depot_widgets)
);

extern void DepotSortList(VehicleList *list);

/**
 * This is the Callback method after the cloning attempt of a vehicle
 * @param result the result of the cloning command
 * @param tile unused
 * @param p1 unused
 * @param p2 unused
 */
void CcCloneVehicle(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Failed()) return;

	const Vehicle *v = Vehicle::Get(_new_vehicle_id);

	ShowVehicleViewWindow(v);
}

static void TrainDepotMoveVehicle(const Vehicle *wagon, VehicleID sel, const Vehicle *head)
{
	const Vehicle *v = Vehicle::Get(sel);

	if (v == wagon) return;

	if (wagon == NULL) {
		if (head != NULL) wagon = head->Last();
	} else {
		wagon = wagon->Previous();
		if (wagon == NULL) return;
	}

	if (wagon == v) return;

	DoCommandP(v->tile, v->index | (_ctrl_pressed ? 1 : 0) << 20, wagon == NULL ? INVALID_VEHICLE : wagon->index, CMD_MOVE_RAIL_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_MOVE_VEHICLE));
}

static VehicleCellSize _base_block_sizes_depot[VEH_COMPANY_END];    ///< Cell size for vehicle images in the depot view.
static VehicleCellSize _base_block_sizes_purchase[VEH_COMPANY_END]; ///< Cell size for vehicle images in the purchase list.
static uint _consistent_train_width;                                ///< Whether trains of all lengths are consistently scaled. Either TRAININFO_DEFAULT_VEHICLE_WIDTH, VEHICLEINFO_FULL_VEHICLE_WIDTH, or 0.

/**
 * Get the GUI cell size for a vehicle image.
 * @param type Vehicle type to get the size for.
 * @param image_type Image type to get size for.
 * @pre image_type == EIT_IN_DEPOT || image_type == EIT_PURCHASE
 * @return Cell dimensions for the vehicle and image type.
 */
VehicleCellSize GetVehicleImageCellSize(VehicleType type, EngineImageType image_type)
{
	switch (image_type) {
		case EIT_IN_DEPOT: return _base_block_sizes_depot[type];
		case EIT_PURCHASE: return _base_block_sizes_purchase[type];
		default: NOT_REACHED();
	}
}

static void InitBlocksizeForVehicles(VehicleType type, EngineImageType image_type)
{
	int max_extend_left  = 0;
	int max_extend_right = 0;
	uint max_height = 0;

	const Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, type) {
		if (!e->IsEnabled()) continue;

		EngineID eid = e->index;
		uint x, y;
		int x_offs, y_offs;

		switch (type) {
			default: NOT_REACHED();
			case VEH_TRAIN:    GetTrainSpriteSize(   eid, x, y, x_offs, y_offs, image_type); break;
			case VEH_ROAD:     GetRoadVehSpriteSize( eid, x, y, x_offs, y_offs, image_type); break;
			case VEH_SHIP:     GetShipSpriteSize(    eid, x, y, x_offs, y_offs, image_type); break;
			case VEH_AIRCRAFT: GetAircraftSpriteSize(eid, x, y, x_offs, y_offs, image_type); break;
		}
		if (y > max_height) max_height = y;
		if (-x_offs > max_extend_left) max_extend_left = -x_offs;
		if ((int)x + x_offs > max_extend_right) max_extend_right = x + x_offs;
	}

	int min_extend = ScaleGUIPixels(16);
	int max_extend = ScaleGUIPixels(98);

	switch (image_type) {
		case EIT_IN_DEPOT:
			_base_block_sizes_depot[type].height       = max<uint>(ScaleGUIPixels(GetVehicleHeight(type)), max_height);
			_base_block_sizes_depot[type].extend_left  = Clamp(max_extend_left, min_extend, max_extend);
			_base_block_sizes_depot[type].extend_right = Clamp(max_extend_right, min_extend, max_extend);
			break;
		case EIT_PURCHASE:
			_base_block_sizes_purchase[type].height       = max<uint>(ScaleGUIPixels(GetVehicleHeight(type)), max_height);
			_base_block_sizes_purchase[type].extend_left  = Clamp(max_extend_left, min_extend, max_extend);
			_base_block_sizes_purchase[type].extend_right = Clamp(max_extend_right, min_extend, max_extend);
			break;

		default: NOT_REACHED();
	}
}

/**
 * Set the size of the blocks in the window so we can be sure that they are big enough for the vehicle sprites in the current game.
 * @note Calling this function once for each game is enough.
 */
void InitDepotWindowBlockSizes()
{
	for (VehicleType vt = VEH_BEGIN; vt < VEH_COMPANY_END; vt++) {
		InitBlocksizeForVehicles(vt, EIT_IN_DEPOT);
		InitBlocksizeForVehicles(vt, EIT_PURCHASE);
	}

	_consistent_train_width = TRAININFO_DEFAULT_VEHICLE_WIDTH;
	bool first = true;
	const Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, VEH_TRAIN) {
		if (!e->IsEnabled()) continue;

		uint w = TRAININFO_DEFAULT_VEHICLE_WIDTH;
		if (e->GetGRF() != NULL && is_custom_sprite(e->u.rail.image_index)) {
			w = e->GetGRF()->traininfo_vehicle_width;
			if (w != VEHICLEINFO_FULL_VEHICLE_WIDTH) {
				/* Hopeless.
				 * This is a NewGRF vehicle that uses TRAININFO_DEFAULT_VEHICLE_WIDTH.
				 * If the vehicles are shorter than 8/8 we have fractional lengths, which are not consistent after rounding.
				 */
				_consistent_train_width = 0;
				break;
			}
		}

		if (first) {
			_consistent_train_width = w;
			first = false;
		} else if (w != _consistent_train_width) {
			_consistent_train_width = 0;
			break;
		}
	}
}

static void DepotSellAllConfirmationCallback(Window *w, bool confirmed);
const Sprite *GetAircraftSprite(EngineID engine);

struct DepotWindow : Window {
	VehicleID sel;
	VehicleID vehicle_over; ///< Rail vehicle over which another one is dragged, \c INVALID_VEHICLE if none.
	VehicleType type;
	Depot *depot;
	bool generate_list;
	int hovered_widget; ///< Index of the widget being hovered during drag/drop. -1 if no drag is in progress.
	VehicleList vehicle_list;
	VehicleList wagon_list;
	uint unitnumber_digits;
	uint num_columns;       ///< Number of columns.
	Scrollbar *hscroll;     ///< Only for trains.
	Scrollbar *vscroll;

	DepotWindow(WindowDesc *desc, DepotID depot_id, VehicleType type) : Window(desc)
	{
		assert(IsCompanyBuildableVehicleType(type)); // ensure that we make the call with a valid type

		this->depot = Depot::Get(depot_id);
		assert(type == this->depot->veh_type);
		this->type = type;
		this->sel = INVALID_VEHICLE;
		this->vehicle_over = INVALID_VEHICLE;
		this->generate_list = true;
		this->hovered_widget = -1;
		this->type = type;
		this->num_columns = 1; // for non-trains this gets set in FinishInitNested()
		this->unitnumber_digits = 2;

		this->CreateNestedTree();
		this->hscroll = (this->type == VEH_TRAIN ? this->GetScrollbar(WID_D_H_SCROLL) : NULL);
		this->vscroll = this->GetScrollbar(WID_D_V_SCROLL);
		/* Don't show 'rename button' of aircraft hangar */
		this->GetWidget<NWidgetStacked>(WID_D_SHOW_RENAME)->SetDisplayedPlane(type == VEH_AIRCRAFT ? SZSP_NONE : 0);
		/* Only train depots have a horizontal scrollbar and a 'sell chain' button */
		if (type == VEH_TRAIN) this->GetWidget<NWidgetCore>(WID_D_MATRIX)->widget_data = 1 << MAT_COL_START;
		this->GetWidget<NWidgetStacked>(WID_D_SHOW_H_SCROLL)->SetDisplayedPlane(type == VEH_TRAIN ? 0 : SZSP_HORIZONTAL);
		this->GetWidget<NWidgetStacked>(WID_D_SHOW_SELL_CHAIN)->SetDisplayedPlane(type == VEH_TRAIN ? 0 : SZSP_NONE);
		this->SetupWidgetData(type);
		this->FinishInitNested(depot_id);

		this->owner = this->depot->company;

		OrderBackup::Reset();
	}

	~DepotWindow()
	{
		DeleteWindowById(WC_BUILD_VEHICLE, this->window_number);
		OrderBackup::Reset(this->window_number);
	}

	/**
	 * Draw a vehicle in the depot window in the box with the top left corner at x,y.
	 * @param v     Vehicle to draw.
	 * @param left  Left side of the box to draw in.
	 * @param right Right side of the box to draw in.
	 * @param y     Top of the box to draw in.
	 */
	void DrawVehicleInDepot(const Vehicle *v, int left, int right, int y) const
	{
		bool free_wagon = false;
		int sprite_y = Center(y, this->resize.step_height, ScaleGUIPixels(GetVehicleHeight(v->type)));

		bool rtl = _current_text_dir == TD_RTL;
		int image_left  = rtl ? left  + this->count_width  : left  + this->header_width;
		int image_right = rtl ? right - this->header_width : right - this->count_width;

		switch (v->type) {
			case VEH_TRAIN: {
				const Train *u = Train::From(v);
				free_wagon = u->IsFreeWagon();

				uint x_space = free_wagon ?
						ScaleGUIPixels(_consistent_train_width != 0 ? _consistent_train_width : TRAININFO_DEFAULT_VEHICLE_WIDTH) :
						0;

				DrawTrainImage(u, image_left + (rtl ? 0 : x_space), image_right - (rtl ? x_space : 0), sprite_y,
						this->sel, EIT_IN_DEPOT, free_wagon ? 0 : this->hscroll->GetPosition(), this->vehicle_over);

				/* Length of consist in tiles with 1 fractional digit (rounded up) */
				SetDParam(0, CeilDiv(u->gcache.cached_total_length * 10, TILE_SIZE));
				SetDParam(1, 1);
				DrawString(rtl ? left + SWD_FRAMERECT_LEFT : right - this->count_width, rtl ? left + this->count_width : right - SWD_FRAMERECT_RIGHT, Center(y, this->resize.step_height, FONT_HEIGHT_SMALL), STR_TINY_BLACK_DECIMAL, TC_FROMSTRING, SA_RIGHT); // Draw the counter
				break;
			}

			case VEH_ROAD:     DrawRoadVehImage( v, image_left, image_right, sprite_y, this->sel, EIT_IN_DEPOT); break;
			case VEH_SHIP:     DrawShipImage(    v, image_left, image_right, sprite_y, this->sel, EIT_IN_DEPOT); break;
			case VEH_AIRCRAFT: DrawAircraftImage(v, image_left, image_right, sprite_y, this->sel, EIT_IN_DEPOT); break;
			default: NOT_REACHED();
		}

		uint diff_x, y_sprite, y_num;
		if (v->IsGroundVehicle()) {
			/* Arrange unitnumber and flag horizontally */
			diff_x = this->flag_width + SWD_FRAMERECT_LEFT;
			y_sprite = Center(y, this->resize.step_height, this->flag_height);
			y_num = Center(y, this->resize.step_height);
		} else {
			/* Arrange unitnumber and flag vertically */
			diff_x = SWD_FRAMERECT_LEFT;
			y_num = Center(y, this->resize.step_height, FONT_HEIGHT_NORMAL + this->flag_height + 2);
			y_sprite = y_num + FONT_HEIGHT_NORMAL;
		}
		int text_left  = rtl ? right - this->header_width - 1 : left + diff_x;
		int text_right = rtl ? right - diff_x : left + this->header_width - 1;

		if (free_wagon) {
			DrawString(text_left, text_right, Center(y, this->resize.step_height), STR_DEPOT_NO_ENGINE);
		} else {
			DrawSprite((v->vehstatus & VS_STOPPED) ? SPR_FLAG_VEH_STOPPED : SPR_FLAG_VEH_RUNNING, PAL_NONE, rtl ? right - this->flag_width : left + SWD_FRAMERECT_LEFT, y_sprite);

			SetDParam(0, v->unitnumber);
			DrawString(text_left, text_right, y_num, (uint16)(v->max_age - DAYS_IN_LEAP_YEAR) >= v->age ? STR_BLACK_COMMA : STR_RED_COMMA);
		}
	}

	void DrawWidget(const Rect &r, int widget) const
	{
		if (widget != WID_D_MATRIX) return;

		bool rtl = _current_text_dir == TD_RTL;

		/* Set the row and number of boxes in each row based on the number of boxes drawn in the matrix */
		const NWidgetCore *wid = this->GetWidget<NWidgetCore>(WID_D_MATRIX);

		/* Draw vertical separators at whole tiles.
		 * This only works in two cases:
		 *  - All vehicles use VEHICLEINFO_FULL_VEHICLE_WIDTH as reference width.
		 *  - All vehicles are 8/8. This cannot be checked for NewGRF, so instead we check for "all vehicles are original vehicles".
		 */
		if (this->type == VEH_TRAIN && _consistent_train_width != 0) {
			int w = ScaleGUIPixels(2 * _consistent_train_width);
			int col = _colour_gradient[wid->colour][4];
			int image_left  = rtl ? r.left  + this->count_width  : r.left  + this->header_width;
			int image_right = rtl ? r.right - this->header_width : r.right - this->count_width;
			int first_line = w + (-this->hscroll->GetPosition()) % w;
			if (rtl) {
				for (int x = image_right - first_line; x >= image_left; x -= w) {
					GfxDrawLine(x, r.top + WD_GUI_UNIT, x, r.bottom - WD_GUI_UNIT, col, WD_GUI_UNIT, 0);
				}
			} else {
				for (int x = image_left + first_line; x <= image_right; x += w) {
					GfxDrawLine(x, r.top + WD_GUI_UNIT, x, r.bottom - WD_GUI_UNIT, col, WD_GUI_UNIT, 0);
				}
			}
		}

		uint16 rows_in_display = wid->current_y / wid->resize_y;

		uint16 num = this->vscroll->GetPosition() * this->num_columns;
		int maxval = min(this->vehicle_list.Length(), num + (rows_in_display * this->num_columns));
		int y;
		for (y = r.top; num < maxval; y += this->resize.step_height) { // Draw the rows
			for (byte i = 0; i < this->num_columns && num < maxval; i++, num++) {
				/* Draw all vehicles in the current row */
				const Vehicle *v = this->vehicle_list[num];
				if (this->num_columns == 1) {
					this->DrawVehicleInDepot(v, r.left, r.right, y);
				} else {
					int x = r.left + (rtl ? (this->num_columns - i - 1) : i) * this->resize.step_width;
					this->DrawVehicleInDepot(v, x, x + this->resize.step_width - 1, y);
				}
			}
		}

		maxval = min(this->vehicle_list.Length() + this->wagon_list.Length(), (this->vscroll->GetPosition() * this->num_columns) + (rows_in_display * this->num_columns));

		/* Draw the train wagons without an engine in front. */
		for (; num < maxval; num++, y += this->resize.step_height) {
			const Vehicle *v = this->wagon_list[num - this->vehicle_list.Length()];
			this->DrawVehicleInDepot(v, r.left, r.right, y);
		}
	}

	void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_D_CAPTION: {
				/* locate the depot struct */
				SetDParam(0, this->type);
				SetDParam(1, (this->type == VEH_AIRCRAFT) ? GetStationIndex(this->depot->xy) : this->window_number);
				break;
			}
			case WID_D_VEHICLE_LIST:
				SetDParam(0, STR_TRAIN + this->type);
				break;
			default: break;
		}
	}

	struct GetDepotVehiclePtData {
		const Vehicle *head;
		const Vehicle *wagon;
	};

	enum DepotGUIAction {
		MODE_ERROR,
		MODE_DRAG_VEHICLE,
		MODE_SHOW_VEHICLE,
		MODE_START_STOP,
	};

	DepotGUIAction GetVehicleFromDepotWndPt(int x, int y, const Vehicle **veh, GetDepotVehiclePtData *d) const
	{
		const NWidgetCore *matrix_widget = this->GetWidget<NWidgetCore>(WID_D_MATRIX);
		/* In case of RTL the widgets are swapped as a whole */
		if (_current_text_dir == TD_RTL) x = matrix_widget->current_x - x;

		uint xt = 0, xm = 0, ym = 0;
		if (this->type == VEH_TRAIN) {
			xm = x;
		} else {
			xt = x / this->resize.step_width;
			xm = x % this->resize.step_width;
			if (xt >= this->num_columns) return MODE_ERROR;
		}
		ym = y % this->resize.step_height;

		uint row = y / this->resize.step_height;
		if (row >= this->vscroll->GetCapacity()) return MODE_ERROR;

		uint pos = ((row + this->vscroll->GetPosition()) * this->num_columns) + xt;

		if (this->vehicle_list.Length() + this->wagon_list.Length() <= pos) {
			/* Clicking on 'line' / 'block' without a vehicle */
			if (this->type == VEH_TRAIN) {
				/* End the dragging */
				d->head  = NULL;
				d->wagon = NULL;
				return MODE_DRAG_VEHICLE;
			} else {
				return MODE_ERROR; // empty block, so no vehicle is selected
			}
		}

		bool wagon = false;
		if (this->vehicle_list.Length() > pos) {
			*veh = this->vehicle_list[pos];
			/* Skip vehicles that are scrolled off the list */
			if (this->type == VEH_TRAIN) x += this->hscroll->GetPosition();
		} else {
			pos -= this->vehicle_list.Length();
			*veh = this->wagon_list[pos];
			/* free wagons don't have an initial loco. */
			x -= ScaleGUIPixels(VEHICLEINFO_FULL_VEHICLE_WIDTH);
			wagon = true;
		}

		const Train *v = NULL;
		if (this->type == VEH_TRAIN) {
			v = Train::From(*veh);
			d->head = d->wagon = v;
		}

		if (xm <= this->header_width) {
			switch (this->type) {
				case VEH_TRAIN:
					if (wagon) return MODE_ERROR;
					FALLTHROUGH;

				case VEH_ROAD:
					if (xm <= this->flag_width) return MODE_START_STOP;
					break;

				case VEH_SHIP:
				case VEH_AIRCRAFT:
					if (xm <= this->flag_width && ym >= (uint)(FONT_HEIGHT_NORMAL + SWD_PAR_VSEP_NORMAL)) return MODE_START_STOP;
					break;

				default: NOT_REACHED();
			}
			return MODE_SHOW_VEHICLE;
		}

		if (this->type != VEH_TRAIN) return MODE_DRAG_VEHICLE;

		/* Clicking on the counter */
		if (xm >= matrix_widget->current_x - this->count_width) return wagon ? MODE_ERROR : MODE_SHOW_VEHICLE;

		/* Account for the header */
		x -= this->header_width;

		/* find the vehicle in this row that was clicked */
		for (; v != NULL; v = v->Next()) {
			x -= v->GetDisplayImageWidth();
			if (x < 0) break;
		}

		d->wagon = (v != NULL ? v->GetFirstEnginePart() : NULL);

		return MODE_DRAG_VEHICLE;
	}

	/**
	 * Handle click in the depot matrix.
	 * @param x Horizontal position in the matrix widget in pixels.
	 * @param y Vertical position in the matrix widget in pixels.
	 */
	void DepotClick(int x, int y)
	{
		GetDepotVehiclePtData gdvp = { NULL, NULL };
		const Vehicle *v = NULL;
		DepotGUIAction mode = this->GetVehicleFromDepotWndPt(x, y, &v, &gdvp);

		if (this->type == VEH_TRAIN) v = gdvp.wagon;

		switch (mode) {
			case MODE_ERROR: // invalid
				return;

			case MODE_DRAG_VEHICLE: { // start dragging of vehicle
				if (v != NULL && VehicleClicked(v)) return;

				VehicleID sel = this->sel;

				if (this->type == VEH_TRAIN && sel != INVALID_VEHICLE) {
					this->sel = INVALID_VEHICLE;
					TrainDepotMoveVehicle(v, sel, gdvp.head);
				} else if (v != NULL) {
					SetObjectToPlaceWnd(SPR_CURSOR_MOUSE, PAL_NONE, HT_DRAG, this);
					SetMouseCursorVehicle(v, EIT_IN_DEPOT);
					_cursor.vehchain = _ctrl_pressed;

					this->sel = v->index;
					this->SetDirty();
				}
				break;
			}

			case MODE_SHOW_VEHICLE: // show info window
				ShowVehicleViewWindow(v);
				break;

			case MODE_START_STOP: // click start/stop flag
				StartStopVehicle(v, false);
				break;

			default: NOT_REACHED();
		}
	}

	/**
	 * Function to set up vehicle specific widgets (mainly sprites and strings).
	 * Only use this function to if the widget is used for several vehicle types and each has
	 * different text/sprites. If the widget is only used for a single vehicle type, or the same
	 * text/sprites are used every time, use the nested widget array to initialize the widget.
	 */
	void SetupWidgetData(VehicleType type)
	{
		this->GetWidget<NWidgetCore>(WID_D_STOP_ALL)->tool_tip     = STR_DEPOT_MASS_STOP_DEPOT_TRAIN_TOOLTIP + type;
		this->GetWidget<NWidgetCore>(WID_D_START_ALL)->tool_tip    = STR_DEPOT_MASS_START_DEPOT_TRAIN_TOOLTIP + type;
		this->GetWidget<NWidgetCore>(WID_D_SELL)->tool_tip         = STR_DEPOT_TRAIN_SELL_TOOLTIP + type;
		this->GetWidget<NWidgetCore>(WID_D_SELL_ALL)->tool_tip     = STR_DEPOT_SELL_ALL_BUTTON_TRAIN_TOOLTIP + type;

		this->GetWidget<NWidgetCore>(WID_D_BUILD)->SetDataTip(STR_DEPOT_TRAIN_NEW_VEHICLES_BUTTON + type, STR_DEPOT_TRAIN_NEW_VEHICLES_TOOLTIP + type);
		this->GetWidget<NWidgetCore>(WID_D_CLONE)->SetDataTip(STR_DEPOT_CLONE_TRAIN + type, STR_DEPOT_CLONE_TRAIN_DEPOT_INFO + type);

		this->GetWidget<NWidgetCore>(WID_D_LOCATION)->tool_tip     = STR_DEPOT_TRAIN_LOCATION_TOOLTIP + type;
		this->GetWidget<NWidgetCore>(WID_D_VEHICLE_LIST)->tool_tip = STR_DEPOT_VEHICLE_ORDER_LIST_TRAIN_TOOLTIP + type;
		this->GetWidget<NWidgetCore>(WID_D_AUTOREPLACE)->tool_tip  = STR_DEPOT_AUTOREPLACE_TRAIN_TOOLTIP + type;
		this->GetWidget<NWidgetCore>(WID_D_MATRIX)->tool_tip       = STR_DEPOT_TRAIN_LIST_TOOLTIP + this->type;

		switch (type) {
			default: NOT_REACHED();

			case VEH_TRAIN:
				this->GetWidget<NWidgetCore>(WID_D_VEHICLE_LIST)->widget_data = STR_TRAIN;

				/* Sprites */
				this->GetWidget<NWidgetCore>(WID_D_SELL)->widget_data        = SPR_SELL_TRAIN;
				this->GetWidget<NWidgetCore>(WID_D_SELL_ALL)->widget_data    = SPR_SELL_ALL_TRAIN;
				this->GetWidget<NWidgetCore>(WID_D_AUTOREPLACE)->widget_data = SPR_REPLACE_TRAIN;
				break;

			case VEH_ROAD:
				this->GetWidget<NWidgetCore>(WID_D_VEHICLE_LIST)->widget_data = STR_LORRY;

				/* Sprites */
				this->GetWidget<NWidgetCore>(WID_D_SELL)->widget_data        = SPR_SELL_ROADVEH;
				this->GetWidget<NWidgetCore>(WID_D_SELL_ALL)->widget_data    = SPR_SELL_ALL_ROADVEH;
				this->GetWidget<NWidgetCore>(WID_D_AUTOREPLACE)->widget_data = SPR_REPLACE_ROADVEH;
				break;

			case VEH_SHIP:
				this->GetWidget<NWidgetCore>(WID_D_VEHICLE_LIST)->widget_data = STR_SHIP;

				/* Sprites */
				this->GetWidget<NWidgetCore>(WID_D_SELL)->widget_data        = SPR_SELL_SHIP;
				this->GetWidget<NWidgetCore>(WID_D_SELL_ALL)->widget_data    = SPR_SELL_ALL_SHIP;
				this->GetWidget<NWidgetCore>(WID_D_AUTOREPLACE)->widget_data = SPR_REPLACE_SHIP;
				break;

			case VEH_AIRCRAFT:
				this->GetWidget<NWidgetCore>(WID_D_VEHICLE_LIST)->widget_data = STR_PLANE;

				/* Sprites */
				this->GetWidget<NWidgetCore>(WID_D_SELL)->widget_data        = SPR_SELL_AIRCRAFT;
				this->GetWidget<NWidgetCore>(WID_D_SELL_ALL)->widget_data    = SPR_SELL_ALL_AIRCRAFT;
				this->GetWidget<NWidgetCore>(WID_D_AUTOREPLACE)->widget_data = SPR_REPLACE_AIRCRAFT;
				break;
		}
	}

	uint count_width;
	uint header_width;
	uint flag_width;
	uint flag_height;

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_D_MATRIX: {
				uint min_height = 0;

				if (this->type == VEH_TRAIN) {
					SetDParamMaxValue(0, 1000, 0, FS_SMALL);
					SetDParam(1, 1);
					this->count_width = GetStringBoundingBox(STR_TINY_BLACK_DECIMAL).width +
							ScaleGUIPixels(WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT);
				} else {
					this->count_width = 0;
				}

				SetDParamMaxDigits(0, this->unitnumber_digits);
				Dimension unumber = GetStringBoundingBox(STR_BLACK_COMMA);
				const Sprite *spr = GetSprite(SPR_FLAG_VEH_STOPPED, ST_NORMAL);
				this->flag_width  = UnScaleGUI(spr->width) + SWD_FRAMERECT_RIGHT;
				this->flag_height = UnScaleGUI(spr->height);

				if (this->type == VEH_TRAIN || this->type == VEH_ROAD) {
					min_height = max<uint>(unumber.height + SWD_MATRIX_TOP, UnScaleGUI(spr->height));
					this->header_width = unumber.width + this->flag_width + SWD_FRAMERECT_LEFT;
				} else {
					min_height = unumber.height + UnScaleGUI(spr->height) + ScaleGUIPixels(WD_MATRIX_TOP + WD_PAR_VSEP_NORMAL + WD_MATRIX_BOTTOM);
					this->header_width = max<uint>(unumber.width, this->flag_width) + SWD_FRAMERECT_RIGHT;
				}
				int base_width = this->count_width + this->header_width;

				resize->height = max<uint>(GetVehicleImageCellSize(this->type, EIT_IN_DEPOT).height, min_height);
				resize->height = GetMinSizing(NWST_STEP, resize->height);
				if (this->type == VEH_TRAIN) {
					resize->width = 1;
					size->width = base_width + 2 * ScaleGUIPixels(29); // about 2 parts
					size->height = resize->height * 6;
				} else {
					resize->width = base_width + GetVehicleImageCellSize(this->type, EIT_IN_DEPOT).extend_left + GetVehicleImageCellSize(this->type, EIT_IN_DEPOT).extend_right;
					size->width = resize->width * (this->type == VEH_ROAD ? 5 : 3);
					size->height = resize->height * (this->type == VEH_ROAD ? 5 : 3);
				}
				fill->width = resize->width;
				fill->height = resize->height;
				break;
			}
		}
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		this->generate_list = true;
	}

	virtual void OnPaint()
	{
		if (this->generate_list) {
			/* Generate the vehicle list
			 * It's ok to use the wagon pointers for non-trains as they will be ignored */
			BuildDepotVehicleList(this->type, this->window_number, &this->vehicle_list, &this->wagon_list);
			this->generate_list = false;
			DepotSortList(&this->vehicle_list);

			uint new_unitnumber_digits = GetUnitNumberDigits(this->vehicle_list);
			/* Only increase the size; do not decrease to prevent constant changes */
			if (this->unitnumber_digits < new_unitnumber_digits) {
				this->unitnumber_digits = new_unitnumber_digits;
				this->ReInit();
			}
		}

		/* determine amount of items for scroller */
		if (this->type == VEH_TRAIN) {
			uint max_width = ScaleGUIPixels(VEHICLEINFO_FULL_VEHICLE_WIDTH);
			for (uint num = 0; num < this->vehicle_list.Length(); num++) {
				uint width = 0;
				for (const Train *v = Train::From(this->vehicle_list[num]); v != NULL; v = v->Next()) {
					width += v->GetDisplayImageWidth();
				}
				max_width = max(max_width, width);
			}
			/* Always have 1 empty row, so people can change the setting of the train */
			this->vscroll->SetCount(this->vehicle_list.Length() + this->wagon_list.Length() + 1);
			/* Always make it longer than the longest train, so you can attach vehicles at the end, and also see the next vertical tile separator line */
			this->hscroll->SetCount(max_width + ScaleGUIPixels(2 * VEHICLEINFO_FULL_VEHICLE_WIDTH + 1));
		} else {
			this->vscroll->SetCount(CeilDiv(this->vehicle_list.Length(), this->num_columns));
		}

		/* Setup disabled buttons. */
		this->SetWidgetsDisabledState(this->owner != _local_company,
			WID_D_STOP_ALL,
			WID_D_START_ALL,
			WID_D_SELL,
			WID_D_SELL_CHAIN,
			WID_D_SELL_ALL,
			WID_D_BUILD,
			WID_D_CLONE,
			WID_D_RENAME,
			WID_D_AUTOREPLACE,
			WIDGET_LIST_END);

		this->DrawWidgets();
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		TileIndex tile = this->depot->xy;
		switch (widget) {
			case WID_D_MATRIX: { // List
				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_D_MATRIX);
				this->DepotClick(pt.x - nwi->pos_x, pt.y - nwi->pos_y);
				break;
			}

			case WID_D_BUILD: // Build vehicle
				ResetObjectToPlace();
				ShowBuildVehicleWindow(this->window_number, this->type);
				break;

			case WID_D_CLONE: // Clone button
				this->SetWidgetDirty(WID_D_CLONE);
				this->ToggleWidgetLoweredState(WID_D_CLONE);

				if (this->IsWidgetLowered(WID_D_CLONE)) {
					static const CursorID clone_icons[] = {
						SPR_CURSOR_CLONE_TRAIN, SPR_CURSOR_CLONE_ROADVEH,
						SPR_CURSOR_CLONE_SHIP, SPR_CURSOR_CLONE_AIRPLANE
					};

					SetObjectToPlaceWnd(clone_icons[this->type], PAL_NONE, HT_VEHICLE, this);
				} else {
					ResetObjectToPlace();
				}
				break;

			case WID_D_LOCATION:
				if (_ctrl_pressed) {
					ShowExtraViewPortWindow(tile);
				} else {
					ScrollMainWindowToTile(tile);
				}
				break;

			case WID_D_RENAME: // Rename button
				SetDParam(0, this->type);
				SetDParam(1, this->window_number);
				ShowQueryString(STR_DEPOT_NAME, STR_DEPOT_RENAME_DEPOT_CAPTION, MAX_LENGTH_DEPOT_NAME_CHARS, this, CS_ALPHANUMERAL, QSF_ENABLE_DEFAULT | QSF_LEN_IN_CHARS);
				break;

			case WID_D_STOP_ALL:
			case WID_D_START_ALL: {
				VehicleListIdentifier vli(VL_DEPOT_LIST, this->type, this->owner);
				DoCommandP(tile, (widget == WID_D_START_ALL ? (1 << 0) : 0), vli.Pack(), CMD_MASS_START_STOP | CMD_MSG(STR_ERROR_CAN_T_START_STOP_VEHICLES));
				break;
			}

			case WID_D_SELL_ALL:
				/* Only open the confirmation window if there are anything to sell */
				if (this->vehicle_list.Length() != 0 || this->wagon_list.Length() != 0) {
					byte vehtype = this->type;

					SetDParam(0, vehtype);
					SetDParam(1, this->window_number);
					ShowQuery(
						STR_DEPOT_CAPTION,
						STR_DEPOT_SELL_CONFIRMATION_TEXT,
						this,
						DepotSellAllConfirmationCallback
					);
				}
				break;

			case WID_D_VEHICLE_LIST:
				ShowVehicleListWindowDep(this->owner, this->type, this->window_number);
				break;

			case WID_D_AUTOREPLACE:
				DoCommandP(tile, this->type, 0, CMD_DEPOT_MASS_AUTOREPLACE);
				break;

		}
	}

	virtual void OnQueryTextFinished(char *str)
	{
		if (str == NULL) return;

		/* Do depot renaming */
		DoCommandP(0, this->window_number, 0, CMD_RENAME_DEPOT | CMD_MSG(STR_ERROR_CAN_T_RENAME_DEPOT), NULL, str);
	}

	virtual bool OnRightClick(Point pt, int widget)
	{
		if (widget != WID_D_MATRIX) return false;

		GetDepotVehiclePtData gdvp = { NULL, NULL };
		const Vehicle *v = NULL;
		NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_D_MATRIX);
		DepotGUIAction mode = this->GetVehicleFromDepotWndPt(pt.x - nwi->pos_x, pt.y - nwi->pos_y, &v, &gdvp);

		if (this->type == VEH_TRAIN) v = gdvp.wagon;

		if (v == NULL || mode != MODE_DRAG_VEHICLE) return false;

		CargoArray capacity, loaded;

		/* Display info for single (articulated) vehicle, or for whole chain starting with selected vehicle */
		bool whole_chain = (this->type == VEH_TRAIN && _ctrl_pressed);

		/* loop through vehicle chain and collect cargoes */
		uint num = 0;
		for (const Vehicle *w = v; w != NULL; w = w->Next()) {
			if (w->cargo_cap > 0 && w->cargo_type < NUM_CARGO) {
				capacity[w->cargo_type] += w->cargo_cap;
				loaded  [w->cargo_type] += w->cargo.StoredCount();
			}

			if (w->type == VEH_TRAIN && !w->HasArticulatedPart()) {
				num++;
				if (!whole_chain) break;
			}
		}

		/* Build tooltipstring */
		static char details[1024];
		details[0] = '\0';
		char *pos = details;

		for (CargoID cargo_type = 0; cargo_type < NUM_CARGO; cargo_type++) {
			if (capacity[cargo_type] == 0) continue;

			SetDParam(0, cargo_type);           // {CARGO} #1
			SetDParam(1, loaded[cargo_type]);   // {CARGO} #2
			SetDParam(2, cargo_type);           // {SHORTCARGO} #1
			SetDParam(3, capacity[cargo_type]); // {SHORTCARGO} #2
			pos = GetString(pos, STR_DEPOT_VEHICLE_TOOLTIP_CARGO, lastof(details));
		}

		/* Show tooltip window */
		uint64 args[2];
		args[0] = (whole_chain ? num : v->engine_type);
		args[1] = (uint64)(size_t)details;
		GuiShowTooltips(this, whole_chain ? STR_DEPOT_VEHICLE_TOOLTIP_CHAIN : STR_DEPOT_VEHICLE_TOOLTIP, 2, args, TCC_RIGHT_CLICK);

		return true;
	}

	/**
	 * Clones a vehicle
	 * @param v the original vehicle to clone
	 * @return Always true.
	 */
	virtual bool OnVehicleSelect(const Vehicle *v)
	{
		if (DoCommandP(this->depot->xy, v->index, _ctrl_pressed ? 1 : 0, CMD_CLONE_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_BUY_TRAIN + v->type), CcCloneVehicle)) {
			ResetObjectToPlace();
		}
		return true;
	}

	virtual void OnPlaceObjectAbort()
	{
		/* abort clone */
		this->RaiseWidget(WID_D_CLONE);
		this->SetWidgetDirty(WID_D_CLONE);

		/* abort drag & drop */
		this->sel = INVALID_VEHICLE;
		this->vehicle_over = INVALID_VEHICLE;
		this->SetWidgetDirty(WID_D_MATRIX);

		if (this->hovered_widget != -1) {
			this->SetWidgetLoweredState(this->hovered_widget, false);
			this->SetWidgetDirty(this->hovered_widget);
			this->hovered_widget = -1;
		}
	}

	virtual void OnMouseDrag(Point pt, int widget)
	{
		if (this->sel == INVALID_VEHICLE) return;
		if (widget != this->hovered_widget) {
			if (this->hovered_widget == WID_D_SELL || this->hovered_widget == WID_D_SELL_CHAIN) {
				this->SetWidgetLoweredState(this->hovered_widget, false);
				this->SetWidgetDirty(this->hovered_widget);
			}
			this->hovered_widget = widget;
			if (this->hovered_widget == WID_D_SELL || this->hovered_widget == WID_D_SELL_CHAIN) {
				this->SetWidgetLoweredState(this->hovered_widget, true);
				this->SetWidgetDirty(this->hovered_widget);
			}
		}
		if (this->type != VEH_TRAIN) return;

		/* A rail vehicle is dragged.. */
		if (widget != WID_D_MATRIX) { // ..outside of the depot matrix.
			if (this->vehicle_over != INVALID_VEHICLE) {
				this->vehicle_over = INVALID_VEHICLE;
				this->SetWidgetDirty(WID_D_MATRIX);
			}
			return;
		}

		NWidgetBase *matrix = this->GetWidget<NWidgetBase>(widget);
		const Vehicle *v = NULL;
		GetDepotVehiclePtData gdvp = {NULL, NULL};

		if (this->GetVehicleFromDepotWndPt(pt.x - matrix->pos_x, pt.y - matrix->pos_y, &v, &gdvp) != MODE_DRAG_VEHICLE) return;

		VehicleID new_vehicle_over = INVALID_VEHICLE;
		if (gdvp.head != NULL) {
			if (gdvp.wagon == NULL && gdvp.head->Last()->index != this->sel) { // ..at the end of the train.
				/* NOTE: As a wagon can't be moved at the begin of a train, head index isn't used to mark a drag-and-drop
				 * destination inside a train. This head index is then used to indicate that a wagon is inserted at
				 * the end of the train.
				 */
				new_vehicle_over = gdvp.head->index;
			} else if (gdvp.wagon != NULL && gdvp.head != gdvp.wagon &&
					gdvp.wagon->index != this->sel &&
					gdvp.wagon->Previous()->index != this->sel) { // ..over an existing wagon.
				new_vehicle_over = gdvp.wagon->index;
			}
		}

		if (this->vehicle_over == new_vehicle_over) return;

		this->vehicle_over = new_vehicle_over;
		this->SetWidgetDirty(widget);
	}

	virtual void OnDragDrop(Point pt, int widget)
	{
		switch (widget) {
			case WID_D_MATRIX: {
				const Vehicle *v = NULL;
				VehicleID sel = this->sel;

				this->sel = INVALID_VEHICLE;
				this->SetDirty();

				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_D_MATRIX);
				if (this->type == VEH_TRAIN) {
					GetDepotVehiclePtData gdvp = { NULL, NULL };

					if (this->GetVehicleFromDepotWndPt(pt.x - nwi->pos_x, pt.y - nwi->pos_y, &v, &gdvp) == MODE_DRAG_VEHICLE && sel != INVALID_VEHICLE) {
						if (gdvp.wagon != NULL && gdvp.wagon->index == sel && _ctrl_pressed) {
							DoCommandP(Vehicle::Get(sel)->tile, Vehicle::Get(sel)->index, true,
									CMD_REVERSE_TRAIN_DIRECTION | CMD_MSG(STR_ERROR_CAN_T_REVERSE_DIRECTION_RAIL_VEHICLE));
						} else if (gdvp.wagon == NULL || gdvp.wagon->index != sel) {
							this->vehicle_over = INVALID_VEHICLE;
							TrainDepotMoveVehicle(gdvp.wagon, sel, gdvp.head);
						} else if (gdvp.head != NULL && gdvp.head->IsFrontEngine()) {
							ShowVehicleViewWindow(gdvp.head);
						}
					}
				} else if (this->GetVehicleFromDepotWndPt(pt.x - nwi->pos_x, pt.y - nwi->pos_y, &v, NULL) == MODE_DRAG_VEHICLE && v != NULL && sel == v->index) {
					ShowVehicleViewWindow(v);
				}
				break;
			}

			case WID_D_SELL: case WID_D_SELL_CHAIN: {
				if (this->IsWidgetDisabled(widget)) return;
				if (this->sel == INVALID_VEHICLE) return;

				this->HandleButtonClick(widget);

				const Vehicle *v = Vehicle::Get(this->sel);
				this->sel = INVALID_VEHICLE;
				this->SetDirty();

				int sell_cmd = (v->type == VEH_TRAIN && (widget == WID_D_SELL_CHAIN || _ctrl_pressed)) ? 1 : 0;
				DoCommandP(v->tile, v->index | sell_cmd << 20 | MAKE_ORDER_BACKUP_FLAG, 0, GetCmdSellVeh(v->type));
				break;
			}

			default:
				this->sel = INVALID_VEHICLE;
				this->SetDirty();
				break;
		}
		this->hovered_widget = -1;
		_cursor.vehchain = false;
	}

	virtual void OnTimeout()
	{
		if (!this->IsWidgetDisabled(WID_D_SELL)) {
			this->RaiseWidget(WID_D_SELL);
			this->SetWidgetDirty(WID_D_SELL);
		}
		if (this->nested_array[WID_D_SELL] != NULL && !this->IsWidgetDisabled(WID_D_SELL_CHAIN)) {
			this->RaiseWidget(WID_D_SELL_CHAIN);
			this->SetWidgetDirty(WID_D_SELL_CHAIN);
		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_D_MATRIX);
		NWidgetCore *nwi = this->GetWidget<NWidgetCore>(WID_D_MATRIX);
		if (this->type == VEH_TRAIN) {
			this->hscroll->SetCapacity(nwi->current_x - this->header_width - this->count_width);
		} else {
			this->num_columns = nwi->current_x / nwi->resize_x;
		}
	}

	virtual EventState OnCTRLStateChange()
	{
		if (this->sel != INVALID_VEHICLE) {
			_cursor.vehchain = _ctrl_pressed;
			this->SetWidgetDirty(WID_D_MATRIX);
			return ES_HANDLED;
		}

		return ES_NOT_HANDLED;
	}
};

static void DepotSellAllConfirmationCallback(Window *win, bool confirmed)
{
	if (confirmed) {
		DepotWindow *w = (DepotWindow*)win;
		DoCommandP(w->depot->xy, w->type, 0, CMD_DEPOT_SELL_ALL_VEHICLES);
	}
}

/**
 * Opens a depot window
 * @param tile The tile where the depot/hangar is located
 * @param type The type of vehicles in the depot
 */
void ShowDepotWindow(TileIndex tile, VehicleType type)
{
	assert(IsDepotTile(tile));
	DepotID depot_id = GetDepotIndex(tile);
	assert(Depot::IsValidID(depot_id));
	if (BringWindowToFrontById(WC_VEHICLE_DEPOT, depot_id) != NULL) return;

	WindowDesc *desc;
	switch (type) {
		default: NOT_REACHED();
		case VEH_TRAIN:    desc = &_train_depot_desc;    break;
		case VEH_ROAD:     desc = &_road_depot_desc;     break;
		case VEH_SHIP:     desc = &_ship_depot_desc;     break;
		case VEH_AIRCRAFT: desc = &_aircraft_depot_desc; break;
	}

	new DepotWindow(desc, depot_id, type);
}

/**
 * Removes the highlight of a vehicle in a depot window
 * @param *v Vehicle to remove all highlights from
 */
void DeleteDepotHighlightOfVehicle(const Vehicle *v)
{
	DepotWindow *w;

	/* If we haven't got any vehicles on the mouse pointer, we haven't got any highlighted in any depots either
	 * If that is the case, we can skip looping though the windows and save time
	 */
	if (_special_mouse_mode != WSM_DRAGDROP) return;
	if (!IsDepotTile(v->tile)) return;

	w = dynamic_cast<DepotWindow*>(FindWindowById(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile)));
	if (w != NULL) {
		if (w->sel == v->index) ResetObjectToPlace();
	}
}

static SmallVector<DepotID, 8> _depots_nearby_list;

/**
 * Add depot on this tile to _depots_nearby_list if it's fully within the
 * depot spread.
 * @param tile Tile just being checked
 * @param user_data Pointer to TileArea context
 * @tparam T the type of station to look for
 */
static bool AddNearbyBigDepot(TileIndex tile, void *user_data)
{
	TileArea *ctx = (TileArea *)user_data;

	/* Check if own station and if we stay within station spread */
	if (!IsBigDepotTile(tile)) return false;
	Depot *dep = Depot::GetByTile(tile);
	if (dep->company != _local_company || _depots_nearby_list.Contains(dep->index)) return false;

	if (dep->BeforeAddTiles(*ctx)) {
		*_depots_nearby_list.Append() = dep->index;
	}

	return false; // We want to include *all* nearby stations
}

/**
 * Circulate around the to-be-built station to find stations we could join.
 * Make sure that only stations are returned where joining wouldn't exceed
 * station spread and are our own station.
 * @param ta Base tile area of the to-be-built station
 * @param distant_join Search for adjacent stations (false) or stations fully
 *                     within station spread
 * @tparam T the type of station to look for
 */
static const Depot *FindBigDepotsNearby(TileArea ta, VehicleType veh_type, bool distant_join)
{
	TileArea ctx = ta;

	_depots_nearby_list.Clear();

	/* Check the inside, to return, if we sit on another big depot */
	Depot *depot;
	TILE_AREA_LOOP(t, ta) {
		if (!IsBigDepotTile(t)) continue;
		depot = Depot::GetByTile(t);
		if (depot->veh_type == veh_type && depot->company == _current_company) return depot;
		//revise owner
	}

	/* Only search tiles where we have a chance to stay within the station spread.
	 * The complete check needs to be done in the callback as we don't know the
	 * extent of the found station, yet. */
	if (distant_join && min(ta.w, ta.h) >= _settings_game.station.station_spread) return NULL;
	uint max_dist = distant_join ? _settings_game.station.station_spread - min(ta.w, ta.h) : 1;

	TileIndex tile = TILE_ADD(ctx.tile, TileOffsByDir(DIR_N));
	CircularTileSearch(&tile, max_dist, ta.w, ta.h, AddNearbyBigDepot, &ctx);

	for (uint i = _depots_nearby_list.Length(); i--;) {
		if (Depot::Get(_depots_nearby_list[i])-> veh_type != veh_type)
			_depots_nearby_list.ErasePreservingOrder(i);
	}

	return NULL;
}

/**
 * Check whether we need to show the depot selection window.
 * @param cmd Command to build the depot.
 * @param ta Tile area of the to-be-built depot.
 * @tparam T the type of depot.
 * @return whether we need to show the depot selection window.
 */
static bool DepotJoinerNeeded(CommandContainer cmd, VehicleType veh_type, TileArea ta)
{
	/* Only show selection when building big depots or if distant join is enabled. */
	if (!HasBit(cmd.p1, 6) || !_settings_game.station.distant_join_stations) return false;

	/* If a window is already opened and we didn't ctrl-click,
	 * return true (i.e. just flash the old window) */
	Window *selection_window = FindWindowById(WC_SELECT_DEPOT, veh_type);
	if (selection_window != NULL) {
		/* Abort current distant-join and start new one */
		delete selection_window;
		UpdateTileSelection();
	}

	/* Only show the popup if we press ctrl. */
	if (!_ctrl_pressed) return false;

	/* Now check if we could build there. */
	if (DoCommand(&cmd, CommandFlagsToDCFlags(GetCommandFlags(cmd.cmd)) | DC_GUI_TEST).Failed()) return false;

	/* Test for adjacent depot or depot below selection.
	 * If adjacent-stations is disabled and we are building next to a depot, do not show the selection window.
	 * but join the other depot immediately. */
	const Depot *depot = FindBigDepotsNearby(ta, veh_type, false);
	return depot == NULL && (_settings_game.station.adjacent_stations || _depots_nearby_list.Length() == 0);
}

/**
 * Window for selecting depots to (distant) join to.
 */
struct SelectDepotWindow : Window {
	CommandContainer select_depot_cmd; ///< Command to build new depot
	TileArea area; ///< Location of new depot
	Scrollbar *vscroll;

	SelectDepotWindow(WindowDesc *desc, CommandContainer cmd, TileArea ta, VehicleType veh_type) :
		Window(desc),
		select_depot_cmd(cmd),
		area(ta)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_JS_SCROLLBAR);
		this->GetWidget<NWidgetCore>(WID_JS_CAPTION)->widget_data = STR_JOIN_DEPOT_CAPTION;
		this->FinishInitNested(veh_type);
		this->OnInvalidateData(0);
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		if (widget != WID_JS_PANEL) return;

		/* Determine the widest string. */
		Dimension d = GetStringBoundingBox(STR_JOIN_DEPOT_CREATE_SPLITTED_DEPOT);
		for (uint i = 0; i < _depots_nearby_list.Length(); i++) {
			assert(Depot::IsValidID(_depots_nearby_list[i]));
			SetDParam(0, this->window_number);
			SetDParam(1, _depots_nearby_list[i]);
			d = maxdim(d, GetStringBoundingBox(STR_DEPOT_LIST_DEPOT));
		}

		resize->height = GetMinSizing(NWST_STEP, d.height);
		d.height = 5 * resize->height;
		d.width += ScaleGUIPixels(WD_FRAMERECT_RIGHT + WD_FRAMERECT_LEFT);
		d.height += ScaleGUIPixels(WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
		*size = d;
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		if (widget != WID_JS_PANEL) return;

		uint y = Center(r.top, this->resize.step_height);
		if (this->vscroll->GetPosition() == 0) {
			DrawString(r.left + SWD_FRAMERECT_LEFT, r.right - SWD_FRAMERECT_RIGHT, y, STR_JOIN_DEPOT_CREATE_SPLITTED_DEPOT);
			y += this->resize.step_height;
		}

		for (uint i = max<uint>(1, this->vscroll->GetPosition()); i <= _depots_nearby_list.Length(); ++i, y += this->resize.step_height) {
			/* Don't draw anything if it extends past the end of the window. */
			if (i - this->vscroll->GetPosition() >= this->vscroll->GetCapacity()) break;

			SetDParam(0, this->window_number);
			SetDParam(1, _depots_nearby_list[i - 1]);
			Depot *depot = Depot::GetIfValid(_depots_nearby_list[i - 1]);
			assert(depot != NULL);
			DrawString(r.left + SWD_FRAMERECT_LEFT, r.right - SWD_FRAMERECT_RIGHT, y, STR_DEPOT_LIST_DEPOT);
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		if (widget != WID_JS_PANEL) return;

		uint st_index = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_JS_PANEL, SWD_FRAMERECT_TOP);
		bool distant_join = (st_index > 0);
		if (distant_join) st_index--;

		if (distant_join && st_index >= _depots_nearby_list.Length()) return;

		/* Insert depot to be joined into stored command. */
		SB(this->select_depot_cmd.p1, 16, 16,
		   (distant_join ? _depots_nearby_list[st_index] : NEW_DEPOT));

		/* Execute stored Command */
		DoCommandP(&this->select_depot_cmd);

		/* Close Window; this might cause double frees! */
		InvalidateWindowData(WC_SELECT_DEPOT, window_number);
		delete this;
	}

	virtual void OnTick()
	{
		if (_thd.dirty & 2) {
			_thd.dirty &= ~2;
			this->SetDirty();
		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_JS_PANEL, ScaleGUIPixels(WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM));
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;
		FindBigDepotsNearby(this->area, (VehicleType)this->window_number, true);
		this->vscroll->SetCount(_depots_nearby_list.Length() + 1);
		this->SetDirty();
	}
};


static const NWidgetPart _nested_select_depot_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_JS_CAPTION), SetDataTip(STR_JOIN_STATION_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_DEFSIZEBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN, WID_JS_PANEL), SetResize(1, 0), SetScrollbar(WID_JS_SCROLLBAR), EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_DARK_GREEN, WID_JS_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_DARK_GREEN),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _select_depot_desc(
	WDP_AUTO, "build_station_join", 200, 180,
	WC_SELECT_DEPOT, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_select_depot_widgets, lengthof(_nested_select_depot_widgets)
);

/**
 * Show the depot selection window when needed. If not, build the depot.
 * @param cmd Command to build the depot.
 * @param ta Area to build the depot in.
 */
void ShowSelectDepotIfNeeded(CommandContainer cmd, TileArea ta, VehicleType veh_type)
{
	if (DepotJoinerNeeded(cmd, veh_type, ta)) {
		if (!_settings_client.gui.persistent_buildingtools) ResetObjectToPlace();
		new SelectDepotWindow(&_select_depot_desc, cmd, ta, veh_type);
	} else {
		TouchCommandP(&cmd);
	}
}
