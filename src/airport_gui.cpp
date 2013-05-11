/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_gui.cpp The GUI for airports. */

#include "stdafx.h"
#include "window_gui.h"
#include "station_gui.h"
#include "terraform_gui.h"
#include "sound_func.h"
#include "window_func.h"
#include "strings_func.h"
#include "viewport_func.h"
#include "company_func.h"
#include "command_func.h"
#include "tilehighlight_func.h"
#include "company_base.h"
#include "station_type.h"
#include "newgrf_airport.h"
#include "newgrf_callbacks.h"
#include "widgets/dropdown_type.h"
#include "core/geometry_func.hpp"
#include "hotkeys.h"
#include "vehicle_func.h"
#include "gui.h"
#include "command_func.h"
#include "air.h"
#include "station_map.h"
#include "pbs_air.h"
#include "engine_base.h"
#include "debug.h"
#include "platform_func.h"

#include "widgets/airport_widget.h"
#include "widgets/road_widget.h"

#include "safeguards.h"


static AirType _cur_airtype;                   ///< Rail type of the current build-rail toolbar.
static DiagDirection _hangar_dir;              ///< Exit direction for new hangars.
static bool _remove_button_clicked;            ///< Flag whether 'remove' toggle-button is currently enabled
static AirportClassID _selected_airport_class; ///< the currently visible airport class
static int _selected_airport_index;            ///< the index of the selected airport in the current class or -1
static byte _selected_airport_layout;          ///< selected airport layout number.

static void ShowBuildAirportPicker(Window *parent);
static void ShowHangarPicker(Window *parent);

SpriteID GetCustomAirportSprite(const AirportSpec *as, byte layout);

void CcBuildAirport(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Failed()) return;

	if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_SPLAT_OTHER, tile);
	if (!_settings_client.gui.persistent_buildingtools) ResetObjectToPlace();
}

/**
 * Place an airport.
 * @param tile Position to put the new airport.
 */
static void PlaceAirport(TileIndex tile)
{
	if (_selected_airport_index == -1) return;
	uint32 p2 = _ctrl_pressed;
	SB(p2, 16, 16, INVALID_STATION); // no station to join

	uint32 p1 = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index)->GetIndex();
	p1 |= _selected_airport_layout << 8;
	CommandContainer cmdcont = { tile, p1, p2, CMD_BUILD_AIRPORT | CMD_MSG(STR_ERROR_CAN_T_BUILD_AIRPORT_HERE), CcBuildAirport, "" };
	ShowSelectStationIfNeeded(cmdcont, TileArea(tile, _thd.size.x / TILE_SIZE, _thd.size.y / TILE_SIZE));
}

/** Airport build toolbar window handler. */
struct BuildAirToolbarWindow : Window {
	const bool allow_by_tile;
	int last_user_action; // Last started user action.

	BuildAirToolbarWindow(bool allow_by_tile, WindowDesc *desc, AirType airtype) : Window(desc), allow_by_tile(allow_by_tile)
	{
		this->InitNested(TRANSPORT_AIR);
		this->SetupAirToolbar(airtype);
		this->DisableWidget(WID_AT_REMOVE);
		this->last_user_action = WIDGET_LIST_END;

		if (_settings_client.gui.link_terraform_toolbar) ShowTerraformToolbar(this);
	}

	~BuildAirToolbarWindow()
	{
		if (_thd.GetCallbackWnd() == this) this->OnPlaceObjectAbort();
		if (_settings_client.gui.link_terraform_toolbar) DeleteWindowById(WC_SCEN_LAND_GEN, 0, false);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;

		if (!CanBuildVehicleInfrastructure(VEH_AIRCRAFT)) delete this;
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == WID_AT_CAPTION) {
			if (_settings_game.station.allow_modify_airports) {
				SetDParam(0, GetAirTypeInfo(_cur_airtype)->strings.toolbar_caption);
			} else {
				SetDParam(0, STR_TOOLBAR_AIRCRAFT_CAPTION);
			}
		}
	}

	/**
	 * Configures the air toolbar for airtype given
	 * @param railtype the railtype to display
	 */
	void SetupAirToolbar(AirType airtype)
	{
		if (!this->allow_by_tile) return;
		_cur_airtype = airtype;
		SetWidgetDisabledState(WID_AT_CONVERT, airtype == AIRTYPE_WATER);
		const AirTypeInfo *ati = GetAirTypeInfo(airtype);

		assert(airtype < AIRTYPE_END);
		const SpriteID *end = &ati->gui_sprites.convert_air;
		uint widget = WID_AT_BUILD_TILE;
		for (const SpriteID *iter = &ati->gui_sprites.build_infrastructure; iter <= end; iter++, widget++) {
			if (!HasWidget(widget)) continue;
			this->GetWidget<NWidgetCore>(widget)->widget_data = *iter;
		}

/*
		assert(railtype < RAILTYPE_END);
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_NS)->widget_data     = rti->gui_sprites.build_ns_rail;
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_X)->widget_data      = rti->gui_sprites.build_x_rail;
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_EW)->widget_data     = rti->gui_sprites.build_ew_rail;
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_Y)->widget_data      = rti->gui_sprites.build_y_rail;
		this->GetWidget<NWidgetCore>(WID_RAT_AUTORAIL)->widget_data     = rti->gui_sprites.auto_rail;
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_DEPOT)->widget_data  = rti->gui_sprites.build_depot;
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_BIG_DEPOT)->widget_data  = rti->gui_sprites.build_depot;
		this->GetWidget<NWidgetCore>(WID_RAT_CONVERT_RAIL)->widget_data = rti->gui_sprites.convert_rail;
		this->GetWidget<NWidgetCore>(WID_RAT_BUILD_TUNNEL)->widget_data = rti->gui_sprites.build_tunnel;
*/
	}

	/**
	 * Switch to another air type.
	 * @param railtype New air type.
	 */
	void ModifyAirType(AirType airtype)
	{
		this->SetupAirToolbar(airtype);
		this->ReInit();
	}

	/**
	* The "remove"-button click proc of the build-air toolbar.
	* @see BuildAirToolbarWindow::OnClick()
	*/
	void BuildAirClick_Remove()
	{
		if (this->IsWidgetDisabled(WID_AT_REMOVE)) return;
		DeleteWindowById(WC_SELECT_STATION, 0);
		this->ToggleWidgetLoweredState(WID_AT_REMOVE);
		this->SetWidgetDirty(WID_AT_REMOVE);
		_remove_button_clicked = this->IsWidgetLowered(WID_AT_REMOVE);

		if (this->last_user_action == WID_AT_RUNWAY_LANDING ||
				this->last_user_action == WID_AT_RUNWAY_NO_LANDING) {
			SetObjectToPlace(SPR_CURSOR_ROAD_DEPOT, PAL_NONE, _remove_button_clicked ? HT_SPECIAL : HT_RECT, this->window_class, this->window_number);
			this->LowerWidget(this->last_user_action);
			this->SetWidgetLoweredState(WID_AT_REMOVE, _remove_button_clicked);
		}

		SetSelectionRed(_remove_button_clicked);
		if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
	}

	void UpdateRemoveWidgetStatus(int clicked_widget)
	{
		if (!this->allow_by_tile) return;

		assert(clicked_widget != WID_AT_REMOVE);

		if (clicked_widget >= WID_AT_REMOVE_FIRST && clicked_widget <= WID_AT_REMOVE_LAST) {
			bool is_button_lowered = this->IsWidgetLowered(clicked_widget);
			_remove_button_clicked &= is_button_lowered;
			this->SetWidgetDisabledState(WID_AT_REMOVE, !is_button_lowered);
			this->SetWidgetLoweredState(WID_AT_REMOVE, _remove_button_clicked);
			SetSelectionRed(_remove_button_clicked);
		} else {
			/* When any other buttons that do not accept "removal",
			 * raise and disable the removal button. */
			this->DisableWidget(WID_AT_REMOVE);
			this->RaiseWidget(WID_AT_REMOVE);
			_remove_button_clicked = false;
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_AT_BUILD_TILE:
				HandlePlacePushButton(this, widget, SPR_CURSOR_AIRPORT, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_AT_TRACKS:
				HandlePlacePushButton(this, widget, SPR_CURSOR_AIRPORT, HT_RAIL);
				this->last_user_action = widget;
				break;

			case WID_AT_REMOVE:
				this->BuildAirClick_Remove();
				return;

			case WID_AT_PRE_AIRPORT:
				if (HandlePlacePushButton(this, widget, SPR_CURSOR_AIRPORT, HT_RECT)) {
					ShowBuildAirportPicker(this);
					this->last_user_action = widget;
				}
				break;

			case WID_AT_DEMOLISH:
				HandlePlacePushButton(this, widget, ANIMCURSOR_DEMOLISH, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_AT_CONVERT:
				HandlePlacePushButton(this, widget, SPR_CURSOR_ROAD_DEPOT, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_AT_INFRASTRUCTURE_CATCH:
			case WID_AT_INFRASTRUCTURE_NO_CATCH:
			case WID_AT_TERMINAL:
			case WID_AT_HELIPAD:
			case WID_AT_HELIPORT:
				HandlePlacePushButton(this, widget, SPR_CURSOR_ROAD_DEPOT, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_AT_HANGAR_SMALL:
			case WID_AT_HANGAR_BIG:
				if (HandlePlacePushButton(this, widget, SPR_CURSOR_ROAD_DEPOT, HT_RECT)) {
					ShowHangarPicker(this);
				}
				this->last_user_action = widget;
				break;

			case WID_AT_RUNWAY_LANDING:
			case WID_AT_RUNWAY_NO_LANDING:
				HandlePlacePushButton(this, widget, SPR_CURSOR_ROAD_DEPOT, _remove_button_clicked ? HT_SPECIAL : HT_RECT);
				this->last_user_action = widget;
				break;

			default: break;
		}

		UpdateRemoveWidgetStatus(widget);
	}

	uint32 FillInParamP2()
	{
		uint32 p2 = 0;
		AirportTileType airport_tile_type;
		bool allow_ctrl_key = true; // Whether diagonal area is allowed.
		bool second_bit = false; // Used to indicate big hangars,
				// infrastructure with catchment,
				// runway allowing landing.

		switch (this->last_user_action) {
			default:
			case WID_AT_BUILD_TILE:
				airport_tile_type = ATT_SIMPLE_TRACK;
				break;
			case WID_AT_INFRASTRUCTURE_CATCH:
				second_bit = true;
			case WID_AT_INFRASTRUCTURE_NO_CATCH:
				airport_tile_type = ATT_INFRASTRUCTURE;
				break;
			case WID_AT_TRACKS:
				airport_tile_type = ATT_SIMPLE_TRACK;
				break;
			case WID_AT_RUNWAY_LANDING:
				second_bit = true;
			case WID_AT_RUNWAY_NO_LANDING:
				allow_ctrl_key = false;
				airport_tile_type = ATT_RUNWAY_START;
				break;
			case WID_AT_TERMINAL:
			case WID_AT_HELIPAD:
			case WID_AT_HELIPORT:
				airport_tile_type = ATT_TERMINAL;
				break;
			case WID_AT_HANGAR_BIG:
				second_bit = true;
			case WID_AT_HANGAR_SMALL:
				allow_ctrl_key = false;
				airport_tile_type = ATT_HANGAR;
				break;
		}

		SB(p2,  0,  1, !_remove_button_clicked);
		SB(p2,  1,  1, _ctrl_pressed && allow_ctrl_key);
		SB(p2,  2,  1, second_bit);

		if (airport_tile_type == ATT_TERMINAL) {
			SB(p2,  4,  3, this->last_user_action - WID_AT_TERMINAL);
		} else {
			SB(p2,  4,  3, _thd.drawstyle & HT_DIR_MASK);
		}

		SB(p2,  7,  2, _hangar_dir);
		SB(p2,  9,  4, _cur_airtype);

		SB(p2, 13,  3, airport_tile_type);
		SB(p2, 16, 16, INVALID_STATION); // no station to join
		return p2;
	}

	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		EraseQueuedTouchCommand();

		switch (this->last_user_action) {
			case WID_AT_BUILD_TILE:
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_BUILD_STATION);
				break;

			case WID_AT_TRACKS:
				VpStartPlaceSizing(tile, VPM_RAILDIRS, DDSP_PLACE_RAIL);
				break;

			case WID_AT_PRE_AIRPORT: {
				VpStartPlaceSizing(tile, VPM_SINGLE_TILE, DDSP_BUILD_STATION);
				break;
			}
 
			case WID_AT_DEMOLISH:
				PlaceProc_DemolishArea(tile);
				break;

			case WID_AT_CONVERT:
				VpStartPlaceSizing(tile, VPM_SINGLE_TILE, DDSP_BUILD_STATION);
				break;

			case WID_AT_HANGAR_SMALL:
			case WID_AT_HANGAR_BIG:
				VpStartPlaceSizing(tile, HasBit(_hangar_dir, 0) ? VPM_FIX_Y : VPM_FIX_X, DDSP_BUILD_STATION);
				break;

			case WID_AT_INFRASTRUCTURE_CATCH:
			case WID_AT_INFRASTRUCTURE_NO_CATCH:
			case WID_AT_TERMINAL:
			case WID_AT_HELIPAD:
			case WID_AT_HELIPORT:
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_BUILD_STATION);
				break;

			case WID_AT_RUNWAY_LANDING:
			case WID_AT_RUNWAY_NO_LANDING:
				VpStartPlaceSizing(tile, _remove_button_clicked ? VPM_SINGLE_TILE : VPM_X_OR_Y, DDSP_BUILD_STATION);
				break;

			default: NOT_REACHED();
		}
	}

	virtual void OnPlaceDrag(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt)
	{
		if ((this->last_user_action == WID_AT_RUNWAY_LANDING ||
				this->last_user_action == WID_AT_RUNWAY_NO_LANDING) &&
				_remove_button_clicked) {
			this->OnPlacePresize(pt, TileVirtXY(pt.x, pt.y));
		} else {
			VpSelectTilesWithMethod(pt.x, pt.y, select_method);
		}
	}

	virtual void OnPlaceMouseUp(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt, TileIndex start_tile, TileIndex end_tile)
	{
		if (pt.x == -1) return;

		uint32 p2 = this->FillInParamP2();

		switch (this->last_user_action) {
			case WID_AT_BUILD_TILE: {
				CommandContainer cmdcont = { start_tile, end_tile, p2, CMD_ADD_REM_AIRPORT | CMD_MSG(STR_ERROR_CAN_T_DO_THIS), NULL, ""};

				ShowSelectStationIfNeeded(cmdcont, TileArea(start_tile, end_tile));
				break;
			}
			case WID_AT_TRACKS:
				TouchCommandP(start_tile, end_tile, p2, CMD_ADD_REM_TRACKS | CMD_MSG(STR_ERROR_CAN_T_DO_THIS));
				break;
			case WID_AT_PRE_AIRPORT:
				assert(start_tile == end_tile);
				PlaceAirport(end_tile);
				break;
			case WID_AT_DEMOLISH:
				GUIPlaceProcDragXY(select_proc, start_tile, end_tile);
				break;
			case WID_AT_CONVERT:
				assert(start_tile == end_tile);
				TouchCommandP(start_tile, 0, p2, CMD_CONVERT_AIRPORT | CMD_MSG(STR_ERROR_CAN_T_DO_THIS));
				break;
			case WID_AT_INFRASTRUCTURE_CATCH:
			case WID_AT_INFRASTRUCTURE_NO_CATCH:
			case WID_AT_TERMINAL:
			case WID_AT_HELIPAD:
			case WID_AT_HELIPORT:
			case WID_AT_HANGAR_SMALL:
			case WID_AT_HANGAR_BIG:
			case WID_AT_RUNWAY_LANDING:
			case WID_AT_RUNWAY_NO_LANDING:
				TouchCommandP(start_tile, end_tile, p2, CMD_CHANGE_AIRPORT | CMD_MSG(STR_ERROR_CAN_T_DO_THIS));
				if (_remove_button_clicked &&
					(this->last_user_action == WID_AT_RUNWAY_LANDING || this->last_user_action == WID_AT_RUNWAY_NO_LANDING)) {
					VpStartPreSizing();
				}
				break;
			default: NOT_REACHED();
		}
	}
	
	virtual void OnPlacePresize(Point pt, TileIndex tile_from)
	{
		assert(this->last_user_action == WID_AT_RUNWAY_LANDING ||
				this->last_user_action == WID_AT_RUNWAY_NO_LANDING);
		assert(_remove_button_clicked);
		if (!IsValidTile(tile_from) || !IsAirportTile(tile_from) ||
				!IsRunwayExtreme(tile_from) || (this->last_user_action == WID_AT_RUNWAY_LANDING) != IsLandingTypeTile(tile_from)) {
			VpSetPresizeRange(tile_from, tile_from);
			return;
		}

		VpSetPresizeRange(GetStartPlatformTile(tile_from), GetOtherStartPlatformTile(tile_from));
	}

	virtual void OnPlaceObjectAbort()
	{
		this->RaiseButtons();

		DeleteWindowById(WC_BUILD_DEPOT, TRANSPORT_AIR);
		DeleteWindowById(WC_BUILD_STATION, TRANSPORT_AIR);
		DeleteWindowById(WC_SELECT_STATION, 0);
		EraseQueuedTouchCommand();
		ResetObjectToPlace();
	}

	static HotkeyList hotkeys;
};

Window *ShowBuildAirToolbar(AirType airtype);

/**
 * Handler for global hotkeys of the BuildAirToolbarWindow.
 * @param hotkey Hotkey
 * @return ES_HANDLED if hotkey was accepted.
 */
static EventState AirportToolbarGlobalHotkeys(int hotkey)
{
	if (_game_mode != GM_NORMAL || !CanBuildVehicleInfrastructure(VEH_AIRCRAFT)) return ES_NOT_HANDLED;
	extern AirType _last_built_airtype;
	Window *w = ShowBuildAirToolbar(_settings_game.station.allow_modify_airports ? _last_built_airtype : INVALID_AIRTYPE);
	if (w == NULL) return ES_NOT_HANDLED;
	return w->OnHotkey(hotkey);
}

static Hotkey airtoolbar_hotkeys[] = {
	Hotkey('1', "airport", WID_AT_PRE_AIRPORT),
	Hotkey('2', "demolish", WID_AT_DEMOLISH),
	HOTKEY_LIST_END
};

HotkeyList BuildAirToolbarWindow::hotkeys("airtoolbar", airtoolbar_hotkeys, AirportToolbarGlobalHotkeys);

/**
 * Add the depot icons depending on availability of construction.
 * @param biggest_index Storage for collecting the biggest index used in the returned tree.
 * @return Panel with company buttons.
 * @post \c *biggest_index contains the largest used index in the tree.
 */
static NWidgetBase *MakeNWidgetHangars(int *biggest_index)
{
	NWidgetHorizontal *hor = new NWidgetHorizontal();

	if (HasBit(_settings_game.depot.hangar_types, 0)) {
		// small depot
		hor->Add(new NWidgetLeaf(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HANGAR_SMALL, SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_BUILD_HANGAR_SMALL));
	}

	if (HasBit(_settings_game.depot.hangar_types, 1)) {
		// big depot
		hor->Add(new NWidgetLeaf(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HANGAR_BIG, SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_BUILD_HANGAR_BIG));
	}

	*biggest_index = WID_AT_HANGAR_BIG;
	return hor;
}

static const NWidgetPart _nested_air_tile_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_AT_CAPTION), SetDataTip(STR_WHITE_STRING, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_VERTICAL),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_BUILD_TILE), SetFill(0, 1), SetMinimalSize(42, 22), SetDataTip(SPR_IMG_AIRPORT, STR_TOOLBAR_AIRCRAFT_ADD_TILES),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_TRACKS), SetFill(0, 1), SetMinimalSize(42, 22), SetDataTip(SPR_IMG_AIRPORT, STR_TOOLBAR_AIRCRAFT_SET_TRACKS),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_REMOVE), SetFill(0, 1),
					SetDataTip(SPR_IMG_REMOVE, STR_AIRPORT_TOOLBAR_TOOLTIP_TOGGLE_BUILD_REMOVE_FOR),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_CONVERT), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_CHANGE_AIRTYPE),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_PRE_AIRPORT), SetFill(0, 1), SetMinimalSize(42, 22),
					SetDataTip(SPR_IMG_AIRPORT, STR_TOOLBAR_AIRCRAFT_BUILD_PRE_AIRPORT_TOOLTIP),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_DEMOLISH), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_INFRASTRUCTURE_CATCH), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_INFRASTRUCTURE_CATCHMENT),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_INFRASTRUCTURE_NO_CATCH), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_INFRASTRUCTURE_NO_CATCHMENT),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_RUNWAY_LANDING), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_DEFINE_RUNWAY_LANDING),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_RUNWAY_NO_LANDING), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_DEFINE_RUNWAY_NO_LANDING),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_TERMINAL), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_BUILD_TERMINAL),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HELIPAD), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_BUILD_HELIPAD),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HELIPORT), SetFill(0, 1),
					SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLBAR_AIRCRAFT_BUILD_HELIPORT),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),
			NWidgetFunction(MakeNWidgetHangars),
		EndContainer(),
	EndContainer(),
};

static const NWidgetPart _nested_air_nontile_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_AT_CAPTION), SetDataTip(STR_BLACK_STRING, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_PRE_AIRPORT), SetFill(0, 1), SetMinimalSize(42, 22),
				SetDataTip(SPR_IMG_AIRPORT, STR_TOOLBAR_AIRCRAFT_BUILD_PRE_AIRPORT_TOOLTIP),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_DEMOLISH), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
	EndContainer(),
};

static WindowDesc _air_tile_toolbar_desc(
	WDP_ALIGN_TOOLBAR, "toolbar_air", 0, 0,
	WC_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_air_tile_toolbar_widgets, lengthof(_nested_air_tile_toolbar_widgets),
	&BuildAirToolbarWindow::hotkeys
);

static WindowDesc _air_nontile_toolbar_desc(
	WDP_ALIGN_TOOLBAR, "toolbar_air_nontile", 0, 0,
	WC_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_air_nontile_toolbar_widgets, lengthof(_nested_air_nontile_toolbar_widgets)
);

/**
 * Open the build airport toolbar window
 * If the terraform toolbar is linked to the toolbar, that window is also opened.
 * @param airtype air type for constructing (a valid air type or
 * 			INVALID_AIRTYPE if the build-airports-by-tile is dissabled).
 * @return newly opened airport toolbar, or NULL if the toolbar could not be opened.
 */
Window *ShowBuildAirToolbar(AirType airtype)
{
	if (!Company::IsValidID(_local_company)) return NULL;

	DeleteToolbarLinkedWindows();

	if (airtype != INVALID_AIRTYPE) {
		// check setting
		assert(_settings_game.station.allow_modify_airports);
	}

	_cur_airtype = airtype;
	_remove_button_clicked = false;

	if (airtype == INVALID_AIRTYPE) {
		return new BuildAirToolbarWindow(false, &_air_nontile_toolbar_desc, airtype);
	} else {
		return new BuildAirToolbarWindow(true, &_air_tile_toolbar_desc, airtype);
	}
}

class BuildAirportWindow : public PickerWindowBase {
	SpriteID preview_sprite; ///< Cached airport preview sprite.
	int line_height;
	Scrollbar *vscroll;

	/** Build a dropdown list of available airport classes */
	static DropDownList *BuildAirportClassDropDown()
	{
		DropDownList *list = new DropDownList();

		for (uint i = 0; i < AirportClass::GetClassCount(); i++) {
			*list->Append() = new DropDownListStringItem(AirportClass::Get((AirportClassID)i)->name, i, false);
		}

		return list;
	}

public:
	BuildAirportWindow(WindowDesc *desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->CreateNestedTree();

		this->vscroll = this->GetScrollbar(WID_AP_SCROLLBAR);
		this->vscroll->SetCapacity(5);
		this->vscroll->SetPosition(0);

		this->FinishInitNested(TRANSPORT_AIR);

		this->SetWidgetLoweredState(WID_AP_BTN_DONTHILIGHT, !_settings_client.gui.station_show_coverage);
		this->SetWidgetLoweredState(WID_AP_BTN_DOHILIGHT, _settings_client.gui.station_show_coverage);
		this->OnInvalidateData();

		/* Ensure airport class is valid (changing NewGRFs). */
		_selected_airport_class = Clamp(_selected_airport_class, APC_BEGIN, (AirportClassID)(AirportClass::GetClassCount() - 1));
		const AirportClass *ac = AirportClass::Get(_selected_airport_class);
		this->vscroll->SetCount(ac->GetSpecCount());

		/* Ensure the airport index is valid for this class (changing NewGRFs). */
		_selected_airport_index = Clamp(_selected_airport_index, -1, ac->GetSpecCount() - 1);

		/* Only when no valid airport was selected, we want to select the first airport. */
		bool selectFirstAirport = true;
		if (_selected_airport_index != -1) {
			const AirportSpec *as = ac->GetSpec(_selected_airport_index);
			if (as->IsAvailable()) {
				/* Ensure the airport layout is valid. */
				_selected_airport_layout = Clamp(_selected_airport_layout, 0, as->num_table - 1);
				selectFirstAirport = false;
				this->UpdateSelectSize();
			}
		}

		if (selectFirstAirport) this->SelectFirstAvailableAirport(true);
	}

	virtual ~BuildAirportWindow()
	{
		EraseQueuedTouchCommand();
		DeleteWindowById(WC_SELECT_STATION, 0);
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_AP_CLASS_DROPDOWN:
				SetDParam(0, AirportClass::Get(_selected_airport_class)->name);
				break;

			case WID_AP_LAYOUT_NUM:
				SetDParam(0, STR_EMPTY);
				if (_selected_airport_index != -1) {
					const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
					StringID string = GetAirportTextCallback(as, _selected_airport_layout, CBID_AIRPORT_LAYOUT_NAME);
					if (string != STR_UNDEFINED) {
						SetDParam(0, string);
					} else if (as->num_table > 1) {
						SetDParam(0, STR_STATION_BUILD_AIRPORT_LAYOUT_NAME);
						SetDParam(1, _selected_airport_layout + 1);
					}
				}
				break;

			default: break;
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_AP_CLASS_DROPDOWN: {
				Dimension d = {0, 0};
				for (uint i = 0; i < AirportClass::GetClassCount(); i++) {
					SetDParam(0, AirportClass::Get((AirportClassID)i)->name);
					d = maxdim(d, GetStringBoundingBox(STR_BLACK_STRING));
				}
				d.width += padding.width;
				d.height += padding.height;
				*size = maxdim(*size, d);
				size->height = GetMinSizing(NWST_STEP, size->height);
				break;
			}

			case WID_AP_AIRPORT_LIST: {
				for (int i = 0; i < NUM_AIRPORTS; i++) {
					const AirportSpec *as = AirportSpec::Get(i);
					if (!as->enabled) continue;

					size->width = max(size->width, GetStringBoundingBox(as->name).width);
				}

				this->line_height = GetMinSizing(NWST_STEP, FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM);
				size->height = 5 * this->line_height;
				break;
			}

			case WID_AP_AIRPORT_SPRITE:
				for (int i = 0; i < NUM_AIRPORTS; i++) {
					const AirportSpec *as = AirportSpec::Get(i);
					if (!as->enabled) continue;
					for (byte layout = 0; layout < as->num_table; layout++) {
						SpriteID sprite = GetCustomAirportSprite(as, layout);
						if (sprite != 0) {
							Dimension d = GetSpriteSize(sprite);
							d.width += WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
							d.height += WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
							*size = maxdim(d, *size);
						}
					}
				}
				break;

			case WID_AP_EXTRA_TEXT:
				for (int i = NEW_AIRPORT_OFFSET; i < NUM_AIRPORTS; i++) {
					const AirportSpec *as = AirportSpec::Get(i);
					if (!as->enabled) continue;
					for (byte layout = 0; layout < as->num_table; layout++) {
						StringID string = GetAirportTextCallback(as, layout, CBID_AIRPORT_ADDITIONAL_TEXT);
						if (string == STR_UNDEFINED) continue;

						/* STR_BLACK_STRING is used to start the string with {BLACK} */
						SetDParam(0, string);
						Dimension d = GetStringMultiLineBoundingBox(STR_BLACK_STRING, *size);
						*size = maxdim(d, *size);
					}
				}
				break;

			default: break;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		switch (widget) {
			case WID_AP_AIRPORT_LIST: {
				int y = r.top;
				AirportClass *apclass = AirportClass::Get(_selected_airport_class);
				for (uint i = this->vscroll->GetPosition(); this->vscroll->IsVisible(i) && i < apclass->GetSpecCount(); i++) {
					const AirportSpec *as = apclass->GetSpec(i);
					if (!as->IsAvailable()) {
						GfxFillRect(r.left + 1, y + 1, r.right - 1, y + this->line_height - 2, PC_BLACK, FILLRECT_CHECKER);
					}

					DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, Center(y, this->line_height), as->name, ((int)i == _selected_airport_index) ? TC_WHITE : TC_BLACK);

					y += this->line_height;
				}
				break;
			}

			case WID_AP_AIRPORT_SPRITE:
				if (this->preview_sprite != 0) {
					Dimension d = GetSpriteSize(this->preview_sprite);
					DrawSprite(this->preview_sprite, COMPANY_SPRITE_COLOUR(_local_company), (r.left + r.right - d.width) / 2, (r.top + r.bottom - d.height) / 2);
				}
				break;

			case WID_AP_EXTRA_TEXT:
				if (_selected_airport_index != -1) {
					const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
					StringID string = GetAirportTextCallback(as, _selected_airport_layout, CBID_AIRPORT_ADDITIONAL_TEXT);
					if (string != STR_UNDEFINED) {
						SetDParam(0, string);
						DrawStringMultiLine(r.left, r.right, r.top, r.bottom, STR_BLACK_STRING);
					}
				}
				break;
		}
	}

	virtual void OnPaint()
	{
		this->DrawWidgets();

		uint16 top = this->GetWidget<NWidgetBase>(WID_AP_BTN_DOHILIGHT)->pos_y + this->GetWidget<NWidgetBase>(WID_AP_BTN_DOHILIGHT)->current_y + WD_PAR_VSEP_NORMAL;
		NWidgetBase *panel_nwi = this->GetWidget<NWidgetBase>(WID_AP_BOTTOMPANEL);

		int right = panel_nwi->pos_x +  panel_nwi->current_x;
		int bottom = panel_nwi->pos_y +  panel_nwi->current_y;

		if (_selected_airport_index != -1) {
			const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
			int rad = _settings_game.station.modified_catchment ? as->catchment : (uint)CA_UNMODIFIED;

			/* only show the station (airport) noise, if the noise option is activated */
			if (_settings_game.economy.station_noise_level) {
				/* show the noise of the selected airport */
				SetDParam(0, as->noise_level);
				DrawString(panel_nwi->pos_x + WD_FRAMERECT_LEFT, right - WD_FRAMERECT_RIGHT, top, STR_STATION_BUILD_NOISE);
				top += FONT_HEIGHT_NORMAL + WD_PAR_VSEP_NORMAL;
			}

			/* strings such as 'Size' and 'Coverage Area' */
			top = DrawStationCoverageAreaText(panel_nwi->pos_x + WD_FRAMERECT_LEFT, right - WD_FRAMERECT_RIGHT, top, SCT_ALL, rad, false) + WD_PAR_VSEP_NORMAL;
			top = DrawStationCoverageAreaText(panel_nwi->pos_x + WD_FRAMERECT_LEFT, right - WD_FRAMERECT_RIGHT, top, SCT_ALL, rad, true) + WD_PAR_VSEP_NORMAL;
		}

		/* Resize background if the window is too small.
		 * Never make the window smaller to avoid oscillating if the size change affects the acceptance.
		 * (This is the case, if making the window bigger moves the mouse into the window.) */
		if (top > bottom) {
			ResizeWindow(this, 0, top - bottom, false);
		}
	}

	void SelectOtherAirport(int airport_index)
	{
		_selected_airport_index = airport_index;
		_selected_airport_layout = 0;

		this->UpdateSelectSize();
		this->SetDirty();
	}

	void UpdateSelectSize()
	{
		EraseQueuedTouchCommand();

		if (_selected_airport_index == -1) {
			SetTileSelectSize(1, 1);
			this->DisableWidget(WID_AP_LAYOUT_DECREASE);
			this->DisableWidget(WID_AP_LAYOUT_INCREASE);
		} else {
			const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
			int w = as->size_x;
			int h = as->size_y;
			Direction rotation = as->rotation[_selected_airport_layout];
			if (rotation == DIR_E || rotation == DIR_W) Swap(w, h);
			SetTileSelectSize(w, h);

			this->preview_sprite = GetCustomAirportSprite(as, _selected_airport_layout);

			this->SetWidgetDisabledState(WID_AP_LAYOUT_DECREASE, _selected_airport_layout == 0);
			this->SetWidgetDisabledState(WID_AP_LAYOUT_INCREASE, _selected_airport_layout + 1 >= as->num_table);

			int rad = _settings_game.station.modified_catchment ? as->catchment : (uint)CA_UNMODIFIED;
			if (_settings_client.gui.station_show_coverage) SetTileSelectBigSize(-rad, -rad, 2 * rad, 2 * rad);
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_AP_CLASS_DROPDOWN:
				ShowDropDownList(this, BuildAirportClassDropDown(), _selected_airport_class, WID_AP_CLASS_DROPDOWN);
				break;

			case WID_AP_AIRPORT_LIST: {
				int num_clicked = this->vscroll->GetPosition() + (pt.y - this->nested_array[widget]->pos_y) / this->line_height;
				if (num_clicked >= this->vscroll->GetCount()) break;
				const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(num_clicked);
				if (as->IsAvailable()) this->SelectOtherAirport(num_clicked);
				break;
			}

			case WID_AP_BTN_DONTHILIGHT: case WID_AP_BTN_DOHILIGHT:
				_settings_client.gui.station_show_coverage = (widget != WID_AP_BTN_DONTHILIGHT);
				this->SetWidgetLoweredState(WID_AP_BTN_DONTHILIGHT, !_settings_client.gui.station_show_coverage);
				this->SetWidgetLoweredState(WID_AP_BTN_DOHILIGHT, _settings_client.gui.station_show_coverage);
				this->SetDirty();
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->UpdateSelectSize();
				break;

			case WID_AP_LAYOUT_DECREASE:
				_selected_airport_layout--;
				this->UpdateSelectSize();
				this->SetDirty();
				break;

			case WID_AP_LAYOUT_INCREASE:
				_selected_airport_layout++;
				this->UpdateSelectSize();
				this->SetDirty();
				break;
		}
	}

	/**
	 * Select the first available airport.
	 * @param change_class If true, change the class if no airport in the current
	 *   class is available.
	 */
	void SelectFirstAvailableAirport(bool change_class)
	{
		/* First try to select an airport in the selected class. */
		AirportClass *sel_apclass = AirportClass::Get(_selected_airport_class);
		for (uint i = 0; i < sel_apclass->GetSpecCount(); i++) {
			const AirportSpec *as = sel_apclass->GetSpec(i);
			if (as->IsAvailable()) {
				this->SelectOtherAirport(i);
				return;
			}
		}
		if (change_class) {
			/* If that fails, select the first available airport
			 * from a random class. */
			for (AirportClassID j = APC_BEGIN; j < APC_MAX; j++) {
				AirportClass *apclass = AirportClass::Get(j);
				for (uint i = 0; i < apclass->GetSpecCount(); i++) {
					const AirportSpec *as = apclass->GetSpec(i);
					if (as->IsAvailable()) {
						_selected_airport_class = j;
						this->SelectOtherAirport(i);
						return;
					}
				}
			}
		}
		/* If all airports are unavailable, select nothing. */
		this->SelectOtherAirport(-1);
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		assert(widget == WID_AP_CLASS_DROPDOWN);
		_selected_airport_class = (AirportClassID)index;
		this->vscroll->SetCount(AirportClass::Get(_selected_airport_class)->GetSpecCount());
		this->SelectFirstAvailableAirport(false);
	}

	virtual void OnTick()
	{
		CheckRedrawStationCoverage(this);
	}
};

static const NWidgetPart _nested_build_airport_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_STATION_BUILD_AIRPORT_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetFill(1, 0), SetPIP(2, 0, 2),
		NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetSizingType(NWST_STEP), SetDataTip(STR_STATION_BUILD_AIRPORT_CLASS_LABEL, STR_NULL), SetFill(1, 0),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_AP_CLASS_DROPDOWN), SetFill(1, 0), SetDataTip(STR_BLACK_STRING, STR_STATION_BUILD_AIRPORT_TOOLTIP),
		NWidget(WWT_EMPTY, COLOUR_DARK_GREEN, WID_AP_AIRPORT_SPRITE), SetFill(1, 0),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_MATRIX, COLOUR_GREY, WID_AP_AIRPORT_LIST), SetFill(1, 0), SetMatrixDataTip(1, 5, STR_STATION_BUILD_AIRPORT_TOOLTIP), SetScrollbar(WID_AP_SCROLLBAR),
			NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_AP_SCROLLBAR),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_AP_LAYOUT_DECREASE), SetSizingType(NWST_STEP), SetMinimalSize(12, 0), SetDataTip(AWV_DECREASE, STR_NULL),
			NWidget(WWT_LABEL, COLOUR_GREY, WID_AP_LAYOUT_NUM), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_BLACK_STRING, STR_NULL),
			NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_AP_LAYOUT_INCREASE), SetSizingType(NWST_STEP), SetMinimalSize(12, 0), SetDataTip(AWV_INCREASE, STR_NULL),
		EndContainer(),
		NWidget(WWT_EMPTY, COLOUR_DARK_GREEN, WID_AP_EXTRA_TEXT), SetFill(1, 0), SetMinimalSize(150, 0),
	EndContainer(),
	/* Bottom panel. */
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN, WID_AP_BOTTOMPANEL), SetPIP(2, 2, 2),
		NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetDataTip(STR_STATION_BUILD_COVERAGE_AREA_TITLE, STR_NULL), SetFill(1, 0),
		NWidget(NWID_HORIZONTAL),
			NWidget(NWID_SPACER), SetMinimalSize(14, 0), SetFill(1, 0),
			NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_AP_BTN_DONTHILIGHT), SetMinimalSize(60, 12), SetFill(1, 0),
				SetDataTip(STR_STATION_BUILD_COVERAGE_OFF, STR_STATION_BUILD_COVERAGE_AREA_OFF_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_AP_BTN_DOHILIGHT), SetMinimalSize(60, 12), SetFill(1, 0),
				SetDataTip(STR_STATION_BUILD_COVERAGE_ON, STR_STATION_BUILD_COVERAGE_AREA_ON_TOOLTIP),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(14, 0), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 10), SetResize(0, 1), SetFill(1, 0),
	EndContainer(),
};

static WindowDesc _build_airport_desc(
	WDP_AUTO, "build_station_air", 0, 0,
	WC_BUILD_STATION, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_airport_widgets, lengthof(_nested_build_airport_widgets)
);

static void ShowBuildAirportPicker(Window *parent)
{
	new BuildAirportWindow(&_build_airport_desc, parent);
}

struct BuildHangarWindow : public PickerWindowBase {
	BuildHangarWindow(WindowDesc *desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->CreateNestedTree();
		this->LowerWidget(_hangar_dir + WID_BROD_DEPOT_NE);
		this->FinishInitNested(TRANSPORT_AIR);
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_BROD_DEPOT_NW:
			case WID_BROD_DEPOT_NE:
			case WID_BROD_DEPOT_SW:
			case WID_BROD_DEPOT_SE:
				this->RaiseWidget(_hangar_dir + WID_BROD_DEPOT_NE);
				_hangar_dir = (DiagDirection)(widget - WID_BROD_DEPOT_NE);
				this->LowerWidget(_hangar_dir + WID_BROD_DEPOT_NE);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				EraseQueuedTouchCommand();
				this->SetDirty();
				break;

			default:
				break;
		}
	}
};

static const NWidgetPart _nested_build_hangar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_BROD_CAPTION), SetDataTip(STR_BUILD_HANGAR_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3),
		NWidget(NWID_HORIZONTAL_LTR),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0), SetFill(1, 0),
			NWidget(NWID_VERTICAL),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_BROD_DEPOT_NW), SetSizingType(NWST_BUTTON), SetDataTip(SPR_IMG_ROAD_DEPOT, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
				NWidget(NWID_SPACER), SetMinimalSize(0, 2),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_BROD_DEPOT_SW), SetSizingType(NWST_BUTTON),  SetDataTip(SPR_IMG_ROAD_DEPOT, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(2, 0),
			NWidget(NWID_VERTICAL),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_BROD_DEPOT_NE), SetSizingType(NWST_BUTTON), SetDataTip(SPR_IMG_ROAD_DEPOT, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
				NWidget(NWID_SPACER), SetMinimalSize(0, 2),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_BROD_DEPOT_SE), SetSizingType(NWST_BUTTON), SetDataTip(SPR_IMG_ROAD_DEPOT, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3),
	EndContainer(),
};

static WindowDesc _build_hangar_desc(
	WDP_AUTO, NULL, 0, 0,
	WC_BUILD_DEPOT, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_hangar_widgets, lengthof(_nested_build_hangar_widgets)
);

static void ShowHangarPicker(Window *parent)
{
	new BuildHangarWindow(&_build_hangar_desc, parent);
}

/** Set the initial (default) airtype to use */
static void SetDefaultAirGui()
{
	if (_local_company == COMPANY_SPECTATOR || !Company::IsValidID(_local_company)) return;

	extern AirType _last_built_airtype;
	AirType at = (AirType)(_settings_client.gui.default_air_type + AIRTYPE_END);
	if (at == DEF_AIRTYPE_MOST_USED) {
		/* Find the most used air type */
		AirType count[AIRTYPE_END];
		memset(count, 0, sizeof(count));
		for (TileIndex t = 0; t < MapSize(); t++) {
			if (IsAirportTile(t)) {
				count[GetAirportType(t)]++;
				}
		}

		at = AIRTYPE_GRAVEL;
		for (AirType a = AIRTYPE_ASPHALT; a < AIRTYPE_END; a++) {
			if (count[a] >= count[at]) at = a;
		}

		/* No rail, just get the first available one */
		if (count[at] == 0) at = DEF_AIRTYPE_FIRST;
	}
	switch (at) {
		case DEF_AIRTYPE_FIRST:
			at = AIRTYPE_GRAVEL;
			while (at < AIRTYPE_END && !HasAirTypeAvail(_local_company, at)) at++;
			break;

		case DEF_AIRTYPE_LAST:
			at = GetBestAirType(_local_company);
			break;

		default:
			break;
	}

	_last_built_airtype = _cur_airtype = at;
	BuildAirToolbarWindow *w = dynamic_cast<BuildAirToolbarWindow *>(FindWindowById(WC_BUILD_TOOLBAR, TRANSPORT_AIR));
	if (w != NULL) w->ModifyAirType(_cur_airtype);
}

/**
 * Compare railtypes based on their sorting order.
 * @param first  The railtype to compare to.
 * @param second The railtype to compare.
 * @return True iff the first should be sorted before the second.
 */
static int CDECL CompareAirTypes(const DropDownListItem * const *first, const DropDownListItem * const *second)
{
	return GetAirTypeInfo((AirType)(*first)->result)->sorting_order < GetAirTypeInfo((AirType)(*second)->result)->sorting_order;
}

/**
 * Create a drop down list for all the air types of the local company.
 * @param for_replacement Whether this list is for the replacement window.
 * @return The populated and sorted #DropDownList.
 */
DropDownList *GetAirTypeDropDownList(bool for_replacement)
{
	AirTypes used_airtypes = AIRTYPES_NONE;

	/* Find the used railtypes. */
	Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, VEH_AIRCRAFT) {
		if (!HasBit(e->info.climates, _settings_game.game_creation.landscape)) continue;

		used_airtypes |= GetAirTypeInfo(e->u.air.airtype)->introduces_airtypes;
	}

	/* Get the date introduced railtypes as well. */
	used_airtypes = AddDateIntroducedAirTypes(used_airtypes, MAX_DAY);

	const Company *c = Company::Get(_local_company);
	DropDownList *list = new DropDownList();
	for (AirType at = AIRTYPE_BEGIN; at != AIRTYPE_END; at++) {
		/* If it's not used ever, don't show it to the user. */
		if (!HasBit(used_airtypes, at)) continue;

		const AirTypeInfo *ati = GetAirTypeInfo(at);
		/* Skip rail type if it has no label */
		if (ati->label == 0) continue;

		StringID str = for_replacement ? ati->strings.replace_text : (ati->max_speed > 0 ? STR_TOOLBAR_RAILTYPE_VELOCITY : STR_JUST_STRING);
		DropDownListParamStringItem *item = new DropDownListParamStringItem(str, at, !HasBit(c->avail_airtypes, at));
		item->SetParam(0, ati->strings.menu_text);
		item->SetParam(1, ati->max_speed);
		*list->Append() = item;
	}
	QSortT(list->Begin(), list->Length(), CompareAirTypes);
	return list;
}

void InitializeAirportGui()
{
	SetDefaultAirGui();

	_selected_airport_class = APC_BEGIN;
	_selected_airport_index = -1;
}

