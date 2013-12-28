/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ship_cmd.cpp Handling of ships. */

#include "stdafx.h"
#include "ship.h"
#include "dock_base.h"
#include "landscape.h"
#include "timetable.h"
#include "news_func.h"
#include "company_func.h"
#include "pathfinder/npf/npf_func.h"
#include "depot_base.h"
#include "station_base.h"
#include "newgrf_engine.h"
#include "pathfinder/yapf/yapf.h"
#include "newgrf_sound.h"
#include "spritecache.h"
#include "strings_func.h"
#include "window_func.h"
#include "date_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "ai/ai.hpp"
#include "game/game.hpp"
#include "pathfinder/opf/opf_ship.h"
#include "engine_base.h"
#include "company_base.h"
#include "tunnelbridge_map.h"
#include "zoom_func.h"
#include "pbs_water.h"
#include "pathfinder/follow_track.hpp"

#include "table/strings.h"

#include "safeguards.h"

/**
 * Determine the effective #WaterClass for a ship travelling on a tile.
 * @param tile Tile of interest
 * @return the waterclass to be used by the ship.
 */
WaterClass GetEffectiveWaterClass(TileIndex tile)
{
	if (HasTileWaterClass(tile)) return GetWaterClass(tile);
	if (IsTileType(tile, MP_TUNNELBRIDGE)) {
		assert(GetTunnelBridgeTransportType(tile) == TRANSPORT_WATER);
		return WATER_CLASS_CANAL;
	}
	if (IsTileType(tile, MP_RAILWAY)) {
		assert(GetRailGroundType(tile) == RAIL_GROUND_WATER);
		return WATER_CLASS_SEA;
	}
	NOT_REACHED();
}

static const uint16 _ship_sprites[] = {0x0E5D, 0x0E55, 0x0E65, 0x0E6D};

template <>
bool IsValidImageIndex<VEH_SHIP>(uint8 image_index)
{
	return image_index < lengthof(_ship_sprites);
}

static inline TrackBits GetTileShipTrackStatus(TileIndex tile)
{
	return TrackStatusToTrackBits(GetTileTrackStatus(tile, TRANSPORT_WATER, 0));
}

static void GetShipIcon(EngineID engine, EngineImageType image_type, VehicleSpriteSeq *result)
{
	const Engine *e = Engine::Get(engine);
	uint8 spritenum = e->u.ship.image_index;

	if (is_custom_sprite(spritenum)) {
		GetCustomVehicleIcon(engine, DIR_W, image_type, result);
		if (result->IsValid()) return;

		spritenum = e->original_image_index;
	}

	assert(IsValidImageIndex<VEH_SHIP>(spritenum));
	result->Set(DIR_W + _ship_sprites[spritenum]);
}

void DrawShipEngine(int left, int right, int preferred_x, int y, EngineID engine, PaletteID pal, EngineImageType image_type)
{
	VehicleSpriteSeq seq;
	GetShipIcon(engine, image_type, &seq);

	Rect rect;
	seq.GetBounds(&rect);
	preferred_x = Clamp(preferred_x,
			left - UnScaleGUI(rect.left),
			right - UnScaleGUI(rect.right));

	seq.Draw(preferred_x, y, pal, pal == PALETTE_CRASH);
}

/**
 * Get the size of the sprite of a ship sprite heading west (used for lists).
 * @param engine The engine to get the sprite from.
 * @param[out] width The width of the sprite.
 * @param[out] height The height of the sprite.
 * @param[out] xoffs Number of pixels to shift the sprite to the right.
 * @param[out] yoffs Number of pixels to shift the sprite downwards.
 * @param image_type Context the sprite is used in.
 */
void GetShipSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type)
{
	VehicleSpriteSeq seq;
	GetShipIcon(engine, image_type, &seq);

	Rect rect;
	seq.GetBounds(&rect);

	width  = UnScaleGUI(rect.right - rect.left + 1);
	height = UnScaleGUI(rect.bottom - rect.top + 1);
	xoffs  = UnScaleGUI(rect.left);
	yoffs  = UnScaleGUI(rect.top);
}

void Ship::GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const
{
	uint8 spritenum = this->spritenum;

	if (is_custom_sprite(spritenum)) {
		GetCustomVehicleSprite(this, direction, image_type, result);
		if (result->IsValid()) return;

		spritenum = this->GetEngine()->original_image_index;
	}

	assert(IsValidImageIndex<VEH_SHIP>(spritenum));
	result->Set(_ship_sprites[spritenum] + direction);
}

static const Depot *FindClosestShipDepot(const Vehicle *v, uint max_distance)
{
	/* Find the closest depot */
	const Depot *depot;
	const Depot *best_depot = NULL;
	/* If we don't have a maximum distance, i.e. distance = 0,
	 * we want to find any depot so the best distance of no
	 * depot must be more than any correct distance. On the
	 * other hand if we have set a maximum distance, any depot
	 * further away than max_distance can safely be ignored. */
	uint best_dist = max_distance == 0 ? UINT_MAX : max_distance + 1;

	FOR_ALL_DEPOTS(depot) {
		if (depot->veh_type == VEH_SHIP && depot->company == v->owner) {
			uint dist = DistanceManhattan(depot->xy, v->tile);
			if (dist < best_dist) {
				best_dist = dist;
				best_depot = depot;
			}
		}
	}

	return best_depot;
}

static void CheckIfShipNeedsService(Vehicle *v)
{
	if (Company::Get(v->owner)->settings.vehicle.servint_ships == 0 || !v->NeedsAutomaticServicing()) return;
	if (v->IsChainInDepot()) {
		VehicleServiceInDepot(v);
		return;
	}

	uint max_distance;
	switch (_settings_game.pf.pathfinder_for_ships) {
		case VPF_OPF:  max_distance = 12; break;
		case VPF_NPF:  max_distance = _settings_game.pf.npf.maximum_go_to_depot_penalty  / NPF_TILE_LENGTH;  break;
		case VPF_YAPF: max_distance = _settings_game.pf.yapf.maximum_go_to_depot_penalty / YAPF_TILE_LENGTH; break;
		default: NOT_REACHED();
	}

	const Depot *depot = FindClosestShipDepot(v, max_distance);

	if (depot == NULL) {
		if (v->current_order.IsType(OT_GOTO_DEPOT)) {
			v->current_order.MakeDummy();
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		}
		return;
	}

	v->current_order.MakeGoToDepot(depot->index, ODTFB_SERVICE);
	v->dest_tile = depot->GetBestDepotTile(v);
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
}

/**
 * Update the caches of this ship.
 */
void Ship::UpdateCache()
{
	const ShipVehicleInfo *svi = ShipVehInfo(this->engine_type);

	/* Get speed fraction for the current water type. Aqueducts are always canals. */
	bool is_ocean = GetEffectiveWaterClass(this->tile) == WATER_CLASS_SEA;
	uint raw_speed = GetVehicleProperty(this, PROP_SHIP_SPEED, svi->max_speed);
	this->vcache.cached_max_speed = svi->ApplyWaterClassSpeedFrac(raw_speed, is_ocean);

	/* Update cargo aging period. */
	this->vcache.cached_cargo_age_period = GetVehicleProperty(this, PROP_SHIP_CARGO_AGE_PERIOD, EngInfo(this->engine_type)->cargo_age_period);

	this->UpdateVisualEffect();
}

Money Ship::GetRunningCost() const
{
	const Engine *e = this->GetEngine();
	uint cost_factor = GetVehicleProperty(this, PROP_SHIP_RUNNING_COST_FACTOR, e->u.ship.running_cost);
	return GetPrice(PR_RUNNING_SHIP, cost_factor, e->GetGRF());
}

void Ship::OnNewDay()
{
	if ((++this->day_counter & 7) == 0) {
		DecreaseVehicleValue(this);
	}

	CheckVehicleBreakdown(this);
	AgeVehicle(this);
	CheckIfShipNeedsService(this);

	CheckOrders(this);

	if (this->running_ticks == 0) return;

	CommandCost cost(EXPENSES_SHIP_RUN, this->GetRunningCost() * this->running_ticks / (DAYS_IN_YEAR * DAY_TICKS));

	this->profit_this_year -= cost.GetCost();
	this->running_ticks = 0;

	SubtractMoneyFromCompanyFract(this->owner, cost);

	SetWindowDirty(WC_VEHICLE_DETAILS, this->index);
	/* we need this for the profit */
	SetWindowClassesDirty(WC_SHIPS_LIST);
}

Trackdir Ship::GetVehicleTrackdir() const
{
	if (this->vehstatus & VS_CRASHED) return INVALID_TRACKDIR;

	if (this->IsInDepot()) {
		/* Only old depots need it. */
		/* We'll assume the ship is facing outwards. */
		if (this->state == TRACK_BIT_DEPOT) return DiagDirToDiagTrackdir(GetShipDepotDirection(this->tile));
	}

	if (this->state == TRACK_BIT_WORMHOLE || this->IsInDepot()) {
		/* ship on aqueduct, so just use his direction and assume a diagonal track */
		return DiagDirToDiagTrackdir(DirToDiagDir(this->direction));
	}

	return TrackDirectionToTrackdir(FindFirstTrack(this->state), this->direction);
}

Ship::~Ship()
{
	if (CleaningPool()) return;

	if (this->state != TRACK_BIT_DEPOT &&
			HasWaterTracksReserved(this->tile, TrackToTrackBits(TrackdirToTrack(this->GetVehicleTrackdir())))) {
		/* Lift reservation for that ship when going bankrupt. */
		LiftPathReservation(this->tile, this->GetVehicleTrackdir());
	}

	this->PreDestructor();
}

/**
 * Mark the ship as stuck.
 * @param stop Whether the velocity must be set to 0.
 * @param ticks How many ticks the ship must be stopped.
 */
void Ship::MarkShipAsStuck(bool stop, uint ticks)
{
	assert(!this->stuck);

	/* Set the ship stuck. */
	this->stuck = true;
	this->wait_counter = ticks;

	/* Stop ship. */
	if (stop) {
		this->cur_speed = 0;
		this->subspeed = 0;
	}

	SetWindowWidgetDirty(WC_VEHICLE_VIEW, this->index, WID_VV_START_STOP);
}

/** Unstuck the ship. */
void Ship::Unstuck() {
	this->stuck = false;
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, this->index, WID_VV_START_STOP);
}

void Ship::MarkDirty()
{
	this->colourmap = PAL_NONE;
	this->UpdateViewport(true, false);
	this->UpdateCache();
}

static void PlayShipSound(const Vehicle *v)
{
	if (!PlayVehicleSound(v, VSE_START)) {
		SndPlayVehicleFx(ShipVehInfo(v->engine_type)->sfx, v);
	}
}

void Ship::PlayLeaveStationSound() const
{
	PlayShipSound(this);
}

/**
 * Of all the docks a station has, return the best destination for a ship.
 * @param v The ship.
 * @param st Station the ship \a v is heading for.
 * @return The free and closest (if none is free, just closest) dock of station \a st to ship \a v.
 */
TileIndex GetBestDock(const Ship *v, const Station *st)
{
	assert(st != NULL && st->HasFacilities(FACIL_DOCK) && st->docks != NULL);
	if (st->docks->next == NULL) return st->docks->flat;

	Dock *best_dock = NULL;
	bool free_found = false;
	uint best_distance = UINT_MAX;

	for (Dock *dock = st->docks; dock != NULL; dock = dock->next) {
		bool is_new_free = !HasWaterTrackReservation(dock->flat);
		uint new_distance = DistanceManhattan(v->tile, dock->flat);
		if (((free_found == is_new_free) && new_distance < best_distance) || (!free_found && is_new_free)) {
			best_dock = dock;
			best_distance = new_distance;
			free_found |= is_new_free;
		}
	}

	assert(best_dock != NULL);
	return best_dock->flat;
}

TileIndex Ship::GetOrderStationLocation(StationID station)
{
	if (station == this->last_station_visited) this->last_station_visited = INVALID_STATION;

	const Station *st = Station::Get(station);
	if (st->HasFacilities(FACIL_DOCK)) {
		if (st->docks == NULL) return st->xy; // A buoy
		else return GetBestDock(this, st);
	} else {
		this->IncrementRealOrderIndex();
		return 0;
	}
}

void Ship::UpdateDeltaXY(Direction direction)
{
	static const int8 _delta_xy_table[8][4] = {
		/* y_extent, x_extent, y_offs, x_offs */
		{ 6,  6,  -3,  -3}, // N
		{ 6, 32,  -3, -16}, // NE
		{ 6,  6,  -3,  -3}, // E
		{32,  6, -16,  -3}, // SE
		{ 6,  6,  -3,  -3}, // S
		{ 6, 32,  -3, -16}, // SW
		{ 6,  6,  -3,  -3}, // W
		{32,  6, -16,  -3}, // NW
	};

	const int8 *bb = _delta_xy_table[direction];
	this->x_offs        = bb[3];
	this->y_offs        = bb[2];
	this->x_extent      = bb[1];
	this->y_extent      = bb[0];
	this->z_extent      = 6;
}

static bool CheckPlaceShipOnDepot(TileIndex tile)
{
	assert(IsShipDepotTile(tile));

	/* Check we can reserve the depot track */
	if (HasWaterTrackReservation(tile)) return false;

	Axis axis = GetShipDepotAxis(tile);
	DiagDirection exit_dir = AxisToDiagDir(axis);

	if (!IsWaterPositionFree(tile, TrackExitdirToTrackdir(AxisToTrack(axis), exit_dir))) return false;
	if (!IsWaterPositionFree(tile, TrackExitdirToTrackdir(AxisToTrack(axis), ReverseDiagDir(exit_dir)))) return false;

	return true;
}

void HandleShipEnterDepot(Ship *v)
{
	assert(IsShipDepotTile(v->tile));

	if (IsBigDepot(v->tile)) {
		v->state |= TRACK_BIT_DEPOT;
		v->cur_speed = 0;
		v->UpdateCache();
		v->UpdateViewport(true, true);
		SetWindowClassesDirty(WC_SHIPS_LIST);
		SetWindowDirty(WC_VEHICLE_VIEW, v->index);

		InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
		v->StartService();
	} else {
		/* Remove reservations on normal depots. */
		SetWaterTrackReservation(v->tile, TrackdirToTrack(v->GetVehicleTrackdir()), false);
		VehicleEnterDepot(v);
	}
}

static bool CheckShipLeaveDepot(Ship *v)
{
	if (!v->IsChainInDepot()) return false;

	/* We are leaving a depot, but have to go to the exact same one; re-enter */
	if (v->current_order.IsType(OT_GOTO_DEPOT) &&
			IsShipDepotTile(v->tile) && GetDepotIndex(v->tile) == v->current_order.GetDestination()) {
		VehicleEnterDepot(v);
		return true;
	}

	TileIndex tile = v->tile;
	Axis axis = GetShipDepotAxis(tile);

	/* Check we can reserve the depot track */
	if (HasWaterTrackReservation(tile)) return true;

	DiagDirection south_dir = AxisToDiagDir(axis);
	DiagDirection north_dir = ReverseDiagDir(south_dir);

	TileIndex north_neighbour = TILE_ADD(tile, TileOffsByDiagDir(north_dir));
	TrackBits north_tracks = DiagdirReachesTracks(north_dir) & GetTileShipTrackStatus(north_neighbour);

	if (!IsWaterPositionFree(tile, TrackExitdirToTrackdir(AxisToTrack(axis), south_dir))) return true;
	if (!IsWaterPositionFree(tile, TrackExitdirToTrackdir(AxisToTrack(axis), north_dir))) return true;

	/* Ask pathfinder for best direction */
	bool reverse = false;
	bool path_found;
	switch (_settings_game.pf.pathfinder_for_ships) {
		case VPF_OPF: reverse = OPFShipChooseTrack(v, north_neighbour, north_dir, north_tracks, path_found) == INVALID_TRACK; break; // OPF always allows reversing
		case VPF_NPF: reverse = NPFShipCheckReverse(v); break;
		case VPF_YAPF: reverse = YapfShipCheckReverse(v); break;
		default: NOT_REACHED();
	}

	/* Leave towards south if reverse. */
	v->direction = DiagDirToDir(reverse ? south_dir : north_dir);

	if (!SetWaterTrackReservation(tile, (Track)axis, true)) NOT_REACHED();
	v->state = AxisToTrackBits(axis);
	v->vehstatus &= ~VS_HIDDEN;

	v->cur_speed = 0;
	v->UpdateViewport(true, true);
	SetWindowDirty(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));

	PlayShipSound(v);
	VehicleServiceInDepot(v);
	InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
	SetWindowClassesDirty(WC_SHIPS_LIST);

	return false;
}

static bool ShipAccelerate(Vehicle *v)
{
	uint spd;
	byte t;

	spd = min(v->cur_speed + 1, v->vcache.cached_max_speed);
	spd = min(spd, v->current_order.GetMaxSpeed() * 2);

	/* updates statusbar only if speed have changed to save CPU time */
	if (spd != v->cur_speed) {
		v->cur_speed = spd;
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}

	/* Convert direction-independent speed into direction-dependent speed. (old movement method) */
	spd = v->GetOldAdvanceSpeed(spd);

	if (spd == 0) return false;
	if ((byte)++spd == 0) return true;

	v->progress = (t = v->progress) - (byte)spd;

	return (t < v->progress);
}

/**
 * Ship arrives at a dock. If it is the first time, send out a news item.
 * @param v  Ship that arrived.
 * @param st Station being visited.
 */
static void ShipArrivesAt(const Vehicle *v, Station *st)
{
	/* Check if station was ever visited before */
	if (!(st->had_vehicle_of_type & HVOT_SHIP)) {
		st->had_vehicle_of_type |= HVOT_SHIP;

		SetDParam(0, st->index);
		AddVehicleNewsItem(
			STR_NEWS_FIRST_SHIP_ARRIVAL,
			(v->owner == _local_company) ? NT_ARRIVAL_COMPANY : NT_ARRIVAL_OTHER,
			v->index,
			st->index
		);
		AI::NewEvent(v->owner, new ScriptEventStationFirstVehicle(st->index, v->index));
		Game::NewEvent(new ScriptEventStationFirstVehicle(st->index, v->index));
	}
}


/**
 * Runs the pathfinder to choose a track to continue along.
 *
 * @param v Ship to navigate
 * @param tile Tile, the ship is about to enter
 * @param enterdir Direction of entering
 * @param tracks Available track choices on \a tile
 * @return Track to choose, or INVALID_TRACK when to reverse.
 */
static Track ChooseShipTrack(Ship *v, TileIndex tile, DiagDirection enterdir, TrackBits tracks)
{
	assert(IsValidDiagDirection(enterdir));

	/* Before choosing a track, if close to the destination station or depot (not an oil rig)... */

	if (WaterTrackMayExist(v->dest_tile) && DistanceManhattan(v->dest_tile, tile) <= 5) {
		if (v->current_order.IsType(OT_GOTO_STATION) && HasWaterTrackReservation(v->dest_tile) &&
				!IsTileType(v->dest_tile, MP_INDUSTRY)) {
			/* Get the closest and free dock if possible. */
			v->dest_tile = GetBestDock(v, Station::Get(v->current_order.GetDestination()));
		} else if (v->current_order.IsType(OT_GOTO_DEPOT) &&
				(!IsShipDepotTile(v->dest_tile) || HasWaterTrackReservation(v->dest_tile) ||
				HasWaterTrackReservation(GetOtherShipDepotTile(v->dest_tile)))) {
			v->dest_tile = Depot::Get(v->current_order.GetDestination())->GetBestDepotTile(v);
		}
	}

	bool path_found = true;
	Track track;
	switch (_settings_game.pf.pathfinder_for_ships) {
		case VPF_OPF: track = OPFShipChooseTrack(v, tile, enterdir, tracks, path_found); break;
		case VPF_NPF: track = NPFShipChooseTrack(v, tile, enterdir, tracks, path_found); break;
		case VPF_YAPF: track = YapfShipChooseTrack(v, tile, enterdir, tracks, path_found); break;
		default: NOT_REACHED();
	}

	/* A path is being found */
	v->HandlePathfindingResult(path_found);

	if (track == INVALID_TRACK) return INVALID_TRACK;

	/* Check that the track is reserved. */
	if (!HasWaterTracksReserved(tile, TrackToTrackBits(track))) {
		/* Track couldn't be reserved, so if there are alternatives, take them. */
		track = FindFirstTrack(tracks);
		Trackdir trackdir = TrackEnterdirToTrackdir(track, enterdir);
		assert(IsWaterPositionFree(tile, trackdir));
		if (!SetWaterTrackReservation(tile, track, true)) NOT_REACHED();
	}

	return track;
}

static inline TrackBits GetAvailShipTracks(TileIndex tile, DiagDirection dir)
{
	return GetTileShipTrackStatus(tile) & DiagdirReachesTracks(dir);
}

static const byte _ship_subcoord[4][6][3] = {
	{
		{15, 8, 1},
		{ 0, 0, 0},
		{ 0, 0, 0},
		{15, 8, 2},
		{15, 7, 0},
		{ 0, 0, 0},
	},
	{
		{ 0, 0, 0},
		{ 8, 0, 3},
		{ 7, 0, 2},
		{ 0, 0, 0},
		{ 8, 0, 4},
		{ 0, 0, 0},
	},
	{
		{ 0, 8, 5},
		{ 0, 0, 0},
		{ 0, 7, 6},
		{ 0, 0, 0},
		{ 0, 0, 0},
		{ 0, 8, 4},
	},
	{
		{ 0, 0, 0},
		{ 8, 15, 7},
		{ 0, 0, 0},
		{ 8, 15, 6},
		{ 0, 0, 0},
		{ 7, 15, 0},
	}
};

/**
 * Stuff to do when a ship reaches a dock destination tile (not oil rig)
 * @param v ship that reaches its dock destination tile
 * @param t tile (dock tile)
 * @pre current order of ship is go to a station
 * @note must check if it is still the valid dock destination
 */
static void ShipReachesDockDestTile(Ship *v, TileIndex t) {
	Station *st = Station::Get(v->current_order.GetDestination());
	/* Check station has water facilities */
	if ((st->facilities & FACIL_DOCK) == 0) {
		/* If not, vehicle is lost */
		v->HandlePathfindingResult(false);
		return;
	}

	if (IsDockTile(t) && Station::GetByTile(t) == st) {
		/* Process station in the orderlist */
		v->last_station_visited = v->current_order.GetDestination();
		ShipArrivesAt(v, st);
		v->BeginLoading();
	} else {
		/* Destination station has water facilities
		 * but they have been moved to another tile */
		v->dest_tile = GetBestDock(v, st);
	}
}

/**
 * Check if after loading/unloading in a dock a ship neeeds reversing.
 * @param v Ship.
 */
bool ShipNeedsReversingInDock(Ship *v)
{
	DiagDirection dir = TrackdirToExitdir(v->GetVehicleTrackdir());
	TileIndex t = TileAddByDiagDir(v->tile, dir);
	if (!IsValidTile(t)) return true;
	TrackBits tracks = TrackStatusToTrackBits(GetTileTrackStatus(t, TRANSPORT_WATER, 0)) & DiagdirReachesTracks(dir);
	return tracks == TRACK_BIT_NONE;
}

/**
 * Get the z-height based on the water level of the tile.
 * @param tile Tile we want the z-height of.
 * @return z-height of the tile.
 */
int GetLockZ(TileIndex tile)
{
	int z;
	GetTilePixelSlope(tile, &z);
	return z + GetLockWaterLevel(tile);
}

static void ShipController(Ship *v)
{
	uint32 r;
	const byte *b;
	Direction dir;
	Track track;
	TrackBits tracks;

	v->tick_counter++;
	v->current_order_time++;

	if (v->HandleBreakdown()) return;

	if (v->vehstatus & VS_STOPPED) return;

	if (v->ContinueServicing()) return;

	if (v->IsStuck() && !v->TryUnblock()) return;

	ProcessOrders(v);
	v->HandleLoading();

	if (v->current_order.IsType(OT_LOADING)) return;

	if (CheckShipLeaveDepot(v)) return;

	v->ShowVisualEffect();

	if (!ShipAccelerate(v)) return;

	GetNewVehiclePosResult gp = GetNewVehiclePos(v);

	/* Lock controller. */
	if (v->lock != SLS_NO_LOCK) {
		if (v->IsStuck()) v->Unstuck();
		assert(IsLockTile(v->tile));

		switch (v->lock) {
			case SLS_PREPARING_LOCK: {
				assert(GetLockPart(v->tile) != LOCK_PART_MIDDLE);
				TileIndex middle = GetLockMiddleTile(v->tile);
				uint level = GetLockWaterLevel(v->tile);
				uint level_middle = GetLockWaterLevel(middle);
				if (level == level_middle) {
					v->lock = SLS_SHIP_ENTER;
					v->cur_speed = 0;
				} else {
					SetLockWaterLevel(middle, level_middle + (level_middle > level ? -1 : +1));
					v->MarkShipAsStuck(false, SHIP_PREPARE_LOCK_TICKS);
				}
				break;
			}
			case SLS_SHIP_ENTER:
				if (gp.new_tile != v->tile) {
					/* Do not let it enter next tile yet. */
					v->lock = SLS_SHIP_UPDOWN;
				} else {
					v->x_pos = gp.x;
					v->y_pos = gp.y;
					v->z_pos = GetSlopePixelZ(gp.x, gp.y);
				}
				break;
			case SLS_SHIP_UPDOWN: {
				assert(v->tile != gp.new_tile);
				assert(GetLockPart(v->tile) != LOCK_PART_MIDDLE);
				uint8 final_level = (GetLockPart(v->tile) == LOCK_PART_LOWER ? TILE_HEIGHT : 0);
				uint8 level = GetLockWaterLevel(v->tile);
				if (final_level != level) {
					level += level > final_level ? -1 : +1;
					SetLockWaterLevel(v->tile, level);
					SetLockWaterLevel(gp.new_tile, level);
					v->z_pos = GetLockZ(gp.new_tile);
					v->MarkShipAsStuck(false, SHIP_UPDOWN_LOCK_TICKS);
				} else {
					v->tile = gp.new_tile;
					v->lock = SLS_SHIP_MOVE;
					v->cur_speed = 0;
				}
				break;
			}
			case SLS_SHIP_MOVE: {
				v->x_pos = gp.x;
				v->y_pos = gp.y;
				v->z_pos = GetLockZ(v->tile);
				if (v->tile != gp.new_tile) v->lock = SLS_RESET_OTHER_END;
				break;
			}
			case SLS_RESET_OTHER_END: {
				TileIndex other = TileAddByDiagDir(v->tile, TrackdirToExitdir(ReverseTrackdir(v->GetVehicleTrackdir())));
				uint8 final_level = GetLockPart(other) == LOCK_PART_UPPER ? TILE_HEIGHT : 0;
				uint8 level = GetLockWaterLevel(other);
				if (level == final_level) {
					assert(CheckSameLock(v->tile, gp.new_tile));
					v->tile = gp.new_tile;
					v->lock = SLS_SHIP_LEAVE;
				} else {
					SetLockWaterLevel(other, level + (level > final_level ? -1 : 1));
				}
				v->x_pos = gp.x;
				v->y_pos = gp.y;
				break;
			}
			case SLS_SHIP_LEAVE: {
				v->x_pos = gp.x;
				v->y_pos = gp.y;
				v->z_pos = GetSlopePixelZ(gp.x, gp.y);

				if (v->tile != gp.new_tile) {
					v->lock = SLS_NO_LOCK;
					/* Will fall and execute the ship controller and decide whether to reverse. */
				}
				break;
			}

			default: NOT_REACHED();
		}

		if (v->lock != SLS_NO_LOCK) goto getout;
	}

	if (v->state != TRACK_BIT_WORMHOLE) {
		/* Not on a bridge */
		if (gp.old_tile == gp.new_tile) {
			/* Staying in tile */
			if (v->IsInDepot()) {
				gp.x = v->x_pos;
				gp.y = v->y_pos;
			} else {
				/* Not inside depot */
				r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
				if (HasBit(r, VETS_CANNOT_ENTER)) goto reverse_direction;

				switch (v->current_order.GetType()) {
					case OT_LEAVESTATION:
						/* A leave station order only needs one tick to get processed,
						 * so we can always skip ahead. */
						v->current_order.Free();
						SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
						if (ShipNeedsReversingInDock(v)) v->direction = ReverseDir(v->direction);
						break;

					case OT_GOTO_STATION:
						/* Oil rigs won't be processed here as they will reverse when reaching the tile */
						if (v->dest_tile == gp.new_tile && (gp.x & 0xF) == 8 && (gp.y & 0xF) == 8) {
							ShipReachesDockDestTile(v, gp.new_tile);
						}
						break;

					default: break;
				}
			}
		} else {
			/* New tile */
			if (!IsValidTile(gp.new_tile)) goto reverse_direction;

			if (v->current_order.IsType(OT_GOTO_WAYPOINT)) {
				if (DistanceManhattan(v->dest_tile, gp.new_tile) <= 3) {
					UpdateVehicleTimetable(v, true);
					v->IncrementRealOrderIndex();
					v->current_order.MakeDummy();
				}
			} else if (v->dest_tile == gp.new_tile && v->current_order.IsType(OT_GOTO_STATION) && IsTileType(gp.new_tile, MP_INDUSTRY)) {
				/* Workaround to deal with oil rigs. */
				v->last_station_visited = v->current_order.GetDestination();
				ShipArrivesAt(v, Station::Get(v->last_station_visited));
				v->BeginLoading();
				goto reverse_direction;
			} else if (v->current_order.IsType(OT_GOTO_DEPOT) &&
						IsShipDepotTile(gp.new_tile) &&
						GetOtherShipDepotTile(gp.new_tile) == gp.old_tile &&
						v->current_order.GetDestination() == GetDepotIndex(gp.new_tile)) {
				/* Free path ahead, if it continues in next tile. */
				if (HasWaterTrackReservation(gp.new_tile)) LiftPathReservation(gp.new_tile, v->GetVehicleTrackdir());

				HandleShipEnterDepot(v);
				goto getout;
			}

			DiagDirection diagdir = DiagdirBetweenTiles(gp.old_tile, gp.new_tile);
			assert(diagdir != INVALID_DIAGDIR);

			/* Choose a ship track */
			tracks = GetAvailShipTracks(gp.new_tile, diagdir);
			if (tracks == TRACK_BIT_NONE) goto reverse_direction;

			/* If we can continue path, do it */
			if (tracks & GetReservedWaterTracks(gp.new_tile)) {
				tracks &= GetReservedWaterTracks(gp.new_tile);
				track = TrackBitsToTrack(tracks);
			} else {
				TrackdirBits trackdirs = TrackBitsToTrackdirBits(tracks) & DiagdirReachesTrackdirs(diagdir);

				/* Lift colliding reservations if possible on first tile */
				LiftReservations(gp.new_tile, trackdirs);

				/* Of the available tracks, get only those that can be reserved */
				tracks = GetFreeWaterTrackReservation(gp.new_tile, trackdirs);

				/* There is a continuation to our path but it is currently occupied. */
				if (tracks == TRACK_BIT_NONE) goto handle_stuck;

				/* Choose a direction, and continue if we find one */
				track = ChooseShipTrack(v, gp.new_tile, diagdir, tracks);
				if (track == INVALID_TRACK) goto handle_stuck;
			}

			if (v->IsStuck()) v->Unstuck();

			b = _ship_subcoord[diagdir][track];

			gp.x = (gp.x & ~0xF) | b[0];
			gp.y = (gp.y & ~0xF) | b[1];

			/* Call the landscape function and tell it that the vehicle entered the tile */
			r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
			if (HasBit(r, VETS_CANNOT_ENTER)) goto reverse_direction;

			if (!HasBit(r, VETS_ENTERED_WORMHOLE)) {
				if (!IsLockTile(gp.old_tile)) {
					SetWaterTrackReservation(gp.old_tile, TrackdirToTrack(v->GetVehicleTrackdir()), false);
					v->lock = SLS_NO_LOCK;
				} else if (!CheckSameLock(gp.old_tile, gp.new_tile)) {
					/* We were on a lock and now we are leaving that lock.
					 * We must lift reservation of all the lock tiles. */
					LiftPathReservation(gp.old_tile, ReverseTrackdir(v->GetVehicleTrackdir()));
				}

				if (IsLockTile(gp.new_tile)) {
					uint level = GetLockWaterLevel(gp.new_tile);
					uint level_middle = GetLockWaterLevel(GetLockMiddleTile(gp.new_tile));
					v->lock = level == level_middle ? SLS_SHIP_ENTER : SLS_PREPARING_LOCK;
				}

				v->tile = gp.new_tile;
				v->state = TrackToTrackBits(track);

				/* Update ship cache when the water class changes. Aqueducts are always canals. */
				WaterClass old_wc = GetEffectiveWaterClass(gp.old_tile);
				WaterClass new_wc = GetEffectiveWaterClass(gp.new_tile);
				if (old_wc != new_wc) v->UpdateCache();
			}

			v->direction = (Direction)b[2];
		}
	} else {
		/* On a bridge */
		if (!IsTileType(gp.new_tile, MP_TUNNELBRIDGE) || !HasBit(VehicleEnterTile(v, gp.new_tile, gp.x, gp.y), VETS_ENTERED_WORMHOLE)) {
			v->x_pos = gp.x;
			v->y_pos = gp.y;
			v->UpdatePosition();
			if ((v->vehstatus & VS_HIDDEN) == 0) v->Vehicle::UpdateViewport(true);
			return;
		}
	}

	/* update image of ship, as well as delta XY */
	v->x_pos = gp.x;
	v->y_pos = gp.y;
	v->z_pos = GetSlopePixelZ(gp.x, gp.y);

getout:
	v->UpdatePosition();
	v->UpdateViewport(true, true);
	return;

reverse_direction:
	if (v->lock == SLS_NO_LOCK && IsLockTile(v->tile)) v->lock = SLS_SHIP_ENTER;
	dir = ReverseDir(v->direction);
	v->direction = dir;
	goto getout;

handle_stuck:
	if (v->IsStuck()) {
		v->Unstuck();
		goto reverse_direction;
	} else {
		v->MarkShipAsStuck(true, SHIP_BLOCKED_TICKS);
		goto getout;
	}
}

bool Ship::Tick()
{
	if (!(this->vehstatus & VS_STOPPED)) this->running_ticks++;

	ShipController(this);

	return true;
}

/**
 * Build a ship.
 * @param tile     tile of the depot where ship is built.
 * @param flags    type of operation.
 * @param e        the engine to build.
 * @param data     unused.
 * @param ret[out] the vehicle that has been built.
 * @return the cost of this operation or an error.
 */
CommandCost CmdBuildShip(TileIndex tile, DoCommandFlag flags, const Engine *e, uint16 data, Vehicle **ret)
{
	tile = GetShipDepotNorthTile(tile);
	if (flags & DC_EXEC) {
		int x;
		int y;

		const ShipVehicleInfo *svi = &e->u.ship;

		Ship *v = new Ship();
		*ret = v;

		v->owner = _current_company;
		v->tile = tile;
		x = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
		y = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
		v->x_pos = x;
		v->y_pos = y;
		v->z_pos = GetSlopePixelZ(x, y);

		v->UpdateDeltaXY(v->direction);
		v->vehstatus = VS_HIDDEN | VS_STOPPED | VS_DEFPAL;

		v->spritenum = svi->image_index;
		v->cargo_type = e->GetDefaultCargoType();
		v->cargo_cap = svi->capacity;
		v->refit_cap = 0;

		v->last_station_visited = INVALID_STATION;
		v->last_loading_station = INVALID_STATION;
		v->engine_type = e->index;

		v->reliability = e->reliability;
		v->reliability_spd_dec = e->reliability_spd_dec;
		v->max_age = e->GetLifeLengthInDays();
		_new_vehicle_id = v->index;

		v->state = TRACK_BIT_DEPOT;

		v->SetServiceInterval(Company::Get(_current_company)->settings.vehicle.servint_ships);
		v->date_of_last_service = _date;
		v->build_year = _cur_year;
		v->sprite_seq.Set(SPR_IMG_QUERY);
		v->random_bits = VehicleRandomBits();

		v->UpdateCache();

		if (e->flags & ENGINE_EXCLUSIVE_PREVIEW) SetBit(v->vehicle_flags, VF_BUILT_AS_PROTOTYPE);
		v->SetServiceIntervalIsPercent(Company::Get(_current_company)->settings.vehicle.servint_ispercent);

		v->InvalidateNewGRFCacheOfChain();

		v->cargo_cap = e->DetermineCapacity(v);

		v->InvalidateNewGRFCacheOfChain();

		v->UpdatePosition();
	}

	return CommandCost();
}

bool Ship::FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse)
{
	const Depot *depot = FindClosestShipDepot(this, 0);

	if (depot == NULL) return false;

	if (location    != NULL) *location    = depot->GetBestDepotTile(this);
	if (destination != NULL) *destination = depot->index;

	return true;
}
