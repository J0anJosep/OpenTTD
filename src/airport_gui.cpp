/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_gui.cpp The GUI for airports. */

#include "stdafx.h"
#include "economy_func.h"
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
#include "dropdown_type.h"
#include "dropdown_func.h"
#include "core/geometry_func.hpp"
#include "hotkeys.h"
#include "vehicle_func.h"
#include "gui.h"
#include "command_func.h"
#include "airport_cmd.h"
#include "station_cmd.h"
#include "air_type.h"
#include "air.h"
#include "air_map.h"
#include "station_map.h"
#include "engine_base.h"
#include "zoom_func.h"
#include "timer/timer.h"
#include "timer/timer_game_calendar.h"

#include "widgets/airport_widget.h"

#include "safeguards.h"

static AirType _cur_airtype;                     ///< Air type of the current build-air toolbar.
static AirportTileType _airport_tile_type;       ///< Current airport tile type (hangar, infrastructure...
static DiagDirection _rotation_dir;              ///< Exit direction for new hangars, or rotation for heliports and infrastructure.
static bool _remove_button_clicked;              ///< Flag whether 'remove' toggle-button is currently enabled
static AirportClassID _selected_airport_class;   ///< the currently visible airport class
static int _selected_airport_index;              ///< the index of the selected airport in the current class or -1
static uint8_t _selected_airport_layout;         ///< selected airport layout number.
bool _show_airport_tracks = 0;                   ///< whether to show the airport tracks on viewports.
static DiagDirection _selected_rotation;         ///< selected rotation for airport.
static uint8_t _selected_infra_catch_rotation;   ///< selected rotation for infrastructure.
static AirportTiles _selected_infra_catch;       ///< selected infrastructure type.
static uint8_t _selected_infra_nocatch_rotation; ///< selected rotation for infrastructure.
static AirportTiles _selected_infra_nocatch;     ///< selected infrastructure type.

static void ShowBuildAirportPicker(Window *parent);
static void ShowHangarPicker(Window *parent);
static void ShowHeliportPicker(Window *parent);
static void ShowAirportInfraNoCatchPicker(Window *parent);
static void ShowAirportInfraWithCatchPicker(Window *parent);
Window *ShowBuildAirToolbar(AirType airtype);

SpriteID GetCustomAirportSprite(const AirportSpec *as, uint8_t layout);

void CcBuildAirport(Commands, const CommandCost &result, TileIndex tile)
{
	if (result.Failed()) return;

	if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_CONSTRUCTION_OTHER, tile);
	if (!_settings_client.gui.persistent_buildingtools) ResetObjectToPlace();
}

/**
 * Place an airport.
 * @param tile Position to put the new airport.
 */
static void PlaceAirport(TileIndex tile)
{
	if (_selected_airport_index == -1) return;

	uint8_t airport_type = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index)->GetIndex();
	uint8_t layout = _selected_airport_layout;
	bool adjacent = _ctrl_pressed;

	auto proc = [=](bool test, StationID to_join) -> bool {
		if (test) {
			return adjacent || Command<CMD_BUILD_AIRPORT>::Do(CommandFlagsToDCFlags(GetCommandFlags<CMD_BUILD_AIRPORT>()), tile, airport_type, layout, _cur_airtype, _selected_rotation, INVALID_STATION, adjacent).Succeeded();
		} else {
			return Command<CMD_BUILD_AIRPORT>::Post(STR_ERROR_CAN_T_BUILD_AIRPORT_HERE, CcBuildAirport, tile, airport_type, layout, _cur_airtype, _selected_rotation, to_join, adjacent);
		}
	};

	ShowSelectStationIfNeeded(TileArea(tile, _thd.size.x / TILE_SIZE, _thd.size.y / TILE_SIZE), proc);
}

/**
 * Get the other tile of a runway.
 * @param tile The tile.
 * @return the other extreme of the runway if the tile checked is the start or end of a runway
 *         or the same tile otherwise.
 */
 TileIndex GetOtherEndOfRunway(TileIndex tile)
{
	if (IsValidTile(tile) && IsAirportTile(tile) && IsRunwayExtreme(tile)) {
		AirportTileType att = GetAirportTileType(tile);
		DiagDirection dir = GetRunwayExtremeDirection(tile);
		if (att == ATT_RUNWAY_END) dir = ReverseDiagDir(dir);
		return GetRunwayExtreme(tile, dir);
	}
	return tile;
}

/** Airport build toolbar window handler. */
struct BuildAirToolbarWindow : Window {
	const bool allow_by_tile;
	int last_user_action; // Last started user action.

	BuildAirToolbarWindow(bool allow_by_tile, WindowDesc &desc, AirType airtype) : Window(desc), allow_by_tile(allow_by_tile)
	{
		this->CreateNestedTree();
		this->SetupAirToolbar(airtype);
		this->FinishInitNested(TRANSPORT_AIR);

		this->DisableWidget(WID_AT_REMOVE);
		this->last_user_action = INVALID_WID_AT;

		this->OnInvalidateData();
		if (_settings_client.gui.link_terraform_toolbar) ShowTerraformToolbar(this);
		_show_airport_tracks = true;
		MarkWholeScreenDirty();
	}

	void Close([[maybe_unused]] int data = 0) override
	{
		_show_airport_tracks = false;
		MarkWholeScreenDirty();

		if (this->IsWidgetLowered(WID_AT_AIRPORT)) SetViewportCatchmentStation(nullptr, true);
		if (_settings_client.gui.link_terraform_toolbar) CloseWindowById(WC_SCEN_LAND_GEN, 0, false);
		this->Window::Close();
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		if (!gui_scope) return;

		bool can_build = CanBuildVehicleInfrastructure(VEH_AIRCRAFT);
		this->SetWidgetDisabledState(WID_AT_AIRPORT, !can_build);
		if (!can_build) {
			CloseWindowById(WC_BUILD_STATION, TRANSPORT_AIR);

			/* Show in the tooltip why this button is disabled. */
			this->GetWidget<NWidgetCore>(WID_AT_AIRPORT)->SetToolTip(STR_TOOLBAR_DISABLED_NO_VEHICLE_AVAILABLE);
		} else {
			this->GetWidget<NWidgetCore>(WID_AT_AIRPORT)->SetToolTip(STR_TOOLBAR_AIRPORT_BUILD_AIRPORT_TOOLTIP);
		}
	}

	void SetStringParameters(int widget) const override
	{
		if (widget == WID_AT_CAPTION) {
			if (_settings_game.station.allow_modify_airports) {
				SetDParam(0, GetAirTypeInfo(_cur_airtype)->strings.toolbar_caption);
			} else {
				SetDParam(0, STR_TOOLBAR_AIRPORT_CAPTION);
			}
		}
	}

	/**
	* Configures the air toolbar for airtype given
	* @param airtype the airtype to display
	*/
	void SetupAirToolbar(AirType airtype)
	{
		if (!this->allow_by_tile) return;
		assert(airtype < AIRTYPE_END);

		_cur_airtype = airtype;
		const AirTypeInfo *ati = GetAirTypeInfo(airtype);

		this->GetWidget<NWidgetCore>(WID_AT_BUILD_TILE)->widget_data = ati->gui_sprites.add_airport_tiles;
		this->GetWidget<NWidgetCore>(WID_AT_TRACKS)->widget_data = ati->gui_sprites.build_track_tile;
		this->GetWidget<NWidgetCore>(WID_AT_CONVERT)->widget_data = ati->gui_sprites.change_airtype;
		this->GetWidget<NWidgetCore>(WID_AT_INFRASTRUCTURE_CATCH)->widget_data = ati->gui_sprites.build_catchment_infra;
		this->GetWidget<NWidgetCore>(WID_AT_INFRASTRUCTURE_NO_CATCH)->widget_data = ati->gui_sprites.build_noncatchment_infra;
		this->GetWidget<NWidgetCore>(WID_AT_RUNWAY_LANDING)->widget_data = ati->gui_sprites.define_landing_runway;
		this->GetWidget<NWidgetCore>(WID_AT_RUNWAY_NO_LANDING)->widget_data = ati->gui_sprites.define_nonlanding_runway;
		this->GetWidget<NWidgetCore>(WID_AT_APRON)->widget_data = ati->gui_sprites.build_apron;
		this->GetWidget<NWidgetCore>(WID_AT_HELIPAD)->widget_data = ati->gui_sprites.build_helipad;
		this->GetWidget<NWidgetCore>(WID_AT_HELIPORT)->widget_data = ati->gui_sprites.build_heliport;
		if (this->HasWidget(WID_AT_HANGAR_STANDARD)) this->GetWidget<NWidgetCore>(WID_AT_HANGAR_STANDARD)->widget_data = ati->gui_sprites.build_hangar;
		if (this->HasWidget(WID_AT_HANGAR_EXTENDED)) this->GetWidget<NWidgetCore>(WID_AT_HANGAR_EXTENDED)->widget_data = ati->gui_sprites.build_hangar;

		if (!AreHeliportsAvailable(airtype)) DisableWidget(WID_AT_HELIPORT);
	}

	/**
	* Switch to another air type.
	* @param airtype New air type.
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
		CloseWindowById(WC_SELECT_STATION, 0);
		this->ToggleWidgetLoweredState(WID_AT_REMOVE);
		this->SetWidgetDirty(WID_AT_REMOVE);
		_remove_button_clicked = this->IsWidgetLowered(WID_AT_REMOVE);

		if (this->last_user_action == WID_AT_RUNWAY_LANDING ||
				this->last_user_action == WID_AT_RUNWAY_NO_LANDING) {
			SetObjectToPlace(GetAirTypeInfo(_cur_airtype)->cursor.build_hangar, PAL_NONE, _remove_button_clicked ? HT_SPECIAL : HT_RECT, this->window_class, this->window_number);
			this->LowerWidget(this->last_user_action);
			this->SetWidgetLoweredState(WID_AT_REMOVE, _remove_button_clicked);
		}

		SetSelectionRed(_remove_button_clicked);
		if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
	}

	void UpdateWidgetSize(int widget, Dimension &size, const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
			if (!IsInsideMM(widget, WID_AT_BUILD_TILE, WID_AT_REMOVE)) return;

			NWidgetCore *wid = this->GetWidget<NWidgetCore>(widget);

			Dimension d = GetSpriteSize(wid->widget_data);
			d.width += padding.width;
			d.height += padding.height;
			size = d;
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

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_AT_BUILD_TILE:
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.add_airport_tiles, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_AT_TRACKS:
				_airport_tile_type = ATT_SIMPLE_TRACK;
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_track_tile, HT_RAIL);
				this->last_user_action = widget;
				break;

			case WID_AT_REMOVE:
				this->BuildAirClick_Remove();
				return;

			case WID_AT_AIRPORT:
				if (HandlePlacePushButton(this, WID_AT_AIRPORT, SPR_CURSOR_AIRPORT, HT_RECT)) {
					ShowBuildAirportPicker(this);
					this->last_user_action = widget;
				}
				break;

			case WID_AT_DEMOLISH:
				HandlePlacePushButton(this, WID_AT_DEMOLISH, ANIMCURSOR_DEMOLISH, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_AT_CONVERT:
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.change_airtype, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_AT_INFRASTRUCTURE_CATCH:
				_airport_tile_type = ATT_INFRASTRUCTURE_WITH_CATCH;
				if (HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_catchment_infra, HT_RECT | HT_DIAGONAL)) {
					ShowAirportInfraWithCatchPicker(this);
				}
				this->last_user_action = widget;
				break;

			case WID_AT_INFRASTRUCTURE_NO_CATCH:
				_airport_tile_type = ATT_INFRASTRUCTURE_NO_CATCH;
				if (HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_noncatchment_infra, HT_RECT | HT_DIAGONAL)) {
					ShowAirportInfraNoCatchPicker(this);
				}
				this->last_user_action = widget;
				break;

			case WID_AT_APRON:
				_airport_tile_type = ATT_APRON_NORMAL;
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_apron, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_AT_HELIPAD:
				_airport_tile_type = ATT_APRON_HELIPAD;
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_helipad, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_AT_HELIPORT:
				_airport_tile_type = ATT_APRON_HELIPORT;
				if (HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_heliport, HT_RECT | HT_DIAGONAL)) {
					ShowHeliportPicker(this);
				}
				this->last_user_action = widget;
				break;

			case WID_AT_HANGAR_STANDARD:
			case WID_AT_HANGAR_EXTENDED:
				_airport_tile_type = widget == WID_AT_HANGAR_STANDARD ? ATT_HANGAR_STANDARD : ATT_HANGAR_EXTENDED;
				if (HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.build_hangar, HT_RECT)) {
					ShowHangarPicker(this);
				}
				this->last_user_action = widget;
				break;

			case WID_AT_RUNWAY_LANDING:
				_airport_tile_type = ATT_RUNWAY_START_ALLOW_LANDING;
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.define_landing_runway, _remove_button_clicked ? HT_SPECIAL : HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_AT_RUNWAY_NO_LANDING:
				_airport_tile_type = ATT_RUNWAY_START_NO_LANDING;
				HandlePlacePushButton(this, widget, GetAirTypeInfo(_cur_airtype)->cursor.define_nonlanding_runway, _remove_button_clicked ? HT_SPECIAL : HT_RECT);
				this->last_user_action = widget;
				break;

			default: break;
		}

		UpdateRemoveWidgetStatus(widget);
	}

	void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override
	{
		switch (this->last_user_action) {
			case WID_AT_BUILD_TILE:
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_BUILD_STATION);
				break;

			case WID_AT_TRACKS:
				VpStartPlaceSizing(tile, VPM_RAILDIRS, DDSP_PLACE_RAIL);
				break;

			case WID_AT_AIRPORT:
				PlaceAirport(tile);
				break;

			case WID_AT_DEMOLISH:
				PlaceProc_DemolishArea(tile);
				break;

			case WID_AT_CONVERT:
				Command<CMD_CONVERT_AIRPORT>::Post(STR_ERROR_CAN_T_DO_THIS, tile, _cur_airtype);
				break;

			case WID_AT_HANGAR_STANDARD:
			case WID_AT_HANGAR_EXTENDED:
				VpStartPlaceSizing(tile, HasBit(_rotation_dir, 0) ? VPM_FIX_Y : VPM_FIX_X, DDSP_BUILD_STATION);
				break;

			case WID_AT_INFRASTRUCTURE_CATCH:
			case WID_AT_INFRASTRUCTURE_NO_CATCH:
			case WID_AT_APRON:
			case WID_AT_HELIPAD:
			case WID_AT_HELIPORT:
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_BUILD_STATION);
				break;

			case WID_AT_RUNWAY_LANDING:
			case WID_AT_RUNWAY_NO_LANDING:
				if (_remove_button_clicked) {
					Command<CMD_CHANGE_AIRPORT>::Post(STR_ERROR_CAN_T_DO_THIS, tile, GetOtherEndOfRunway(tile), _cur_airtype, _airport_tile_type, ATTG_DEFAULT_GFX, (DiagDirection)0, !_remove_button_clicked, false);
				} else {
					VpStartPlaceSizing(tile, VPM_X_OR_Y, DDSP_BUILD_STATION);
				}
				break;

			default: NOT_REACHED();
		}
	}

	void OnPlaceDrag(ViewportPlaceMethod select_method, [[maybe_unused]] ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt) override
	{
		VpSelectTilesWithMethod(pt.x, pt.y, select_method);
	}

	void OnPlaceMouseUp([[maybe_unused]] ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt, TileIndex start_tile, TileIndex end_tile) override
	{
		if (pt.x == -1) return;

		switch (this->last_user_action) {
			case WID_AT_BUILD_TILE: {
				assert(_settings_game.station.allow_modify_airports);

				auto proc = [=](bool test, StationID to_join) -> bool {
					if (test) {
						return (_ctrl_pressed && !_remove_button_clicked) || Command<CMD_ADD_REM_AIRPORT>::Do(CommandFlagsToDCFlags(GetCommandFlags<CMD_ADD_REM_AIRPORT>()), start_tile, end_tile, !_remove_button_clicked, _cur_airtype, INVALID_STATION, _ctrl_pressed).Succeeded();
					} else {
						return Command<CMD_ADD_REM_AIRPORT>::Post(STR_ERROR_CAN_T_DO_THIS, start_tile, end_tile, !_remove_button_clicked, _cur_airtype, to_join, _ctrl_pressed);
					}
				};

				ShowSelectStationIfNeeded(TileArea(start_tile, end_tile), proc);
				break;
			}
			case WID_AT_TRACKS:
				assert(_settings_game.station.allow_modify_airports);
				Command<CMD_ADD_REM_TRACKS>::Post(STR_ERROR_CAN_T_DO_THIS, start_tile, end_tile, _cur_airtype, !_remove_button_clicked, (Track)(_thd.drawstyle & HT_DIR_MASK));
				break;
			case WID_AT_AIRPORT:
				assert(start_tile == end_tile);
				PlaceAirport(end_tile);
				break;
			case WID_AT_DEMOLISH:
				GUIPlaceProcDragXY(select_proc, start_tile, end_tile);
				break;
			case WID_AT_CONVERT:
				NOT_REACHED();
			case WID_AT_INFRASTRUCTURE_CATCH:
			case WID_AT_INFRASTRUCTURE_NO_CATCH:
			case WID_AT_APRON:
			case WID_AT_HELIPAD:
			case WID_AT_HELIPORT:
			case WID_AT_HANGAR_STANDARD:
			case WID_AT_HANGAR_EXTENDED:
			case WID_AT_RUNWAY_LANDING:
			case WID_AT_RUNWAY_NO_LANDING: {
				DiagDirection dir = _rotation_dir;
				AirportTiles gfx = _selected_infra_catch;
				bool diagonal = _ctrl_pressed &&
						(this->last_user_action != WID_AT_HANGAR_EXTENDED && this->last_user_action != WID_AT_HANGAR_STANDARD);
				if (this->last_user_action == WID_AT_INFRASTRUCTURE_CATCH) {
					dir = (DiagDirection)_selected_infra_catch_rotation;
				} else if (this->last_user_action == WID_AT_INFRASTRUCTURE_NO_CATCH) {
					dir = (DiagDirection)_selected_infra_nocatch_rotation;
					gfx = (AirportTiles)(ATTG_NO_CATCH_FLAG + _selected_infra_nocatch);
				}
				bool ret = Command<CMD_CHANGE_AIRPORT>::Post(STR_ERROR_CAN_T_DO_THIS, start_tile, end_tile, _cur_airtype, _airport_tile_type, gfx, dir, !_remove_button_clicked, diagonal);
				if (ret && _remove_button_clicked &&
						(this->last_user_action == WID_AT_RUNWAY_LANDING || this->last_user_action == WID_AT_RUNWAY_NO_LANDING)) {
					VpStartPlaceSizing(start_tile, VPM_X_OR_Y, DDSP_BUILD_STATION);
				}
				break;
			}
			default: NOT_REACHED();
		}
	}

	void OnPlacePresize([[maybe_unused]] Point pt, TileIndex tile) override
	{
		assert(this->last_user_action == WID_AT_RUNWAY_LANDING ||
				this->last_user_action == WID_AT_RUNWAY_NO_LANDING);
		assert(_remove_button_clicked);
		VpSetPresizeRange(tile, GetOtherEndOfRunway(tile));
	}

	void OnPlaceObjectAbort() override
	{
		if (this->IsWidgetLowered(WID_AT_AIRPORT)) SetViewportCatchmentStation(nullptr, true);

		this->RaiseButtons();

		CloseWindowById(WC_BUILD_DEPOT, TRANSPORT_AIR);
		CloseWindowById(WC_BUILD_STATION, TRANSPORT_AIR);
		CloseWindowById(WC_SELECT_STATION, 0);
		CloseWindowById(WC_BUILD_HELIPORT, TRANSPORT_AIR);
		CloseWindowById(WC_BUILD_AIRPORT_INFRASTRUCTURE, TRANSPORT_AIR);
	}

	/**
	 * Handler for global hotkeys of the BuildAirToolbarWindow.
	 * @param hotkey Hotkey
	 * @return ES_HANDLED if hotkey was accepted.
	 */
	static EventState AirportToolbarGlobalHotkeys(int hotkey)
	{
		if (_game_mode != GM_NORMAL  || !CanBuildVehicleInfrastructure(VEH_AIRCRAFT)) return ES_NOT_HANDLED;
		extern AirType _last_built_airtype;
		Window *w = ShowBuildAirToolbar(_settings_game.station.allow_modify_airports ? _last_built_airtype : INVALID_AIRTYPE);
		if (w == nullptr) return ES_NOT_HANDLED;
		return w->OnHotkey(hotkey);
	}

	static inline HotkeyList hotkeys{"airtoolbar", {
		Hotkey('1', "airport", WID_AT_AIRPORT),
		Hotkey('2', "demolish", WID_AT_DEMOLISH),
		Hotkey('3', "remove", WID_AT_REMOVE),
		Hotkey('4', "tiles", WID_AT_BUILD_TILE),
		Hotkey('5', "tracks", WID_AT_TRACKS),
		Hotkey('6', "infra_with_catch", WID_AT_INFRASTRUCTURE_CATCH),
		Hotkey('7', "infra_no_catch", WID_AT_INFRASTRUCTURE_NO_CATCH),
	}, AirportToolbarGlobalHotkeys};
};

/**
 * Add the depot icons depending on availability of construction.
 * @return Panel with hangar buttons.
 */
static std::unique_ptr<NWidgetBase> MakeNWidgetHangars()
{
	auto hor = std::make_unique<NWidgetHorizontal>();

	if (HasBit(_settings_game.depot.hangar_types, 0)) {
		/* Add the widget for building standard hangar. */
		hor->Add(std::make_unique<NWidgetLeaf>(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HANGAR_STANDARD, 0, STR_TOOLBAR_AIRPORT_BUILD_HANGAR_STANDARD));
	}

	if (HasBit(_settings_game.depot.hangar_types, 1)) {
		/* Add the widget for building extended hangar. */
		hor->Add(std::make_unique<NWidgetLeaf>(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HANGAR_EXTENDED, 0, STR_TOOLBAR_AIRPORT_BUILD_HANGAR_EXTENDED));
	}

	return hor;
}

static constexpr NWidgetPart _nested_air_tile_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_AT_CAPTION), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS), SetTextStyle(TC_WHITE),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_VERTICAL),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_AIRPORT), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_AIRPORT, STR_TOOLBAR_AIRPORT_BUILD_PRE_AIRPORT_TOOLTIP),

			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_DEMOLISH), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_REMOVE), SetFill(0, 1),
			SetDataTip(SPR_IMG_REMOVE, STR_TOOLBAR_AIRPORT_TOGGLE_BUILD_REMOVE),

			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_BUILD_TILE), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(0, STR_TOOLBAR_AIRPORT_ADD_TILES),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_TRACKS), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(0, STR_TOOLBAR_AIRPORT_SET_TRACKS),

			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_INFRASTRUCTURE_CATCH), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_INFRASTRUCTURE_CATCHMENT),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_INFRASTRUCTURE_NO_CATCH), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_INFRASTRUCTURE_NO_CATCHMENT),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_RUNWAY_LANDING), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_DEFINE_RUNWAY_LANDING),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_RUNWAY_NO_LANDING), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_DEFINE_RUNWAY_NO_LANDING),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),

			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_APRON), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_BUILD_APRON),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HELIPAD), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_BUILD_HELIPAD),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_HELIPORT), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_BUILD_HELIPORT),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),
			NWidgetFunction(MakeNWidgetHangars),
			NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),
			NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_CONVERT), SetFill(0, 1),
					SetDataTip(0, STR_TOOLBAR_AIRPORT_CHANGE_AIRTYPE),
		EndContainer(),
	EndContainer(),
};

static const NWidgetPart _nested_air_nontile_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_AT_CAPTION), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_AIRPORT), SetFill(0, 1), SetMinimalSize(42, 22),
				SetDataTip(SPR_IMG_AIRPORT, STR_TOOLBAR_AIRCRAFT_BUILD_PRE_AIRPORT_TOOLTIP),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), SetFill(1, 1), EndContainer(),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_AT_DEMOLISH), SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
	EndContainer(),
};

static WindowDesc _air_tile_toolbar_desc(
	WDP_ALIGN_TOOLBAR, "toolbar_air", 0, 0,
	WC_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_air_tile_toolbar_widgets,
	&BuildAirToolbarWindow::hotkeys
);

static WindowDesc _air_nontile_toolbar_desc(
	WDP_ALIGN_TOOLBAR, "toolbar_air_nontile", 0, 0,
	WC_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_air_nontile_toolbar_widgets,
	&BuildAirToolbarWindow::hotkeys
);


/**
 * Open the build airport toolbar window.
 * If the terraform toolbar is linked to the toolbar, that window is also opened.
 * @param airtype air type for constructing (a valid air type or
 *  			INVALID_AIRTYPE if the build-airports-by-tile is dissabled).
 * @return newly opened airport toolbar, or nullptr if the toolbar could not be opened.
 */
Window *ShowBuildAirToolbar(AirType airtype)
{
	if (!Company::IsValidID(_local_company)) return nullptr;
	if (airtype != INVALID_AIRTYPE && !ValParamAirType(airtype)) return nullptr;

	CloseWindowByClass(WC_BUILD_TOOLBAR);
	assert((airtype == INVALID_AIRTYPE) != (_settings_game.station.allow_modify_airports));

	_cur_airtype = airtype;
	_remove_button_clicked = false;

	if (airtype == INVALID_AIRTYPE) {
		return new BuildAirToolbarWindow(false, _air_nontile_toolbar_desc, airtype);
	} else {
		return new BuildAirToolbarWindow(true, _air_tile_toolbar_desc, airtype);
	}
}

class BuildAirportWindow : public PickerWindowBase {
	SpriteID preview_sprite; ///< Cached airport preview sprite.
	int line_height;
	Scrollbar *vscroll;

	/** Build a dropdown list of available airport classes */
	static DropDownList BuildAirportClassDropDown()
	{
		DropDownList list;

		for (const auto &cls : AirportClass::Classes()) {
			if (cls.Index() == APC_CUSTOM) continue;
			bool unavailable = true;
			for (const auto &as : cls.Specs()) {
				if (!as->IsAvailable(_cur_airtype)) continue;
				unavailable = false;
				break;
			}
			list.push_back(MakeDropDownListStringItem(cls.name, cls.Index(), unavailable));
		}

		return list;
	}

public:
	BuildAirportWindow(WindowDesc &desc, Window *parent) : PickerWindowBase(desc, parent)
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
			if (as->IsAvailable(_cur_airtype)) {
				/* Ensure the airport layout is valid. */
				_selected_airport_layout = Clamp(_selected_airport_layout, 0, as->layouts.size() - 1);
				_selected_rotation = (DiagDirection)Clamp(_selected_rotation, 0, 3);
				selectFirstAirport = false;
				this->UpdateSelectSize();
			}
		}

		if (selectFirstAirport) this->SelectFirstAvailableAirport(true);
	}

	void Close([[maybe_unused]] int data = 0) override
	{
		CloseWindowById(WC_SELECT_STATION, 0);
		this->PickerWindowBase::Close();
	}

	void SetStringParameters(WidgetID widget) const override
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
					} else if (as->layouts.size() > 1) {
						SetDParam(0, STR_STATION_BUILD_AIRPORT_LAYOUT_NAME);
						SetDParam(1, _selected_airport_layout + 1);
					}
				}
				break;

			case WID_AP_ROTATION:
				SetDParam(0, STR_EMPTY);
				if (_selected_airport_index != -1) {
					SetDParam(0, STR_AIRPORT_ROTATION_0 + _selected_rotation);
				}
				break;

			default: break;
		}
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		switch (widget) {
			case WID_AP_CLASS_DROPDOWN: {
				Dimension d = {0, 0};
				for (const auto &cls : AirportClass::Classes()) {
					d = maxdim(d, GetStringBoundingBox(cls.name));
				}
				d.width += padding.width;
				d.height += padding.height;
				size = maxdim(size, d);
				break;
			}

			case WID_AP_AIRPORT_LIST: {
				for (int i = 0; i < NUM_AIRPORTS; i++) {
					const AirportSpec *as = AirportSpec::Get(i);
					if (!as->enabled) continue;

					size.width = std::max(size.width, GetStringBoundingBox(as->name).width + padding.width);
				}

				this->line_height = GetCharacterHeight(FS_NORMAL) + padding.height;
				size.height = 5 * this->line_height;
				break;
			}

			case WID_AP_AIRPORT_SPRITE:
				for (int i = 0; i < NUM_AIRPORTS; i++) {
					const AirportSpec *as = AirportSpec::Get(i);
					if (!as->enabled) continue;
					for (uint8_t layout = 0; layout < static_cast<uint8_t>(as->layouts.size()); layout++) {
						SpriteID sprite = GetCustomAirportSprite(as, layout);
						if (sprite != 0) {
							Dimension d = GetSpriteSize(sprite);
							d.width += WidgetDimensions::scaled.framerect.Horizontal();
							d.height += WidgetDimensions::scaled.framerect.Vertical();
							size = maxdim(d, size);
						}
					}
				}
				break;

			case WID_AP_EXTRA_TEXT:
				for (int i = NEW_AIRPORT_OFFSET; i < NUM_AIRPORTS; i++) {
					const AirportSpec *as = AirportSpec::Get(i);
					if (!as->enabled) continue;
					for (uint8_t layout = 0; layout < static_cast<uint8_t>(as->layouts.size()); layout++) {
						StringID string = GetAirportTextCallback(as, layout, CBID_AIRPORT_ADDITIONAL_TEXT);
						if (string == STR_UNDEFINED) continue;

						Dimension d = GetStringMultiLineBoundingBox(string, size);
						size = maxdim(d, size);
					}
				}
				break;

			default: break;
		}
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		switch (widget) {
			case WID_AP_AIRPORT_LIST: {
				Rect row = r.WithHeight(this->line_height).Shrink(WidgetDimensions::scaled.bevel);
				Rect text = r.WithHeight(this->line_height).Shrink(WidgetDimensions::scaled.matrix);
				const auto specs = AirportClass::Get(_selected_airport_class)->Specs();
				auto [first, last] = this->vscroll->GetVisibleRangeIterators(specs);
				for (auto it = first; it != last; ++it) {
					const AirportSpec *as = *it;
					if (!as->IsAvailable(_cur_airtype)) {
						GfxFillRect(row, PC_BLACK, FILLRECT_CHECKER);
					}
					DrawString(text, as->name, (static_cast<int>(as->index) == _selected_airport_index) ? TC_WHITE : TC_BLACK);
					row = row.Translate(0, this->line_height);
					text = text.Translate(0, this->line_height);
				}
				break;
			}

			case WID_AP_AIRPORT_SPRITE:
				if (this->preview_sprite != 0) {
					Dimension d = GetSpriteSize(this->preview_sprite);
					DrawSprite(this->preview_sprite, COMPANY_SPRITE_COLOUR(_local_company), CenterBounds(r.left, r.right, d.width), CenterBounds(r.top, r.bottom, d.height));
				}
				break;

			case WID_AP_EXTRA_TEXT:
				if (_selected_airport_index != -1) {
					const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
					StringID string = GetAirportTextCallback(as, _selected_airport_layout, CBID_AIRPORT_ADDITIONAL_TEXT);
					if (string != STR_UNDEFINED) {

						SetDParam(0, string);
						DrawStringMultiLine(r.left, r.right, r.top, r.bottom, TC_BLACK);
					} else if (as->layouts.size() > 1) {
						SetDParam(0, STR_STATION_BUILD_AIRPORT_LAYOUT_NAME);
						SetDParam(1, _selected_airport_layout + 1);
					}
				}
				break;
		}
	}

	void OnPaint() override
	{
		this->DrawWidgets();

		Rect r = this->GetWidget<NWidgetBase>(WID_AP_ACCEPTANCE)->GetCurrentRect();
		int top = r.top;

		if (_selected_airport_index != -1) {
			const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
			AirType airtype = _settings_game.station.allow_modify_airports ? _cur_airtype : as->airtype;
			const AirTypeInfo *ati = GetAirTypeInfo(airtype);
			int rad = _settings_game.station.modified_catchment ? ati->catchment_radius : (uint)CA_UNMODIFIED;

			/* only show the station (airport) noise, if the noise option is activated */
			if (_settings_game.economy.station_noise_level) {
				/* show the noise of the selected airport */
				SetDParam(0, as->GetAirportNoise(airtype));
				DrawString(r.left, r.right, top, STR_STATION_BUILD_NOISE);
				top += GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal;
			}

			/* strings such as 'Size' and 'Coverage Area' */
			top = DrawStationCoverageAreaText(r.left, r.right, top, SCT_ALL, rad, false) + WidgetDimensions::scaled.vsep_normal;
			top = DrawStationCoverageAreaText(r.left, r.right, top, SCT_ALL, rad, true);
		}

		/* Resize background if the window is too small.
		 * Never make the window smaller to avoid oscillating if the size change affects the acceptance.
		 * (This is the case, if making the window bigger moves the mouse into the window.) */
		if (top > r.bottom) {
			ResizeWindow(this, 0, top - r.bottom, false);
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
		if (_selected_airport_index == -1) {
			SetTileSelectSize(1, 1);
			this->DisableWidget(WID_AP_LAYOUT_DECREASE);
			this->DisableWidget(WID_AP_LAYOUT_INCREASE);
			this->DisableWidget(WID_AP_ROTATION_DECREASE);
			this->DisableWidget(WID_AP_ROTATION_INCREASE);
		} else {
			const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(_selected_airport_index);
			int w = as->layouts[_selected_airport_layout].size_x;
			int h = as->layouts[_selected_airport_layout].size_y;
			if (_selected_rotation % 2 != 0) Swap(w, h);
			SetTileSelectSize(w, h);

			this->preview_sprite = GetCustomAirportSprite(as, _selected_airport_layout) + _selected_rotation;

			this->SetWidgetDisabledState(WID_AP_LAYOUT_DECREASE, _selected_airport_layout == 0);
			this->SetWidgetDisabledState(WID_AP_LAYOUT_INCREASE, _selected_airport_layout + 1U >= as->layouts.size());

			const AirTypeInfo *ati = GetAirTypeInfo(as->airtype);
			int rad = _settings_game.station.modified_catchment ? ati->catchment_radius : (uint)CA_UNMODIFIED;
			if (_settings_client.gui.station_show_coverage) SetTileSelectBigSize(-rad, -rad, 2 * rad, 2 * rad);
		}
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_AP_CLASS_DROPDOWN:
				ShowDropDownList(this, BuildAirportClassDropDown(), _selected_airport_class, WID_AP_CLASS_DROPDOWN);
				break;

			case WID_AP_AIRPORT_LIST: {
				int32_t num_clicked = this->vscroll->GetScrolledRowFromWidget(pt.y, this, widget, 0, this->line_height);
				if (num_clicked == INT32_MAX) break;
				const AirportSpec *as = AirportClass::Get(_selected_airport_class)->GetSpec(num_clicked);
				if (as->IsAvailable(_cur_airtype)) this->SelectOtherAirport(num_clicked);
				break;
			}

			case WID_AP_BTN_DONTHILIGHT: case WID_AP_BTN_DOHILIGHT:
				_settings_client.gui.station_show_coverage = (widget != WID_AP_BTN_DONTHILIGHT);
				this->SetWidgetLoweredState(WID_AP_BTN_DONTHILIGHT, !_settings_client.gui.station_show_coverage);
				this->SetWidgetLoweredState(WID_AP_BTN_DOHILIGHT, _settings_client.gui.station_show_coverage);
				this->SetDirty();
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->UpdateSelectSize();
				SetViewportCatchmentStation(nullptr, true);
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

			case WID_AP_ROTATION_DECREASE:
				_selected_rotation = (DiagDirection)((_selected_rotation + 3) % 4);
				this->UpdateSelectSize();
				this->SetDirty();
				break;

			case WID_AP_ROTATION_INCREASE:
				_selected_rotation = (DiagDirection)((_selected_rotation + 1) % 4);
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
		for (const AirportSpec *as : sel_apclass->Specs()) {
			if (as->IsAvailable(_cur_airtype)) {
				this->SelectOtherAirport(as->index);
				return;
			}
		}
		if (change_class) {
			/* If that fails, select the first available airport
			 * from the first class where airports are available. */
			for (const auto &cls : AirportClass::Classes()) {
				for (const auto &as : cls.Specs()) {
					if (as->IsAvailable(_cur_airtype)) {
						_selected_airport_class = cls.Index();
						this->vscroll->SetCount(cls.GetSpecCount());
						this->SelectOtherAirport(as->GetIndex());
						return;
					}
				}
			}
		}
		/* If all airports are unavailable, select nothing. */
		this->SelectOtherAirport(-1);
	}

	void OnDropdownSelect(WidgetID widget, int index) override
	{
		if (widget == WID_AP_CLASS_DROPDOWN) {
			_selected_airport_class = (AirportClassID)index;
			this->vscroll->SetCount(AirportClass::Get(_selected_airport_class)->GetSpecCount());
			this->SelectFirstAvailableAirport(false);
		}
	}

	void OnRealtimeTick([[maybe_unused]] uint delta_ms) override
	{
		CheckRedrawStationCoverage(this);
	}

	IntervalTimer<TimerGameCalendar> yearly_interval = {{TimerGameCalendar::YEAR, TimerGameCalendar::Priority::NONE}, [this](auto) {
		this->InvalidateData();
	}};
};

static constexpr NWidgetPart _nested_build_airport_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_STATION_BUILD_AIRPORT_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_VERTICAL), SetPIP(0, WidgetDimensions::unscaled.vsep_normal, 0), SetPadding(WidgetDimensions::unscaled.picker),
			NWidget(NWID_VERTICAL), SetPIP(0, WidgetDimensions::unscaled.vsep_picker, 0),
				NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetDataTip(STR_STATION_BUILD_AIRPORT_CLASS_LABEL, STR_NULL), SetFill(1, 0),
				NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_AP_CLASS_DROPDOWN), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_STATION_BUILD_AIRPORT_TOOLTIP),
				NWidget(WWT_EMPTY, COLOUR_DARK_GREEN, WID_AP_AIRPORT_SPRITE), SetFill(1, 0),
				NWidget(NWID_HORIZONTAL),
					NWidget(WWT_MATRIX, COLOUR_GREY, WID_AP_AIRPORT_LIST), SetFill(1, 0), SetMatrixDataTip(1, 5, STR_STATION_BUILD_AIRPORT_TOOLTIP), SetScrollbar(WID_AP_SCROLLBAR),
					NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_AP_SCROLLBAR),
				EndContainer(),
				NWidget(NWID_HORIZONTAL),
					NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_AP_LAYOUT_DECREASE), SetMinimalSize(12, 0), SetDataTip(AWV_DECREASE, STR_NULL),
					NWidget(WWT_LABEL, COLOUR_GREY, WID_AP_LAYOUT_NUM), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING1, STR_NULL),
					NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_AP_LAYOUT_INCREASE), SetMinimalSize(12, 0), SetDataTip(AWV_INCREASE, STR_NULL),
				EndContainer(),
				NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_AP_ROTATION_DECREASE), SetMinimalSize(12, 0), SetDataTip(AWV_DECREASE, STR_NULL),
				NWidget(WWT_LABEL, COLOUR_GREY, WID_AP_ROTATION), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_NULL),
				NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_AP_ROTATION_INCREASE), SetMinimalSize(12, 0), SetDataTip(AWV_INCREASE, STR_NULL),
				EndContainer(),
				NWidget(WWT_EMPTY, COLOUR_DARK_GREEN, WID_AP_EXTRA_TEXT), SetFill(1, 0), SetMinimalSize(150, 0),
				NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetDataTip(STR_STATION_BUILD_COVERAGE_AREA_TITLE, STR_NULL), SetFill(1, 0),
				NWidget(NWID_HORIZONTAL), SetPIP(14, 0, 14), SetPIPRatio(1, 0, 1),
					NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
						NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_AP_BTN_DONTHILIGHT), SetMinimalSize(60, 12), SetFill(1, 0),
													SetDataTip(STR_STATION_BUILD_COVERAGE_OFF, STR_STATION_BUILD_COVERAGE_AREA_OFF_TOOLTIP),
						NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_AP_BTN_DOHILIGHT), SetMinimalSize(60, 12), SetFill(1, 0),
													SetDataTip(STR_STATION_BUILD_COVERAGE_ON, STR_STATION_BUILD_COVERAGE_AREA_ON_TOOLTIP),
					EndContainer(),
				EndContainer(),
			EndContainer(),
			NWidget(WWT_EMPTY, COLOUR_DARK_GREEN, WID_AP_ACCEPTANCE), SetResize(0, 1), SetFill(1, 0), SetMinimalTextLines(2, WidgetDimensions::unscaled.vsep_normal),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _build_airport_desc(
	WDP_AUTO, nullptr, 0, 0,
	WC_BUILD_STATION, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_airport_widgets
);

static void ShowBuildAirportPicker(Window *parent)
{
	new BuildAirportWindow(_build_airport_desc, parent);
}

struct BuildHangarWindow : public PickerWindowBase {
	BuildHangarWindow(WindowDesc &desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->CreateNestedTree();
		this->LowerWidget(WID_BHW_NE + _rotation_dir);
		this->FinishInitNested(TRANSPORT_AIR);
	}

	uint GetHangarSpriteHeight() const { return 48; }

	void UpdateWidgetSize(int widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (!IsInsideMM(widget, WID_BHW_NE, WID_BHW_NW + 1)) return;

		size.width  = ScaleGUITrad(64) + 2;
		size.height = ScaleGUITrad(52) + 2;
	}

	void DrawWidget(const Rect &r, int widget) const override
	{
		if (!IsInsideMM(widget, WID_BHW_NE, WID_BHW_NW + 1)) return;

		int x = r.left + 1 + ScaleGUITrad(TILE_PIXELS - 1);
		/* Height of depot sprite in OpenGFX is TILE_PIXELS + GetHangarSpriteHeight(). */
		int y = r.bottom - ScaleGUITrad(TILE_PIXELS - 1);

		SpriteID ground = GetAirTypeInfo(_cur_airtype)->base_sprites.ground;
		DiagDirection dir = (DiagDirection)(widget - WID_BHW_NE + DIAGDIR_NE);
		PaletteID palette = COMPANY_SPRITE_COLOUR(_local_company);
		extern const DrawTileSprites _airport_hangars[4];
		const DrawTileSprites *dts = &_airport_hangars[dir];
		DrawSprite(ground, PAL_NONE, x, y);
		DrawRailTileSeqInGUI(x, y, dts, ground, 0, palette);
	}

	void OnClick([[maybe_unused]] Point pt, int widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_BHW_NW:
			case WID_BHW_NE:
			case WID_BHW_SW:
			case WID_BHW_SE:
				this->RaiseWidget(WID_BHW_NE + _rotation_dir);
				_rotation_dir = (DiagDirection)(widget - WID_BHW_NE);
				this->LowerWidget(WID_BHW_NE + _rotation_dir);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
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
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_BHW_CAPTION), SetDataTip(STR_BUILD_HANGAR_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_HORIZONTAL_LTR), SetPIP(0, WidgetDimensions::unscaled.hsep_normal, 0), SetPIPRatio(1, 0, 1), SetPadding(WidgetDimensions::unscaled.picker),
			NWidget(NWID_VERTICAL), SetPIP(0, WidgetDimensions::unscaled.vsep_normal, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_NW), SetDataTip(0x0, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_SW), SetDataTip(0x0, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
			EndContainer(),
			NWidget(NWID_VERTICAL), SetPIP(0, WidgetDimensions::unscaled.vsep_normal, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_NE), SetDataTip(0x0, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_SE), SetDataTip(0x0, STR_BUILD_HANGAR_ORIENTATION_TOOLTIP),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _build_hangar_desc(
	WDP_AUTO, nullptr, 0, 0,
	WC_BUILD_DEPOT, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_hangar_widgets
);

static void ShowHangarPicker(Window *parent)
{
	new BuildHangarWindow(_build_hangar_desc, parent);
}

struct BuildHeliportWindow : public PickerWindowBase {
	BuildHeliportWindow(WindowDesc &desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->CreateNestedTree();
		this->LowerWidget(WID_BHW_NE + _rotation_dir);
		this->FinishInitNested(TRANSPORT_AIR);
	}

	uint GetHeliportSpriteHeight() const { return 91; }

	void UpdateWidgetSize(int widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (!IsInsideMM(widget, WID_BHW_NE, WID_BHW_NW + 1)) return;

		size.width  = ScaleGUITrad(64) + 2;
		size.height = ScaleGUITrad(GetHeliportSpriteHeight()) + 2;
	}

	void DrawWidget(const Rect &r, int widget) const override
	{
		if (!IsInsideMM(widget, WID_BHW_NE, WID_BHW_NW + 1)) return;

		int x = r.left + 1 + ScaleGUITrad(TILE_PIXELS - 1);
		/* Height of depot sprite in OpenGFX is TILE_PIXELS + GetHangarSpriteHeight(). */
		int y = r.bottom - ScaleGUITrad(TILE_PIXELS - 1);

		SpriteID ground = GetAirTypeInfo(_cur_airtype)->base_sprites.ground;
		DiagDirection dir = (DiagDirection)(widget - WID_BHW_NE + DIAGDIR_NE);
		PaletteID palette = COMPANY_SPRITE_COLOUR(_local_company);
		extern const DrawTileSprites _airport_heliports[];
		const DrawTileSprites *dts = &_airport_heliports[0];
		DrawSprite(ground, PAL_NONE, x, y);
		DrawRailTileSeqInGUI(x, y, dts, ground + dir, 0, palette);
	}

	void OnClick([[maybe_unused]] Point pt, int widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_BHW_NW:
			case WID_BHW_NE:
			case WID_BHW_SW:
			case WID_BHW_SE:
				this->RaiseWidget(WID_BHW_NE + _rotation_dir);
				_rotation_dir = (DiagDirection)(widget - WID_BHW_NE);
				this->LowerWidget(WID_BHW_NE + _rotation_dir);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->SetDirty();
				break;

			default:
				break;
		}
	}
};

static const NWidgetPart _nested_build_heliport_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_BHW_CAPTION), SetDataTip(STR_BUILD_HELIPORT_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_HORIZONTAL_LTR), SetPIP(0, WidgetDimensions::unscaled.hsep_normal, 0), SetPIPRatio(1, 0, 1), SetPadding(WidgetDimensions::unscaled.picker),
			NWidget(NWID_VERTICAL), SetPIP(0, WidgetDimensions::unscaled.vsep_normal, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_NW), SetDataTip(0x0, STR_BUILD_HELIPORT_ORIENTATION_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_SW), SetDataTip(0x0, STR_BUILD_HELIPORT_ORIENTATION_TOOLTIP),
			EndContainer(),
			NWidget(NWID_VERTICAL), SetPIP(0, WidgetDimensions::unscaled.vsep_normal, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_NE), SetDataTip(0x0, STR_BUILD_HELIPORT_ORIENTATION_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BHW_SE), SetDataTip(0x0, STR_BUILD_HELIPORT_ORIENTATION_TOOLTIP),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _build_heliport_desc(
	WDP_AUTO, nullptr, 0, 0,
	WC_BUILD_HELIPORT, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_heliport_widgets
);

static void ShowHeliportPicker(Window *parent)
{
	new BuildHeliportWindow(_build_heliport_desc, parent);
}

struct BuildAirportInfraNoCatchWindow : public PickerWindowBase {
	BuildAirportInfraNoCatchWindow(WindowDesc &desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->CreateNestedTree();
		this->LowerWidget(WID_BAINC_FLAG + (BuildAirportInfrastructureNoCatchmentWidgets)_selected_infra_nocatch);
		this->FinishInitNested(TRANSPORT_AIR);
	}

	uint GetHeliportSpriteHeight() const { return 97; }

	void UpdateWidgetSize(int widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (!IsInsideMM(widget, WID_BAINC_FLAG, WID_BAINC_EMPTY + 1)) return;

		size.width  = ScaleGUITrad(64) + 2;
		size.height = ScaleGUITrad(GetHeliportSpriteHeight()) + 2;
	}

	void DrawWidget(const Rect &r, int widget) const override
	{
		if (!IsInsideMM(widget, WID_BAINC_FLAG, WID_BAINC_EMPTY + 1)) return;

		int x = r.left + 1 + ScaleGUITrad(TILE_PIXELS - 1);
		int y = r.bottom - ScaleGUITrad(TILE_PIXELS - 1);

		SpriteID ground = GetAirTypeInfo(_cur_airtype)->base_sprites.ground;
		PaletteID palette = COMPANY_SPRITE_COLOUR(_local_company);
		extern const DrawTileSprites _airtype_display_datas[];
		extern const DrawTileSprites _airtype_display_datas_radar[];
		extern const DrawTileSprites _airtype_display_datas_tower[];
		extern const DrawTileSprites _airtype_display_datas_transmitter[];
		const DrawTileSprites *dts = nullptr;
		DrawSprite(ground, PAL_NONE, x, y);

		switch (widget) {
			case WID_BAINC_FLAG: {
				extern const DrawTileSprites _airtype_display_datas_flag_NE[];
				extern const DrawTileSprites _airtype_display_datas_flag_SE[];
				extern const DrawTileSprites _airtype_display_datas_flag_SW[];
				extern const DrawTileSprites _airtype_display_datas_flag_NW[];

				const DrawTileSprites *flags[4] = {
						_airtype_display_datas_flag_NE,
						_airtype_display_datas_flag_SE,
						_airtype_display_datas_flag_SW,
						_airtype_display_datas_flag_NW,
				};

				ground = 0;
				dts = flags[_selected_infra_nocatch_rotation];
				break;
			}
			case WID_BAINC_RADAR:
				ground = 0;
				dts = &_airtype_display_datas_radar[2];
				break;
			case WID_BAINC_TOWER:
				dts = &_airtype_display_datas_tower[_selected_infra_nocatch_rotation];
				break;
			case WID_BAINC_TRANSMITTER:
				dts = &_airtype_display_datas_transmitter[_selected_infra_nocatch_rotation];
				break;

			case WID_BAINC_EMPTY:
			case WID_BAINC_PIER:
				dts = &_airtype_display_datas[(widget - WID_BAINC_FLAG + ATTG_NO_CATCH_FLAG) * 4 + _selected_infra_nocatch_rotation];
				break;
			default:
				NOT_REACHED();
		}
		if (dts != nullptr) DrawRailTileSeqInGUI(x, y, dts, ground, 0, palette);
	}

	void SetStringParameters(int widget) const override
	{
		if (widget != WID_BAINC_ROTATION) return;

		SetDParam(0, STR_EMPTY);
		SetDParam(0, STR_AIRPORT_ROTATION_0 + _selected_infra_nocatch_rotation);
	}

	void OnClick([[maybe_unused]] Point pt, int widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_BAINC_FLAG:
			case WID_BAINC_TRANSMITTER:
			case WID_BAINC_TOWER:
			case WID_BAINC_RADAR:
			case WID_BAINC_PIER:
			case WID_BAINC_EMPTY:
				this->RaiseWidget(WID_BAINC_FLAG + (BuildAirportInfrastructureNoCatchmentWidgets)_selected_infra_nocatch);
				_selected_infra_nocatch = (AirportTiles)(widget - WID_BAINC_FLAG);
				this->LowerWidget(WID_BAINC_FLAG + (BuildAirportInfrastructureNoCatchmentWidgets)_selected_infra_nocatch);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->SetDirty();
				break;

			case WID_BAINC_ROTATION_DECREASE:
				_selected_infra_nocatch_rotation = ((int)_selected_infra_nocatch_rotation + 3) % 4;
				this->SetDirty();
				break;

			case WID_BAINC_ROTATION_INCREASE:
				_selected_infra_nocatch_rotation = (_selected_infra_nocatch_rotation + 1) % 4;
				this->SetDirty();
				break;

			default:
				break;
		}
	}
};

static const NWidgetPart _nested_build_airport_infra_no_catch_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_BAINC_CAPTION), SetDataTip(STR_BUILD_AIRPORT_INFRA_NO_CATCH_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		/* Graphics */
		NWidget(NWID_HORIZONTAL_LTR), SetPIP(0, WidgetDimensions::unscaled.hsep_normal, 0), SetPIPRatio(1, 0, 1), SetPadding(WidgetDimensions::unscaled.picker),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAINC_FLAG), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAINC_TRANSMITTER), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAINC_TOWER), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAINC_RADAR), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAINC_PIER), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAINC_EMPTY), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
		EndContainer(),
		/* Rotation */
		NWidget(NWID_HORIZONTAL),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0), SetFill(1, 0),
			NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_BAINC_ROTATION_DECREASE), SetMinimalSize(12, 0), SetDataTip(AWV_DECREASE, STR_NULL),
			NWidget(WWT_LABEL, COLOUR_GREY, WID_BAINC_ROTATION), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_NULL),
			NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_BAINC_ROTATION_INCREASE), SetMinimalSize(12, 0), SetDataTip(AWV_INCREASE, STR_NULL),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3), SetFill(1, 0),
	EndContainer(),
};

static WindowDesc _build_airport_infra_no_catch_desc(
	WDP_ALIGN_TOOLBAR, nullptr, 0, 0,
	WC_BUILD_AIRPORT_INFRASTRUCTURE, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_airport_infra_no_catch_widgets
);

static void ShowAirportInfraNoCatchPicker(Window *parent)
{
	new BuildAirportInfraNoCatchWindow(_build_airport_infra_no_catch_desc, parent);
}

struct BuildAirportInfraWithCatchWindow : public PickerWindowBase {
	BuildAirportInfraWithCatchWindow(WindowDesc &desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->CreateNestedTree();
		this->LowerWidget(WID_BAIWC_BUILDING_1 + (BuildAirportInfrastructureNoCatchmentWidgets)_selected_infra_catch);
		this->FinishInitNested(TRANSPORT_AIR);
	}

	uint GetHeliportSpriteHeight() const { return 91; }

	void UpdateWidgetSize(int widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (!IsInsideMM(widget, WID_BAIWC_BUILDING_1, WID_BAIWC_BUILDING_TERMINAL + 1)) return;

		size.width  = ScaleGUITrad(64) + 2;
		size.height = ScaleGUITrad(GetHeliportSpriteHeight()) + 2;
	}

	void DrawWidget(const Rect &r, int widget) const override
	{
		if (!IsInsideMM(widget, WID_BAIWC_BUILDING_1, WID_BAIWC_BUILDING_TERMINAL + 1)) return;

		int x = r.left + 1 + ScaleGUITrad(TILE_PIXELS - 1);
		int y = r.bottom - ScaleGUITrad(TILE_PIXELS - 1);

		SpriteID ground = GetAirTypeInfo(_cur_airtype)->base_sprites.ground;
		PaletteID palette = COMPANY_SPRITE_COLOUR(_local_company);
		extern const DrawTileSprites _airtype_display_datas[];
		const DrawTileSprites *dts = &_airtype_display_datas[
				(widget - WID_BAIWC_BUILDING_1 + ATTG_WITH_CATCH_BUILDING_1) * 4 + _selected_infra_catch_rotation];
		DrawSprite(ground, PAL_NONE, x, y);
		DrawRailTileSeqInGUI(x, y, dts, ground, 0, palette);
	}

	void SetStringParameters(int widget) const override
	{
		if (widget != WID_BAIWC_ROTATION) return;

		SetDParam(0, STR_EMPTY);
		SetDParam(0, STR_AIRPORT_ROTATION_0 + _selected_infra_catch_rotation);
	}

	void OnClick([[maybe_unused]] Point pt, int widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_BAIWC_BUILDING_1:
			case WID_BAIWC_BUILDING_2:
			case WID_BAIWC_BUILDING_3:
			case WID_BAIWC_BUILDING_FLAT:
			case WID_BAIWC_BUILDING_TERMINAL:
				this->RaiseWidget(WID_BAIWC_BUILDING_1 + (BuildAirportInfrastructureNoCatchmentWidgets)_selected_infra_catch);
				_selected_infra_catch = (AirportTiles)(widget - WID_BAIWC_BUILDING_1);
				this->LowerWidget(WID_BAIWC_BUILDING_1 + (BuildAirportInfrastructureNoCatchmentWidgets)_selected_infra_catch);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->SetDirty();
				break;

			case WID_BAIWC_ROTATION_DECREASE:
				_selected_infra_catch_rotation = (_selected_infra_catch_rotation + 3) % 4;
				this->SetDirty();
				break;

			case WID_BAIWC_ROTATION_INCREASE:
				_selected_infra_catch_rotation = (_selected_infra_catch_rotation + 1) % 4;
				this->SetDirty();
				break;

			default:
				break;
		}
	}
};

static const NWidgetPart _nested_build_airport_infra_with_catch_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_BAIWC_CAPTION), SetDataTip(STR_BUILD_AIRPORT_INFRA_WITH_CATCH_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		/* Graphics */
		NWidget(NWID_HORIZONTAL_LTR), SetPIP(0, WidgetDimensions::unscaled.hsep_normal, 0), SetPIPRatio(1, 0, 1), SetPadding(WidgetDimensions::unscaled.picker),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAIWC_BUILDING_1), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAIWC_BUILDING_2), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAIWC_BUILDING_3), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAIWC_BUILDING_FLAT), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BAIWC_BUILDING_TERMINAL), SetDataTip(0x0, STR_BUILD_AIRPORT_INFRA_TOOLTIP),
		EndContainer(),
		/* Rotation */
		NWidget(NWID_HORIZONTAL),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0), SetFill(1, 0),
			NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_BAIWC_ROTATION_DECREASE), SetMinimalSize(12, 0), SetDataTip(AWV_DECREASE, STR_NULL),
			NWidget(WWT_LABEL, COLOUR_GREY, WID_BAIWC_ROTATION), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_NULL),
			NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_BAIWC_ROTATION_INCREASE), SetMinimalSize(12, 0), SetDataTip(AWV_INCREASE, STR_NULL),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3), SetFill(1, 0),
	EndContainer(),
};

static WindowDesc _build_airport_infra_with_catch_desc(
	WDP_ALIGN_TOOLBAR, nullptr, 0, 0,
	WC_BUILD_AIRPORT_INFRASTRUCTURE, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_airport_infra_with_catch_widgets
);

static void ShowAirportInfraWithCatchPicker(Window *parent)
{
	new BuildAirportInfraWithCatchWindow(_build_airport_infra_with_catch_desc, parent);
}


/** Set the initial (default) airtype to use */
static void SetDefaultAirGui()
{
	if (_local_company == COMPANY_SPECTATOR || !Company::IsValidID(_local_company)) return;

	extern AirType _last_built_airtype;
	AirType rt;
	switch (_settings_client.gui.default_air_type) {
		case 2: {
			/* Find the most used air type */
			uint count[AIRTYPE_END];
			memset(count, 0, sizeof(count));
			for (TileIndex t = 0; t < Map::Size(); t++) {
				if (IsAirportTile(t) && GetTileOwner(t) == _local_company) {
					count[GetAirType(t)]++;
				}
			}

			rt = static_cast<AirType>(std::max_element(count + AIRTYPE_BEGIN, count + AIRTYPE_END) - count);
			if (count[rt] > 0) break;

			/* No air, just get the first available one */
			[[fallthrough]];
		}
		case 0: {
			/* Use first available type */
			std::vector<AirType>::const_iterator it = std::find_if(_sorted_airtypes.begin(), _sorted_airtypes.end(),
					[](AirType r){ return HasAirTypeAvail(_local_company, r); });
			rt = it != _sorted_airtypes.end() ? *it : AIRTYPE_BEGIN;
			break;
		}
		case 1: {
			/* Use last available type */
			std::vector<AirType>::const_reverse_iterator it = std::find_if(_sorted_airtypes.rbegin(), _sorted_airtypes.rend(),
					[](AirType r){ return HasAirTypeAvail(_local_company, r); });
			rt = it != _sorted_airtypes.rend() ? *it : AIRTYPE_BEGIN;
			break;
		}
		default:
			NOT_REACHED();
	}

	_last_built_airtype = _cur_airtype = rt;
	BuildAirToolbarWindow *w = dynamic_cast<BuildAirToolbarWindow *>(FindWindowById(WC_BUILD_TOOLBAR, TRANSPORT_AIR));
	if (w != nullptr) w->ModifyAirType(_cur_airtype);
}

/**
 * Create a drop down list for all the air types of the local company.
 * @param for_replacement Whether this list is for the replacement window.
 * @param all_option Whether to add an 'all types' item.
 * @return The populated and sorted #DropDownList.
 */
DropDownList GetAirTypeDropDownList(bool for_replacement, bool all_option)
{
	AirTypes used_airtypes;
	AirTypes avail_airtypes;

	const Company *c = Company::Get(_local_company);

	/* Find the used airtypes. */
	if (for_replacement) {
		avail_airtypes = GetCompanyAirTypes(c->index, false);
		used_airtypes  = GetAirTypes(false);
	} else {
		avail_airtypes = c->avail_airtypes;
		used_airtypes  = GetAirTypes(true);
	}

	DropDownList list;

	if (all_option) {
		list.push_back(MakeDropDownListStringItem(STR_REPLACE_ALL_AIRTYPE, INVALID_AIRTYPE, false));
	}

	Dimension d = { 0, 0 };
	/* Get largest icon size, to ensure text is aligned on each menu item. */
	if (!for_replacement) {
		for (const auto &at : _sorted_airtypes) {
			if (!HasBit(used_airtypes, at)) continue;
			const AirTypeInfo *ati = GetAirTypeInfo(at);
			d = maxdim(d, GetSpriteSize(ati->gui_sprites.build_helipad));
		}
	}

	for (const auto &at : _sorted_airtypes) {
		/* If it's not used ever, don't show it to the user. */
		if (!HasBit(used_airtypes, at)) continue;

		const AirTypeInfo *ati = GetAirTypeInfo(at);
		SetDParam(0, ati->strings.menu_text);
		SetDParam(1, ati->max_speed);

		if (for_replacement) {
			list.push_back(MakeDropDownListStringItem(ati->strings.replace_text, at, !HasBit(avail_airtypes, at)));
		} else {
			StringID str = ati->max_speed > 0 ? STR_TOOLBAR_RAILTYPE_VELOCITY : STR_JUST_STRING;
			auto iconitem = MakeDropDownListIconItem(d, ati->gui_sprites.build_helipad, PAL_NONE, str, at, !HasBit(avail_airtypes, at));
			list.push_back(std::move(iconitem));
		}
	}

	if (list.empty()) {
		/* Empty dropdowns are not allowed */
		list.push_back(MakeDropDownListStringItem(STR_NONE, INVALID_AIRTYPE, true));
	}

	return list;
}

void InitializeAirportGui()
{
	SetDefaultAirGui();

	_selected_airport_class = APC_BEGIN;
	_selected_airport_index = -1;
	_selected_infra_catch_rotation = 0;
	_selected_infra_catch = ATTG_DEFAULT_GFX;
	_selected_infra_nocatch_rotation = 0;
	_selected_infra_nocatch = ATTG_DEFAULT_GFX;
}

