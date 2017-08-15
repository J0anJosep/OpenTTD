/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file aircraft_cmd.cpp
 * This file deals with aircraft and airport movements functionalities
 */

#include "stdafx.h"
#include "aircraft.h"
#include "landscape.h"
#include "news_func.h"
#include "newgrf_engine.h"
#include "newgrf_sound.h"
#include "spritecache.h"
#include "strings_func.h"
#include "command_func.h"
#include "window_func.h"
#include "date_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "cheat_type.h"
#include "company_base.h"
#include "ai/ai.hpp"
#include "game/game.hpp"
#include "company_func.h"
#include "effectvehicle_func.h"
#include "station_base.h"
#include "engine_base.h"
#include "core/random_func.hpp"
#include "core/backup_type.hpp"
#include "zoom_func.h"
#include "disaster_vehicle.h"
#include "depot_map.h"
#include "depot_base.h"
#include "air.h"
#include "viewport_func.h"
#include "pbs_air.h"
#include "platform_func.h"
#include "pathfinder/yapf/yapf.h"
#include "debug.h"

#include "table/strings.h"

#include "safeguards.h"

/** Mark the vehicle as stuck. */
void Aircraft::MarkAsStuck()
{
	DEBUG(misc, 0, "Marked stuck");
	assert(!this->IsStuck());

	/* Set the aircraft as stuck. */
	SetBit(this->flags, VAF_STUCK);
	this->wait_counter = 0;

	/* Stop aircraft. */
	this->cur_speed = 0;
	this->subspeed = 0;

	SetWindowWidgetDirty(WC_VEHICLE_VIEW, this->index, WID_VV_START_STOP);
}

/** Unstuck the aircraft. */
void Aircraft::Unstuck() {
	ClrBit(this->flags, VAF_STUCK);
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, this->index, WID_VV_START_STOP);
}

void Aircraft::UpdateDeltaXY(Direction direction)
{
	this->x_offs = -1;
	this->y_offs = -1;
	this->x_extent = 2;
	this->y_extent = 2;

	switch (this->subtype) {
		default: NOT_REACHED();

		case AIR_AIRCRAFT:
		case AIR_HELICOPTER:
			switch (this->cur_state) {
				default: break;
				case AM_TAKEOFF:
				case AM_HELICOPTER_TAKEOFF:
				case AM_FLYING:
				case AM_LANDING:
				case AM_HELICOPTER_LANDING:
					this->x_extent = 24;
					this->y_extent = 24;
					break;
			}
			this->z_extent = 5;
			break;

		case AIR_SHADOW:
			this->z_extent = 1;
			this->x_offs = 0;
			this->y_offs = 0;
			break;

		case AIR_ROTOR:
			this->z_extent = 1;
			break;
	}
}

static void CrashAirplane(Aircraft *v);

static const SpriteID _aircraft_sprite[] = {
	0x0EB5, 0x0EBD, 0x0EC5, 0x0ECD,
	0x0ED5, 0x0EDD, 0x0E9D, 0x0EA5,
	0x0EAD, 0x0EE5, 0x0F05, 0x0F0D,
	0x0F15, 0x0F1D, 0x0F25, 0x0F2D,
	0x0EED, 0x0EF5, 0x0EFD, 0x0F35,
	0x0E9D, 0x0EA5, 0x0EAD, 0x0EB5,
	0x0EBD, 0x0EC5
};

template <>
bool IsValidImageIndex<VEH_AIRCRAFT>(uint8 image_index)
{
	return image_index < lengthof(_aircraft_sprite);
}

void Aircraft::GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const
{
	uint8 spritenum = this->spritenum;

	if (is_custom_sprite(spritenum)) {
		GetCustomVehicleSprite(this, direction, image_type, result);
		if (result->IsValid()) return;

		spritenum = this->GetEngine()->original_image_index;
	}

	assert(IsValidImageIndex<VEH_AIRCRAFT>(spritenum));
	result->Set(direction + _aircraft_sprite[spritenum]);
}

void GetRotorImage(const Aircraft *v, EngineImageType image_type, VehicleSpriteSeq *result)
{
	assert(v->subtype == AIR_HELICOPTER);

	const Aircraft *w = v->Next()->Next();
	if (is_custom_sprite(v->spritenum)) {
		GetCustomRotorSprite(v, false, image_type, result);
		if (result->IsValid()) return;
	}

	/* Return standard rotor sprites if there are no custom sprites for this helicopter */
	DEBUG(misc, 0, "cur_state for rotor: %d", w->cur_state);
	result->Set(SPR_ROTOR_STOPPED + w->cur_state);
}

static void GetAircraftIcon(EngineID engine, EngineImageType image_type, VehicleSpriteSeq *result)
{
	const Engine *e = Engine::Get(engine);
	uint8 spritenum = e->u.air.image_index;

	if (is_custom_sprite(spritenum)) {
		GetCustomVehicleIcon(engine, DIR_W, image_type, result);
		if (result->IsValid()) return;

		spritenum = e->original_image_index;
	}

	assert(IsValidImageIndex<VEH_AIRCRAFT>(spritenum));
	result->Set(DIR_W + _aircraft_sprite[spritenum]);
}

void DrawAircraftEngine(int left, int right, int preferred_x, int y, EngineID engine, PaletteID pal, EngineImageType image_type)
{
	VehicleSpriteSeq seq;
	GetAircraftIcon(engine, image_type, &seq);

	Rect rect;
	seq.GetBounds(&rect);
	preferred_x = Clamp(preferred_x,
			left - UnScaleGUI(rect.left),
			right - UnScaleGUI(rect.right));

	seq.Draw(preferred_x, y, pal, pal == PALETTE_CRASH);

	if (!(AircraftVehInfo(engine)->subtype & AIR_CTOL)) {
		VehicleSpriteSeq rotor_seq;
		GetCustomRotorIcon(engine, image_type, &rotor_seq);
		if (!rotor_seq.IsValid()) rotor_seq.Set(SPR_ROTOR_STOPPED);
		rotor_seq.Draw(preferred_x, y - ScaleGUIPixels(5), PAL_NONE, false);
	}
}

/**
 * Get the size of the sprite of an aircraft sprite heading west (used for lists).
 * @param engine The engine to get the sprite from.
 * @param[out] width The width of the sprite.
 * @param[out] height The height of the sprite.
 * @param[out] xoffs Number of pixels to shift the sprite to the right.
 * @param[out] yoffs Number of pixels to shift the sprite downwards.
 * @param image_type Context the sprite is used in.
 */
void GetAircraftSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type)
{
	VehicleSpriteSeq seq;
	GetAircraftIcon(engine, image_type, &seq);

	Rect rect;
	seq.GetBounds(&rect);

	width  = UnScaleGUI(rect.right - rect.left + 1);
	height = UnScaleGUI(rect.bottom - rect.top + 1);
	xoffs  = UnScaleGUI(rect.left);
	yoffs  = UnScaleGUI(rect.top);
}

void Aircraft::SetHidden(bool hidden)
{
	/* If this gives problems, move it to the end of the function. */
	assert(IsHangarTile(this->tile));

	if (hidden) {
		this->vehstatus |= VS_HIDDEN;
		this->Next()->vehstatus |= VS_HIDDEN;
		if (this->IsHelicopter()) this->Next()->Next()->vehstatus |= VS_HIDDEN;
	} else {
		this->vehstatus &= ~VS_HIDDEN;
		this->Next()->vehstatus &= ~VS_HIDDEN;
		if (this->IsHelicopter()) this->Next()->Next()->vehstatus &= ~VS_HIDDEN;
	}

	this->UpdateViewport(true, true);
	this->UpdatePosition();
}

void Aircraft::FreeReservation()
{
	if (this->cur_state == AM_HANGAR) {
		if (IsBigHangar(this->tile)) {
			RemoveAirportTrackReservation(this->tile, TrackdirToTrack(this->trackdir));
			if (_settings_client.gui.show_airport_tracks) MarkTileDirtyByTile(this->tile);
		}
	} else {
		// free path
	}
}

/**
 * Return whether the airport is closed: manually, due to a crash or
 * due to an invalid design of the airport.
 * @param tile a tile of the airport.
 */
bool IsAirportClosed(TileIndex tile)
{
	assert(IsAirportTile(tile));
	return (Station::GetByTile(tile)->airport.flags & AF_CLOSED) != 0;
}

/**
 * Return whether the airport has a valid design.
 * @param tile a tile of the airport.
 */
bool IsValidAirportDesign(TileIndex tile)
{
	assert(IsAirportTile(tile));
	return (Station::GetByTile(tile)->airport.flags & AF_CLOSED_DESIGN) == 0;
}

/**
 * Find the nearest hangar to v
 * INVALID_STATION is returned, if the company does not have any suitable
 * airports (like helipads only)
 * @param v vehicle looking for a hangar
 * @return the StationID if one is found, otherwise, INVALID_STATION
 */
static StationID FindNearestHangar(const Aircraft *v)
{
	assert(v != NULL && v->IsNormalAircraft());

	uint best_dist = UINT_MAX;
	StationID best_airport = INVALID_STATION;

	Station *st;
	FOR_ALL_STATIONS(st) {
		if (st->owner != v->owner) continue;
		if (!st->airport.HasHangar()) continue;
		if ((st->airport.flags & AF_CLOSED) == 0) continue;

		if (v->IsHelicopter()) {
			if ((st->airport.flags & AF_HELICOPTERS) == 0) continue;
		} else {
			if ((st->airport.flags & AF_LANDING) == 0) continue;
		}

		uint dist = DistanceManhattan(st->xy, v->tile);
		if (dist < best_dist) {
			best_dist = dist;
			best_airport = st->index;
		}
	}

	return best_airport;
}

/**
 * Find a free hangar of a given depot.
 * INVALID_TILE is returned, if the company does not have any suitable
 * hangar. Small hangars are always suitable. Big hangars are suitable
 * if there is no aircraft occupying it.
 * @param dep_id Depot index where to look for an empty hangar.
 * @return the tile of a hangar or INVALID_TILE if none found.
 */
TileIndex FindCompatibleHangar(DepotID dep_id)
{
	assert(Depot::IsValidID(dep_id));
	Depot *depot = Depot::Get(dep_id);
	assert(depot->veh_type == VEH_AIRCRAFT);

	for (uint i = depot->depot_tiles.Length(); i--;) {
		TileIndex tile = depot->depot_tiles[i];
		if (IsSmallHangar(tile)) return tile;
		if (IsAirportPositionFree(tile, DiagDirToDiagTrackdir(GetHangarDirection(tile)))) return tile;
	}

	return INVALID_TILE;
}

bool Aircraft::FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse)
{
	const Station *st = GetTargetAirportIfValid(this);
	if (!this->IsAircraftFlying()) st = Station::GetByTile(this->tile);
	/* If the station is not a valid airport or if it has no hangars */
	if (st == NULL || !st->airport.HasHangar()) {
		/* the aircraft has to search for a hangar on its own */
		StationID station = FindNearestHangar(this);

		if (station == INVALID_STATION) return false;

		st = Station::Get(station);
	}

	if (location    != NULL) *location    = st->xy;
	if (destination != NULL) *destination = st->index;

	return true;
}

/**
 * Returns aircraft's target station if v->target_airport
 * is a valid station with airport.
 * @param v vehicle to get target airport for
 * @return pointer to target station, NULL if invalid
 */
Station *GetTargetAirportIfValid(const Aircraft *v)
{
	assert(v->type == VEH_AIRCRAFT);

	Station *st = Station::GetIfValid(v->targetairport);

	if (st == NULL) return NULL;

	if (v->subtype == AIR_HELICOPTER && st->airport.HasHelipad()) return st;

	if (!st->airport.HasLanding()) return NULL;

	return st;
}

/**
 * Updates the status of the Aircraft heading or in the station
 * @param st Station been updated
 */
void UpdateAirplanesOnChangedAirport(const Station *st)
{
	Aircraft *v;
	FOR_ALL_AIRCRAFT(v) {
		if (!v->IsNormalAircraft() || v->targetairport != st->index) continue;
		assert(v->cur_state == AM_FLYING);
		//v->pos = v->previous_pos = AircraftGetEntryPoint(v, ap, rotation);
		UpdateAircraftCache(v);
	}
}

static void CheckIfAircraftNeedsService(Aircraft *v)
{
	if (Company::Get(v->owner)->settings.vehicle.servint_aircraft == 0 || !v->NeedsAutomaticServicing()) return;
	if (v->IsChainInDepot()) {
		VehicleServiceInDepot(v);
		return;
	}

	/* When we're parsing conditional orders and the like
	 * we don't want to consider going to a depot too. */
	if (!v->current_order.IsType(OT_GOTO_DEPOT) && !v->current_order.IsType(OT_GOTO_STATION)) return;

	const Station *st = Station::Get(v->current_order.GetDestination());

	assert(st != NULL);

	/* only goto depot if the target airport has a depot */
	if (st->airport.HasHangar() && CanVehicleUseStation(v, st)) {
		v->current_order.MakeGoToDepot(st->airport.depot_id, ODTFB_SERVICE);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	} else if (v->current_order.IsType(OT_GOTO_DEPOT)) {
		v->current_order.MakeDummy();
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}
}

Money Aircraft::GetRunningCost() const
{
	const Engine *e = this->GetEngine();
	uint cost_factor = GetVehicleProperty(this, PROP_AIRCRAFT_RUNNING_COST_FACTOR, e->u.air.running_cost);
	return GetPrice(PR_RUNNING_AIRCRAFT, cost_factor, e->GetGRF());
}

/**
 * Build an aircraft.
 * @param tile     tile of the depot where aircraft is built.
 * @param flags    type of operation.
 * @param e        the engine to build.
 * @param data     unused.
 * @param ret[out] the vehicle that has been built.
 * @return the cost of this operation or an error.
 */
CommandCost CmdBuildAircraft(TileIndex tile, DoCommandFlag flags, const Engine *e, uint16 data, Vehicle **ret)
{
	const AircraftVehicleInfo *avi = &e->u.air;
	const Station *st = Station::GetByTile(tile);

	if (!IsCompatibleAirType(avi->airtype, st->airport.air_type)) return_cmd_error(STR_ERROR_AIRCRAFT_INCOMPATIBLE_AIR_TYPE);

	if (!IsValidAirportDesign(tile)) return_cmd_error(STR_ERROR_AIRPORT_BAD_DESIGN);

	if (st->airport.depot_id == INVALID_DEPOT) {
		//REVISE: choose between not reached or an error
		NOT_REACHED();
		return_cmd_error(STR_ERROR_NO_HANGAR);
	}

	tile = FindCompatibleHangar(st->airport.depot_id);

	if (!IsValidTile(tile)) return_cmd_error(STR_ERROR_NO_FREE_HANGAR);

	if (flags & DC_EXEC) {
		Aircraft *v = new Aircraft(); // aircraft
		Aircraft *u = new Aircraft(); // shadow
		*ret = v;

		v->airtype = avi->airtype;
		v->compatible_airtypes = GetCompatibleAirTypes(v->airtype);
		v->owner = u->owner = _current_company;
		v->cur_state = AM_HANGAR;
		v->next_state = AM_HANGAR;
		v->tile = tile;
		v->next_tile = tile;
		v->direction = DiagDirToDir(GetHangarDirection(tile));
		v->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
		v->next_trackdir = v->trackdir;
		v->desired_trackdir = INVALID_TRACKDIR;

		uint x = TileX(tile) * TILE_SIZE + 8;
		uint y = TileY(tile) * TILE_SIZE + 8;

		v->x_pos = u->x_pos = x;
		v->y_pos = u->y_pos = y;

		DiagDirection diag_dir = GetHangarDirection(tile);
		v->direction = u->direction = DiagDirToDir(diag_dir);
		v->trackdir = DiagDirToDiagTrackdir(diag_dir);

		/* Store some things on the shadow. */
		u->next_tile = tile;
		u->next_trackdir = v->trackdir;

		u->z_pos = GetTileMaxPixelZ(v->tile);
		v->z_pos = u->z_pos + 1;

		v->vehstatus = VS_HIDDEN | VS_STOPPED | VS_DEFPAL;
		u->vehstatus = VS_HIDDEN | VS_UNCLICKABLE | VS_SHADOW;

		v->spritenum = avi->image_index;
		u->spritenum = avi->image_index;

		v->cargo_cap = avi->passenger_capacity;
		v->refit_cap = 0;
		u->cargo_cap = avi->mail_capacity;
		u->refit_cap = 0;

		v->cargo_type = e->GetDefaultCargoType();
		u->cargo_type = CT_MAIL;

		v->name = NULL;
		v->last_station_visited = INVALID_STATION;
		v->last_loading_station = INVALID_STATION;

		v->acceleration = avi->acceleration;
		v->engine_type = e->index;
		u->engine_type = e->index;

		v->subtype = (avi->subtype & AIR_CTOL ? AIR_AIRCRAFT : AIR_HELICOPTER);
		v->UpdateDeltaXY(INVALID_DIR);

		u->subtype = AIR_SHADOW;
		u->UpdateDeltaXY(INVALID_DIR);

		v->reliability = e->reliability;
		v->reliability_spd_dec = e->reliability_spd_dec;
		v->max_age = e->GetLifeLengthInDays();

		_new_vehicle_id = v->index;

		v->flags = 0;
		v->wait_counter = 0;

		v->cur_state = AM_HANGAR;
		v->targetairport = GetStationIndex(tile);
		v->dest_tile = tile;
		v->SetNext(u);

		if (IsBigHangar(tile)) {
			SetAirportTracksReservation(tile, DiagDirToDiagTrackBits(diag_dir));
			if (_settings_client.gui.show_airport_tracks) MarkTileDirtyByTile(tile);
		}

		v->SetServiceInterval(Company::Get(_current_company)->settings.vehicle.servint_aircraft);

		v->date_of_last_service = _date;
		v->build_year = u->build_year = _cur_year;

		v->sprite_seq.Set(SPR_IMG_QUERY);
		u->sprite_seq.Set(SPR_IMG_QUERY);

		v->random_bits = VehicleRandomBits();
		u->random_bits = VehicleRandomBits();

		v->vehicle_flags = 0;
		if (e->flags & ENGINE_EXCLUSIVE_PREVIEW) SetBit(v->vehicle_flags, VF_BUILT_AS_PROTOTYPE);
		v->SetServiceIntervalIsPercent(Company::Get(_current_company)->settings.vehicle.servint_ispercent);

		v->InvalidateNewGRFCacheOfChain();

		v->cargo_cap = e->DetermineCapacity(v, &u->cargo_cap);

		v->InvalidateNewGRFCacheOfChain();

		UpdateAircraftCache(v, true);

		v->UpdatePosition();
		u->UpdatePosition();

		/* Aircraft with 3 vehicles (chopper)? */
		if (v->subtype == AIR_HELICOPTER) {
			Aircraft *w = new Aircraft();
			w->engine_type = e->index;
			w->direction = DIR_N;
			w->owner = _current_company;
			w->x_pos = v->x_pos;
			w->y_pos = v->y_pos;
			w->z_pos = v->z_pos + ROTOR_Z_OFFSET;
			w->tick_counter = 0;
			w->cur_speed = 0;
			w->vehstatus = VS_HIDDEN | VS_UNCLICKABLE;
			w->spritenum = 0xFF;
			w->subtype = AIR_ROTOR;
			w->sprite_seq.Set(SPR_ROTOR_STOPPED);
			w->random_bits = VehicleRandomBits();
			/* Use rotor's air.state to store the rotor animation frame */
			w->cur_state = HRS_ROTOR_STOPPED;
			w->UpdateDeltaXY(INVALID_DIR);

			u->SetNext(w);
			w->UpdatePosition();
		}

		v->SetHidden(IsSmallHangar(v->tile));
	}

	return CommandCost();
}

void Aircraft::OnNewDay()
{
	if (!this->IsNormalAircraft()) return;

	if ((++this->day_counter & 7) == 0) DecreaseVehicleValue(this);

	CheckOrders(this);

	CheckVehicleBreakdown(this);
	AgeVehicle(this);
	CheckIfAircraftNeedsService(this);

	if (this->running_ticks == 0) return;

	CommandCost cost(EXPENSES_AIRCRAFT_RUN, this->GetRunningCost() * this->running_ticks / (DAYS_IN_YEAR * DAY_TICKS));

	this->profit_this_year -= cost.GetCost();
	this->running_ticks = 0;

	SubtractMoneyFromCompanyFract(this->owner, cost);

	SetWindowDirty(WC_VEHICLE_DETAILS, this->index);
	SetWindowClassesDirty(WC_AIRCRAFT_LIST);
}

static void HelicopterTickHandler(Aircraft *v)
{
	Aircraft *u = v->Next()->Next();

	if (u->vehstatus & VS_HIDDEN) return;

	/* if true, helicopter rotors do not rotate. This should only be the case if a helicopter is
	 * loading/unloading at a terminal or stopped */
	if (v->current_order.IsType(OT_LOADING) || (v->vehstatus & VS_STOPPED)) {
		if (u->cur_speed != 0) {
			u->cur_speed++;
			if (u->cur_speed >= 0x80 && u->cur_state == HRS_ROTOR_MOVING_3) {
				u->cur_speed = 0;
			}
		}
	} else {
		if (u->cur_speed == 0) {
			u->cur_speed = 0x70;
		}
		if (u->cur_speed >= 0x50) {
			u->cur_speed--;
		}
	}

	int tick = ++u->tick_counter;
	int spd = u->cur_speed >> 4;

	VehicleSpriteSeq seq;
	if (spd == 0) {
		u->cur_state = HRS_ROTOR_STOPPED;
		GetRotorImage(v, EIT_ON_MAP, &seq);
		if (u->sprite_seq == seq) return;
	} else if (tick >= spd) {
		u->tick_counter = 0;
		u->cur_state++;
		if (u->cur_state > HRS_ROTOR_MOVING_3) u->cur_state = HRS_ROTOR_MOVING_1;
		GetRotorImage(v, EIT_ON_MAP, &seq);
	} else {
		return;
	}

	u->sprite_seq = seq;

	u->UpdatePositionAndViewport();
}

/**
 * Set aircraft position.
 * @param v Aircraft to position.
 * @param x New X position.
 * @param y New y position.
 * @param z New z position.
 */
void SetAircraftPosition(Aircraft *v, int x, int y, int z)
{
	v->Next()->direction = v->direction;
	v->x_pos = x;
	v->y_pos = y;
	v->z_pos = z;

	v->UpdatePosition();
	v->UpdateViewport(true, false);
	if (v->subtype == AIR_HELICOPTER) {
		GetRotorImage(v, EIT_ON_MAP, &v->Next()->Next()->sprite_seq);
	}

	Aircraft *u = v->Next();

	int safe_x = Clamp(x, 0, MapMaxX() * TILE_SIZE);
	int safe_y = Clamp(y - 1, 0, MapMaxY() * TILE_SIZE);
	u->x_pos = x;
	u->y_pos = y - ((v->z_pos - GetSlopePixelZ(safe_x, safe_y)) >> 3);

	safe_y = Clamp(u->y_pos, 0, MapMaxY() * TILE_SIZE);
	u->z_pos = GetSlopePixelZ(safe_x, safe_y);
	u->sprite_seq.CopyWithoutPalette(v->sprite_seq); // the shadow is never coloured

	u->UpdatePositionAndViewport();

	u = u->Next();
	if (u != NULL) {
		u->x_pos = x;
		u->y_pos = y;
		u->z_pos = z + ROTOR_Z_OFFSET;

		u->UpdatePositionAndViewport();
	}
}

/**
 * Update cached values of an aircraft.
 * Currently caches callback 36 max speed.
 * @param v Vehicle
 * @param update_range Update the aircraft range.
 */
void UpdateAircraftCache(Aircraft *v, bool update_range)
{
	uint max_speed = GetVehicleProperty(v, PROP_AIRCRAFT_SPEED, 0);
	if (max_speed != 0) {
		/* Convert from original units to km-ish/h */
		max_speed = (max_speed * 128) / 10;

		v->vcache.cached_max_speed = max_speed;
	} else {
		/* Use the default max speed of the vehicle. */
		v->vcache.cached_max_speed = AircraftVehInfo(v->engine_type)->max_speed;
	}

	/* Update cargo aging period. */
	v->vcache.cached_cargo_age_period = GetVehicleProperty(v, PROP_AIRCRAFT_CARGO_AGE_PERIOD, EngInfo(v->engine_type)->cargo_age_period);
	Aircraft *u = v->Next(); // Shadow for mail
	u->vcache.cached_cargo_age_period = GetVehicleProperty(u, PROP_AIRCRAFT_CARGO_AGE_PERIOD, EngInfo(u->engine_type)->cargo_age_period);

	/* Update aircraft range. */
	if (update_range) {
		v->acache.cached_max_range = GetVehicleProperty(v, PROP_AIRCRAFT_RANGE, AircraftVehInfo(v->engine_type)->max_range);
		/* Squared it now so we don't have to do it later all the time. */
		v->acache.cached_max_range_sqr = v->acache.cached_max_range * v->acache.cached_max_range;
	}
}


/**
 * Special velocities for aircraft
 */
enum AircraftSpeedLimits {
	SPEED_LIMIT_TAXI     =     50,  ///< Maximum speed of an aircraft while taxiing
	SPEED_LIMIT_APPROACH =    230,  ///< Maximum speed of an aircraft on finals
	SPEED_LIMIT_BROKEN   =    320,  ///< Maximum speed of an aircraft that is broken
	SPEED_LIMIT_HOLD     =    425,  ///< Maximum speed of an aircraft that flies the holding pattern
	SPEED_LIMIT_NONE     = 0xFFFF,  ///< No environmental speed limit. Speed limit is type dependent
};

/**
 * Sets the new speed for an aircraft
 * @param v The vehicle for which the speed should be obtained
 * @param speed_limit The maximum speed the vehicle may have.
 * @param hard_limit If true, the limit is directly enforced, otherwise the plane is slowed down gradually
 * @return The number of position updates needed within the tick
 */
static int UpdateAircraftSpeed(Aircraft *v, bool mode = false)
{
	if (mode) return v->progress >> 8;
	/**
	 * 'acceleration' has the unit 3/8 mph/tick. This function is called twice per tick.
	 * So the speed amount we need to accelerate is:
	 *     acceleration * 3 / 16 mph = acceleration * 3 / 16 * 16 / 10 km-ish/h
	 *                               = acceleration * 3 / 10 * 256 * (km-ish/h / 256)
	 *                               ~ acceleration * 77 (km-ish/h / 256)
	 */
	uint spd = v->acceleration * 77;
	byte t;
	uint speed_limit = SPEED_LIMIT_TAXI;
	bool hard_limit = true;

	if (HasBit(v->flags, VAF_HOLD)) {
		speed_limit = SPEED_LIMIT_HOLD;
		hard_limit = false;
	} else if (v->IsAircraftFlying()) {
		hard_limit = false;
		speed_limit = SPEED_LIMIT_NONE;
		if (v->IsAircraftLanding() && (v->z_pos > GetSlopePixelZ(v->x_pos, v->y_pos))) {
			speed_limit = SPEED_LIMIT_APPROACH;
		}
	}

	/* Adjust speed limits by plane speed factor to prevent taxiing
	 * and take-off speeds being too low. */
	speed_limit *= _settings_game.vehicle.plane_speed;

	if (v->vcache.cached_max_speed < speed_limit) {
		if (v->cur_speed < speed_limit) hard_limit = false;
		speed_limit = v->vcache.cached_max_speed;
	}

	v->subspeed = (t = v->subspeed) + (byte)spd;

	/* Aircraft's current speed is used twice so that very fast planes are
	 * forced to slow down rapidly in the short distance needed. The magic
	 * value 16384 was determined to give similar results to the old speed/48
	 * method at slower speeds. This also results in less reduction at slow
	 * speeds to that aircraft do not get to taxi speed straight after
	 * touchdown. */
	if (!hard_limit && v->cur_speed > speed_limit) {
		speed_limit = v->cur_speed - max(1, ((v->cur_speed * v->cur_speed) / 16384) / _settings_game.vehicle.plane_speed);
	}

	spd = min(v->cur_speed + (spd >> 8) + (v->subspeed < t), speed_limit);

	/* adjust speed for broken vehicles */
	if (v->vehstatus & VS_AIRCRAFT_BROKEN) spd = min(spd, SPEED_LIMIT_BROKEN);

	/* updates statusbar only if speed have changed to save CPU time */
	if (spd != v->cur_speed) {
		v->cur_speed = spd;
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}

	/* Adjust distance moved by plane speed setting */
	if (_settings_game.vehicle.plane_speed > 1) spd /= _settings_game.vehicle.plane_speed;

	/* Convert direction-independent speed into direction-dependent speed. (old movement method) */
	spd = v->GetOldAdvanceSpeed(spd);

	spd += v->progress;
	v->progress = (byte)spd;
	return spd >> 8;
}

/**
 * Get the tile height below the aircraft.
 * This function is needed because aircraft can leave the mapborders.
 *
 * @param v The vehicle to get the height for.
 * @return The height in pixels from 'z_pos' 0.
 */
int GetTileHeightBelowAircraft(const Vehicle *v)
{
	int safe_x = Clamp(v->x_pos, 0, MapMaxX() * TILE_SIZE);
	int safe_y = Clamp(v->y_pos, 0, MapMaxY() * TILE_SIZE);
	return TileHeight(TileVirtXY(safe_x, safe_y)) * TILE_HEIGHT;
}

/**
 * Get the 'flight level' bounds, in pixels from 'z_pos' 0 for a particular
 * vehicle for normal flight situation.
 * When the maximum is reached the vehicle should consider descending.
 * When the minimum is reached the vehicle should consider ascending.
 *
 * @param v               The vehicle to get the flight levels for.
 * @param [out] min_level The minimum bounds for flight level.
 * @param [out] max_level The maximum bounds for flight level.
 */
void GetAircraftFlightLevelBounds(const Vehicle *v, int *min_level, int *max_level)
{
	int base_altitude = GetTileHeightBelowAircraft(v);
	if (v->type == VEH_AIRCRAFT && Aircraft::From(v)->subtype == AIR_HELICOPTER) {
		base_altitude += HELICOPTER_HOLD_MAX_FLYING_ALTITUDE - PLANE_HOLD_MAX_FLYING_ALTITUDE;
	}

	/* Make sure eastbound and westbound planes do not "crash" into each
	 * other by providing them with vertical separation
	 */
	switch (v->direction) {
		case DIR_N:
		case DIR_NE:
		case DIR_E:
		case DIR_SE:
			base_altitude += 10;
			break;

		default: break;
	}

	/* Make faster planes fly higher so that they can overtake slower ones */
	base_altitude += min(20 * (v->vcache.cached_max_speed / 200) - 90, 0);

	if (min_level != NULL) *min_level = base_altitude + AIRCRAFT_MIN_FLYING_ALTITUDE;
	if (max_level != NULL) *max_level = base_altitude + AIRCRAFT_MAX_FLYING_ALTITUDE;
}

/**
 * Gets the maximum 'flight level' for the holding pattern of the aircraft,
 * in pixels 'z_pos' 0, depending on terrain below..
 *
 * @param v The aircraft that may or may not need to decrease its altitude.
 * @return Maximal aircraft holding altitude, while in normal flight, in pixels.
 */
int GetAircraftHoldMaxAltitude(const Aircraft *v)
{
	int tile_height = GetTileHeightBelowAircraft(v);

	return tile_height + ((v->subtype == AIR_HELICOPTER) ? HELICOPTER_HOLD_MAX_FLYING_ALTITUDE : PLANE_HOLD_MAX_FLYING_ALTITUDE);
}

int GetAircraftFlightLevel(DisasterVehicle *v)
{
	/* Aircraft is in flight. We want to enforce it being somewhere
	 * between the minimum and the maximum allowed altitude. */
	int aircraft_min_altitude;
	int aircraft_max_altitude;
	GetAircraftFlightLevelBounds(v, &aircraft_min_altitude, &aircraft_max_altitude);
	int aircraft_middle_altitude = (aircraft_min_altitude + aircraft_max_altitude) / 2;

	/* If those assumptions would be violated, aircraft would behave fairly strange. */
	assert(aircraft_min_altitude < aircraft_middle_altitude);
	assert(aircraft_middle_altitude < aircraft_max_altitude);

	int z = v->z_pos;
	if (z < aircraft_min_altitude ||
			(HasBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION) && z < aircraft_middle_altitude)) {
		/* Ascend. And don't fly into that mountain right ahead.
		 * And avoid our aircraft become a stairclimber, so if we start
		 * correcting altitude, then we stop correction not too early. */
		SetBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION);
		z += 1;
	} else if (z > aircraft_max_altitude ||
			(HasBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION) && z > aircraft_middle_altitude)) {
		/* Descend lower. You are an aircraft, not an space ship.
		 * And again, don't stop correcting altitude too early. */
		SetBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION);
		z--;
	} else if (HasBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION) && z >= aircraft_middle_altitude) {
		/* Now, we have corrected altitude enough. */
		ClrBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION);
	} else if (HasBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION) && z <= aircraft_middle_altitude) {
		/* Now, we have corrected altitude enough. */
		ClrBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION);
	}

	return z;
}

int GetAircraftFlightLevel(Aircraft *v)
{
	assert(v->IsAircraftMoving());

	switch (v->cur_state) {
		case AM_MOVING:
			return GetTileMaxPixelZ(v->tile) + 1;
		case AM_TAKEOFF:
			return GetTileMaxPixelZ(v->tile);
			break;
		case AM_HELICOPTER_TAKEOFF:
			NOT_REACHED();
		case AM_LANDING:
			break;
		case AM_HELICOPTER_LANDING:
			NOT_REACHED();
		case AM_FLYING:
			break;
		default: NOT_REACHED();
	}

	/* Aircraft is in flight. We want to enforce it being somewhere
	 * between the minimum and the maximum allowed altitude. */
	int aircraft_min_altitude;
	int aircraft_max_altitude;
	GetAircraftFlightLevelBounds(v, &aircraft_min_altitude, &aircraft_max_altitude);
	int aircraft_middle_altitude = (aircraft_min_altitude + aircraft_max_altitude) / 2;

	/* If those assumptions would be violated, aircraft would behave fairly strange. */
	assert(aircraft_min_altitude < aircraft_middle_altitude);
	assert(aircraft_middle_altitude < aircraft_max_altitude);

	int z = v->z_pos;
	if (z < aircraft_min_altitude ||
			(HasBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION) && z < aircraft_middle_altitude)) {
		/* Ascend. And don't fly into that mountain right ahead.
		 * And avoid our aircraft become a stairclimber, so if we start
		 * correcting altitude, then we stop correction not too early. */
		SetBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION);
	z += (v->cur_state == AM_TAKEOFF) ? 2 : 1;
	} else if (v->cur_state != AM_TAKEOFF && ((z > aircraft_max_altitude ||
			(HasBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION) && z > aircraft_middle_altitude)))) {
		/* Descend lower. You are an aircraft, not an space ship.
		 * And again, don't stop correcting altitude too early. */
		SetBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION);
		z--;
	} else if (HasBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION) && z >= aircraft_middle_altitude) {
		/* Now, we have corrected altitude enough. */
		ClrBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION);
	} else if (HasBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION) && z <= aircraft_middle_altitude) {
		/* Now, we have corrected altitude enough. */
		ClrBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION);
	}

	return z;
}


void HandleMissingAircraftOrders(Aircraft *v)
{
	/*
	 * We do not have an order. This can be divided into two cases:
	 * 1) we are heading to an invalid station. In this case we must
	 *    find another airport to go to. If there is nowhere to go,
	 *    we will destroy the aircraft as it otherwise will enter
	 *    the holding pattern for the first airport, which can cause
	 *    the plane to go into an undefined state when building an
	 *    airport with the same StationID.
	 * 2) we are (still) heading to a (still) valid airport, then we
	 *    can continue going there. This can happen when you are
	 *    changing the aircraft's orders while in-flight or in for
	 *    example a depot. However, when we have a current order to
	 *    go to a depot, we have to keep that order so the aircraft
	 *    actually stops.
	 */
	const Station *st = GetTargetAirportIfValid(v);
	if (st == NULL) {
		Backup<CompanyByte> cur_company(_current_company, v->owner, FILE_LINE);
		CommandCost ret = DoCommand(v->tile, v->index, 0, DC_EXEC, CMD_SEND_VEHICLE_TO_DEPOT);
		cur_company.Restore();

		if (ret.Failed()) CrashAirplane(v);
	} else if (!v->current_order.IsType(OT_GOTO_DEPOT)) {
		v->current_order.Free();
	}
}


TileIndex Aircraft::GetOrderStationLocation(StationID station)
{
	if (station == this->last_station_visited) this->last_station_visited = INVALID_STATION;

	const Station *st = Station::Get(station);
	if (!CanVehicleUseStation(this, st)) {
		/* There is no stop left at the station, so don't even TRY to go there */
		this->IncrementRealOrderIndex();
		return INVALID_TILE;
	}

	return st->xy;
}


uint Aircraft::Crash(bool flooded)
{
	uint pass = Vehicle::Crash(flooded) + 2; // pilots
	this->crashed_counter = flooded ? 9000 : 0; // max 10000, disappear pretty fast when flooded

	return pass;
}

/**
 * Bring the aircraft in a crashed state, create the explosion animation,
 * and create a news item about the crash.
 * @param v Aircraft that crashed.
 */
static void CrashAirplane(Aircraft *v)
{
	CreateEffectVehicleRel(v, 4, 4, 8, EV_EXPLOSION_LARGE);

	uint pass = v->Crash();
	SetDParam(0, pass);

	v->cargo.Truncate();
	v->Next()->cargo.Truncate();
	const Station *st = GetTargetAirportIfValid(v);
	StringID newsitem;
	if (st == NULL) {
		newsitem = STR_NEWS_PLANE_CRASH_OUT_OF_FUEL;
	} else {
		SetDParam(1, st->index);
		newsitem = STR_NEWS_AIRCRAFT_CRASH;
	}

	AI::NewEvent(v->owner, new ScriptEventVehicleCrashed(v->index, v->tile, st == NULL ? ScriptEventVehicleCrashed::CRASH_AIRCRAFT_NO_AIRPORT : ScriptEventVehicleCrashed::CRASH_PLANE_LANDING));
	Game::NewEvent(new ScriptEventVehicleCrashed(v->index, v->tile, st == NULL ? ScriptEventVehicleCrashed::CRASH_AIRCRAFT_NO_AIRPORT : ScriptEventVehicleCrashed::CRASH_PLANE_LANDING));

	AddVehicleNewsItem(newsitem, NT_ACCIDENT, v->index, st != NULL ? st->index : INVALID_STATION);

	ModifyStationRatingAround(v->tile, v->owner, -160, 30);
	if (_settings_client.sound.disaster) SndPlayVehicleFx(SND_12_EXPLOSION, v);
}

/**
 * Handle crashed aircraft \a v.
 * @param v Crashed aircraft.
 * @return whether the vehicle is still valid.
 * @revise when crash, fall down and crash everytinhg in a 3x3 area.
 * @revise if crashes in an airport, destroy infrastructure and block any modification or movement in airport
 */
static bool HandleCrashedAircraft(Aircraft *v)
{
	v->crashed_counter += 3;
	DEBUG(misc, 0, "Crashed %d", v->crashed_counter);

	Station *st = GetTargetAirportIfValid(v);

	/* make aircraft crash down to the ground */
	if (v->crashed_counter < 500 && st == NULL && ((v->crashed_counter % 3) == 0) ) {
		int z = GetSlopePixelZ(Clamp(v->x_pos, 0, MapMaxX() * TILE_SIZE), Clamp(v->y_pos, 0, MapMaxY() * TILE_SIZE));
		v->z_pos -= 1;
		if (v->z_pos == z) {
			v->crashed_counter = 500;
			v->z_pos++;
		}
	}

	if (v->crashed_counter < 650) {
		uint32 r;
		if (Chance16R(1, 32, r)) {
			static const DirDiff delta[] = {
				DIRDIFF_45LEFT, DIRDIFF_SAME, DIRDIFF_SAME, DIRDIFF_45RIGHT
			};

			v->direction = ChangeDir(v->direction, delta[GB(r, 16, 2)]);
			SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
			r = Random();
			CreateEffectVehicleRel(v,
				GB(r, 0, 4) - 4,
				GB(r, 4, 4) - 4,
				GB(r, 8, 4),
				EV_EXPLOSION_SMALL);
		}
	} else if (v->crashed_counter >= 10000) {
		/*  remove rubble of crashed airplane */

		if (st != NULL) {
			// revise clear reservation in case we weren't flying
		}

		delete v;

		return false;
	}

	return true;
}

/**
 * Handle smoke of broken aircraft.
 * @param v Aircraft
 * @param mode Is this the non-first call for this vehicle in this tick?
 */
static void HandleAircraftSmoke(Aircraft *v, bool mode)
{
	static const struct {
		int8 x;
		int8 y;
	} smoke_pos[] = {
		{  5,  5 },
		{  6,  0 },
		{  5, -5 },
		{  0, -6 },
		{ -5, -5 },
		{ -6,  0 },
		{ -5,  5 },
		{  0,  6 }
	};

	if (!(v->vehstatus & VS_AIRCRAFT_BROKEN)) return;

	/* Stop smoking when landed */
	if (v->cur_speed < 10) {
		v->vehstatus &= ~VS_AIRCRAFT_BROKEN;
		v->breakdown_ctr = 0;
		return;
	}

	/* Spawn effect et most once per Tick, i.e. !mode */
	if (!mode && (v->tick_counter & 0x0F) == 0) {
		CreateEffectVehicleRel(v,
			smoke_pos[v->direction].x,
			smoke_pos[v->direction].y,
			2,
			EV_BREAKDOWN_SMOKE_AIRCRAFT
		);
	}
}

void Aircraft::MarkDirty()
{
	this->colourmap = PAL_NONE;
	this->UpdateViewport(true, false);
	if (this->subtype == AIR_HELICOPTER) {
		GetRotorImage(this, EIT_ON_MAP, &this->Next()->Next()->sprite_seq);
	}
}

/**
 * Aircraft arrives at an airport. If it is the first time, send out a news item.
 * @param v  Aircraft that arrived.
 * @param st Station being visited.
 */
static void AircraftArrivesAt(const Vehicle *v, Station *st)
{
	/* Check if station was ever visited before */
	if (!(st->had_vehicle_of_type & HVOT_AIRCRAFT)) {
		st->had_vehicle_of_type |= HVOT_AIRCRAFT;

		SetDParam(0, st->index);
		AddVehicleNewsItem(
			STR_NEWS_FIRST_AIRCRAFT_ARRIVAL,
			(v->owner == _local_company) ? NT_ARRIVAL_COMPANY : NT_ARRIVAL_OTHER,
			v->index,
			st->index
		);
		AI::NewEvent(v->owner, new ScriptEventStationFirstVehicle(st->index, v->index));
		Game::NewEvent(new ScriptEventStationFirstVehicle(st->index, v->index));
	}
}

/**
 * Checks if an aircraft is at its next desired state.
 * @param v aircraft
 * @return true if it is at its desired state, false if it needs to move towards another tile.
 */
static bool CheckPartialDestination(Aircraft *v)
{
	/* Stay quiet if it reached the target state. */
	if (v->cur_state != v->next_state || v->tile != v->next_tile) return false;
	// if (v->tile != v->next_tile) return false;
	if ((v->x_pos & 0xF) != 8 || (v->y_pos & 0xF) != 8) return false;
	if (v->trackdir != v->next_trackdir) return false;

	DEBUG(misc, 0, "yes, partial destination checked");
	return true;
}

/**
 * Checks if an aircraft is at its next desired state.
 * @param v aircraft
 * @return true if it is at its desired state, false if it needs to move towards another tile.
 */
static bool CheckDestination(Aircraft *v)
{
	if (!IsTileType(v->tile, MP_STATION)) return false;
	if (v->IsAircraftFlying()) return false;
	if (v->tile != v->next_tile) return false;
	if ((v->x_pos & 0xF) != 8 || (v->y_pos & 0xF) != 8) return false;
	return v->trackdir == v->next_trackdir;
}

/**
 * Checks is a path reservation can be made towards
 * next target of the aircraft.
 * @param v aircraft for the path.
 * @return whether a path can be reserved.
 */
bool TryReservePath(Aircraft *v)
{
	DEBUG(misc, 0, "Trying start...");

	/* First, assert diagonal diadgir.
	 * We shouldn't start paths in stranger tracks. */
	v->next_tile = INVALID_TILE;
	Trackdir trackdir = v->GetVehicleTrackdir();
	TrackBits reserved = GetReservedAirportTracks(v->tile);
	//RemoveAirportTrackReservation(v->tile, TrackdirToTrack(trackdir));
	assert(IsDiagonalTrackdir(trackdir));

	/* Then, if current state is hangar and we are in a small hangar, make sure it is not reserved. */
	if (v->cur_state == AM_HANGAR) {
		assert(IsHangarTile(v->tile));
		//DEBUG(misc, 0, "...Small hangar already reserved.");
		if (IsSmallHangar(v->tile) && HasAirportTrackReserved(v->tile)) return false;
	} else {
		assert(reserved != TRACK_BIT_NONE);
	}

	PBSTileInfo best_dest;
	Trackdir first_trackdir = YapfAircraftFindPath(v, &best_dest, false);
	assert(first_trackdir == INVALID_TRACKDIR || IsDiagonalTrackdir(first_trackdir));

	if (first_trackdir == INVALID_TRACKDIR) {
		first_trackdir = YapfAircraftFindPath(v, &best_dest, true);
		v->HandlePathfindingResult(first_trackdir != INVALID_TRACKDIR);
		return false;
	}

	v->HandlePathfindingResult(true);

	assert(best_dest.okay);
	v->next_tile = best_dest.tile;
	v->next_trackdir = best_dest.trackdir;

	/* In order to solve problems with loops (and lifting reservations),
	 * store the tile and trackdir of the first track reservation
	 * in the shadow. We can later access it. */
	v->Next()->next_tile = v->tile;
	v->Next()->next_trackdir = v->trackdir;

	// revise possible unneeded servicing here
	/* If a path is found, service, reserve and return true. */
	if (v->cur_state == AM_HANGAR) {
		if (first_trackdir != v->trackdir) v->desired_trackdir = first_trackdir;
		SetAirportTracksReservation(v->tile, TrackToTrackBits(TrackdirToTrack(first_trackdir)));
		DepotID depot_id = GetDepotIndex(v->tile);
		assert(depot_id != INVALID_DEPOT);

		v->cur_speed = 0;
		v->subspeed = 0;
		v->progress = 0;

		/* Rotor blades */
		if (v->Next()->Next() != NULL) {
			v->Next()->Next()->cur_speed = 80;
		}

		if (IsBigHangar(v->tile)) {
			assert(TrackToTrackBits(TrackdirToTrack(v->trackdir)) == GetReservedAirportTracks(v->tile));
			Trackdir exit_dir = DiagDirToDiagTrackdir(GetHangarDirection(v->tile));
			if (exit_dir != v->trackdir) v->desired_trackdir = exit_dir;
		} else {
			v->SetHidden(false);
			v->UpdateViewport(true, true);
			v->UpdatePosition();
		}

		VehicleServiceInDepot(v);
		InvalidateWindowData(WC_VEHICLE_DEPOT, depot_id);
		SetWindowClassesDirty(WC_AIRCRAFT_LIST);
		v->PlayLeaveStationSound();

	}

	DEBUG(misc, 0, "1st trackdir %d (invalid %d)", first_trackdir, INVALID_TRACKDIR);
	assert(IsDiagonalTrackdir(first_trackdir));
	if (first_trackdir != v->GetVehicleTrackdir()) {
		v->desired_trackdir = first_trackdir;
		assert(v->Next()->next_tile == v->tile);
		v->Next()->next_trackdir = first_trackdir;
		DEBUG(misc, 0, "Already reserved tracks %d", GetReservedAirportTracks(v->tile));
		assert(HasAirportTracksReserved(v->tile, TrackToTrackBits(TrackdirToTrack(v->trackdir))));
		if (GetReservedAirportTracks(v->tile) == TRACK_BIT_CROSS) {
			RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
		}
	}

	if (v->tile == v->next_tile) {
		DEBUG(misc, 0, "We are not really moving");
	} else {
		DEBUG(misc, 0, "YES moving");
		v->cur_state = AM_MOVING;
	}

	return true;
}

const char* air_states[] = { "Idle", "Hangar", "Terminal", "Helipad", "Moving", "Takeoff", "Helicoptertakeoff", "Flying", "Landing", "Helicopterlanding", "End-Invalid"};

void DumpAircraftState(Aircraft *v)
{
	DEBUG(misc, 0, "Dumping info of aircraft %d (index %d)", v->unitnumber, v->index);
	DEBUG(misc, 0, "Current %s, next %s, target %s",
					air_states[v->cur_state], air_states[v->next_state], air_states[v->target_state]);
	DEBUG(misc, 0, "Current tile %d, next_tile %d, dest_tile %d", v->tile, v->next_tile, v->dest_tile);
	StationID st_id = IsTileType(v->tile, MP_STATION) ? GetStationIndex(v->tile) : 0;
	DEBUG(misc, 0, "Actual station %d. Target station %d", st_id, v->targetairport);
	DEBUG(misc, 0, "Trackdir %d, next_trackdir %d, desired_trackdir %d, invalid trackdir %d", v->trackdir,
			v->next_trackdir, v->desired_trackdir, INVALID_TRACKDIR);
}

byte GetFakeDelta(uint orig, uint dest) {
	if (orig == dest) return 1;
	if (orig  < dest) return 2;
	return 0;
}

TileIndex GetClosestLandingTile(Aircraft *v)
{
	v->targetairport = v->current_order.GetDestination();

	if (v->targetairport == INVALID_STATION) return INVALID_TILE;

	TileIndex landing_tile = INVALID_TILE;
	uint32 best_dist = UINT32_MAX;
	Station *st = Station::Get(v->targetairport);
	assert(st != NULL);

	// Make helicopters land in helipads if possible
	if (v->IsHelicopter()) {
		for (uint i = st->airport.helipads.Length(); i--;) {
			if (DistanceSquare(st->airport.helipads[i], v->tile) < best_dist) {
				landing_tile = st->airport.helipads[i];
				best_dist = DistanceSquare(st->airport.helipads[i], v->tile);
			}
		}

		for (uint i = st->airport.terminals.Length(); i--;) {
			if (DistanceSquare(st->airport.terminals[i], v->tile) < best_dist) {
				landing_tile = st->airport.terminals[i];
				best_dist = DistanceSquare(st->airport.terminals[i], v->tile);
			}
		}

		return landing_tile;
	}

	for (uint i = st->airport.runways.Length(); i--;) {
		if (!IsLandingTypeTile(st->airport.runways[i])) continue;
		if (DistanceSquare(st->airport.runways[i], v->tile) < best_dist) {
			landing_tile = st->airport.runways[i];
			best_dist = DistanceSquare(st->airport.runways[i], v->tile);
		}
	}

	return landing_tile;
}

bool DoesAircraftNeedRotation(Aircraft *v)
{
	return v->desired_trackdir != INVALID_TRACKDIR;
}

void DoRotationStep(Aircraft *v)
{
	assert(v->desired_trackdir != INVALID_TRACKDIR);
	assert(IsDiagonalTrackdir(v->desired_trackdir));
	if (v->trackdir == v->desired_trackdir) {
		v->desired_trackdir = INVALID_TRACKDIR;
		v->Unstuck();
	} else {
		DirDiff difference = DirDifference(v->direction, DiagDirToDir(TrackdirToExitdir(v->desired_trackdir)));
		if (difference == DIRDIFF_SAME) {
			DumpAircraftState(v);
			NOT_REACHED(); // revise, should be not_reached
		} else if (difference <= DIRDIFF_REVERSE) {
			difference = DIRDIFF_45LEFT;
		} else {
			difference = DIRDIFF_45RIGHT;
		}
		v->direction = ChangeDir(v->direction, difference);
		if (IsDiagonalDirection(v->direction)) v->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
		if (v->IsStuck()) {
		} else {
			v->MarkAsStuck();
		}
	}

	for (Aircraft *u = v; u != NULL; u = u->Next()) {
		if ((u->vehstatus & VS_HIDDEN) != 0) continue;
		v->UpdatePosition();
		u->UpdateViewport(true, true);
	}
}

bool CanRunwayBeReserved(TileIndex tile)
{
	assert(IsRunwayExtreme(tile));
	DiagDirection dir = GetRunwayDirection(tile);
	if (IsRunwayEnd(tile)) dir = ReverseDiagDir(dir);
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	do {
		if (HasAirportTileAnyReservation(tile)) return false;
		tile = TILE_ADD(tile, diff);
	} while (!IsRunwayExtreme(tile));

	return !HasAirportTileAnyReservation(tile);
}

Trackdir GetFreeTrackdir(TileIndex tile, Trackdir prefered_trackdir)
{
	DEBUG(misc, 0, "Trying a free trackdir");
	assert(IsDiagonalTrackdir(prefered_trackdir));
	TrackBits tracks = GetAirportTileTracks(tile) & TRACK_BIT_CROSS;
	assert(tracks != TRACK_BIT_NONE);

	Track prefered_track = TrackdirToTrack(prefered_trackdir);
	if ((TrackToTrackBits(prefered_track) & tracks) != 0) {
		if (IsAirportPositionFree(tile, prefered_track)) return prefered_trackdir;
	}

	tracks &= ~TrackToTrackBits(prefered_track);

	if (tracks != TRACK_BIT_NONE) {
		prefered_track = RemoveFirstTrack(&tracks);
		if (IsAirportPositionFree(tile, prefered_track)) return TrackToTrackdir(prefered_track);
		assert(tracks == TRACK_BIT_NONE);
	}
	DEBUG(misc, 0, "not found free trackdir");

	return INVALID_TRACKDIR;
}

void HandleAircraftEnterHangar(Aircraft *v)
{
	assert(IsHangarTile(v->tile));

	if (IsBigHangar(v->tile)) {
		v->cur_speed = 0;
		UpdateAircraftCache(v);
		v->UpdateViewport(true, true);
		SetWindowClassesDirty(WC_SHIPS_LIST);
		SetWindowDirty(WC_VEHICLE_VIEW, v->index);

		InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
		v->StartService();
	} else {
		v->SetHidden(true);
		VehicleEnterDepot(v);
		RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
	}

	v->cur_state = AM_HANGAR;
}


/**
 * Updates target state.
 * @param v aircraft.
 */
void UpdateTargetState(Aircraft *v)
{
	DEBUG(misc, 0, "Update target state");
	/* Aircraft will only update next partial destination when
	 * it reached the previous one and it is currently not moving. */
	//if (v->IsAircraftMoving()) return;

	//assert(IsTileType(v->dest_tile, MP_STATION)); // revise removed airport

	switch (v->current_order.GetType()) {
		case OT_GOTO_STATION:
			v->target_state = v->IsHelicopter() ? AM_HELIPAD : AM_TERMINAL;
			v->targetairport = v->current_order.GetDestination();
			break;

		case OT_NOTHING:
			if (Station::GetByTile(v->tile)->airport.HasHangar()) {
				v->target_state = AM_HANGAR;
				v->targetairport = GetStationIndex(v->tile);
				// v->targetairport = GetStationIndex(v->tile); it should already be that.
				DEBUG(misc, 0, "Ok, empty order. Remember station index: %d and destination is %d, and depot index", GetStationIndex(v->tile), v->current_order.GetDestination());
			}
			break;

		case OT_GOTO_DEPOT:
			v->target_state = AM_HANGAR;
			v->targetairport = v->current_order.GetDestination();
			DEBUG(misc, 0, "Ok, hangar. Remember station index: %d and destination is %d, and depot index", GetStationIndex(v->tile), v->current_order.GetDestination());
			break;

		case OT_LOADING:
		case OT_LEAVESTATION:
		case OT_DUMMY:
		case OT_GOTO_WAYPOINT:
		case OT_CONDITIONAL:
		case OT_IMPLICIT:
		default:
			v->target_state = AM_IDLE;
			break;
	}
	
	DEBUG(misc, 0, "End update target state");
}

bool CheckAircraftSanity(Aircraft *v)
{
	return v->cur_state == AM_HANGAR || v->IsAircraftFlying() || HasAirportTrackReserved(v->tile) || IsValidTrackdir(v->trackdir);
}

void AssignLandingTile(Aircraft *v, TileIndex tile)
{
	if (IsValidTile(tile)) {
		v->next_state = AM_LANDING;
		if (IsTerminal(tile)) {
			assert(v->IsHelicopter());
			switch (GetTerminalType(tile)) {
				case HTT_TERMINAL:
				case HTT_HELIPAD:
					v->next_state = AM_HELICOPTER_LANDING;
					break;
				case HTT_BUILTIN_HELIPORT:
					/* Oil rig heliport is not the actual station tile. */
					tile +=  ToTileIndexDiff({2, 0});
				case HTT_HELIPORT:
					v->next_state = AM_HELICOPTER_LANDING;
					break;
				default: NOT_REACHED();
			}
		}
		v->next_tile = tile;
	} else {
		v->next_state = AM_IDLE;
		v->next_tile = INVALID_TILE;
	}
}

/**
 * Updates partial state.
 * @param v aircraft.
 */
void AircraftArrivesAtPartialDestination(Aircraft *v)
{
	switch (v->next_state) {
		default:
			assert(IsTileType(v->tile, MP_STATION) && IsAirportTile(v->tile));
			break;
		case AM_HELICOPTER_LANDING:
			if (!IsBuiltInHeliportTile(v->tile)) break;
			TileIndex heli_tile = v->tile + ToTileIndexDiff({-2, 0});
			assert(IsAirportTile(heli_tile));
			Station *st = Station::GetByTile(v->tile);
			assert(st != NULL && st->airport.type == AT_OILRIG);
			v->tile = heli_tile;
			break;
	}


	DEBUG(misc, 0, "Arrive at part dest");
	DumpAircraftState(v);
	/* Fill in target state and target airport. */
	UpdateTargetState(v);

	/* Process arrival. */
	switch (v->cur_state) {
		case AM_IDLE:
		case AM_HANGAR:
		case AM_TERMINAL:
		case AM_HELIPAD:
			break;
		case AM_MOVING:
			LiftAirportPathReservation(v, true, false);
			break;
		case AM_TAKEOFF:
			assert(IsRunway(v->tile));
			assert(GetReservationAsRunway(v->tile));
			SetPlatformReservation(v->tile, INVALID_DIAGDIR, false);
			break;
		case AM_HELICOPTER_TAKEOFF:
			// revise free tile
			break;
		case AM_FLYING:
			break;
		case AM_LANDING: {
			DEBUG(misc, 0, "Aircraft number %d", v->unitnumber);
			assert(IsRunway(v->tile));
			assert(GetReservationAsRunway(v->tile));
			assert(!HasAirportTrackReserved(v->tile));
			SetPlatformReservation(v->tile, INVALID_DIAGDIR, false);
			Trackdir trackdir = GetFreeTrackdir(v->tile, v->trackdir);
			if (trackdir == INVALID_TRACKDIR) {
				SetPlatformReservation(v->tile, INVALID_DIAGDIR, true);
				return;
			}
			if (v->trackdir != trackdir) v->desired_trackdir = trackdir;
			v->next_trackdir = trackdir;
			SetAirportTrackReservation(v->tile, TrackdirToTrack(trackdir));
			v->cur_state = AM_IDLE;
			CheckAircraftSanity(v);
			break;
		}
		case AM_HELICOPTER_LANDING:
			break;
		default:
			break;
	}

	/* Update next state. */
	/* Deal with idle aircraft. */
	if (v->target_state == AM_IDLE) {
		switch (v->cur_state) {
			case AM_HANGAR:
				/* Stay here. */
				v->cur_state = AM_HANGAR;
				v->next_state = AM_HANGAR;
				break;
			case AM_TAKEOFF:
			case AM_HELICOPTER_TAKEOFF:
			case AM_FLYING:
			case AM_LANDING:
			case AM_HELICOPTER_LANDING:
				/* Let it fly. */
				break;
			default:
				v->cur_state = AM_IDLE;
				v->next_state = AM_HANGAR;
				break;
		}
	} else {
		/* Not idle. */
		/* Does aircraft need to go to a different airport? */
		if (GetStationIndex(v->tile) != v->targetairport) {
			if (v->IsAircraftFlying()) {
				/* Get a landing tile on target airport. */
				AssignLandingTile(v, GetClosestLandingTile(v));
			} else {
				if (!v->IsAircraftFlying() && v->IsHelicopter() && IsTerminal(v->tile)) {
					v->cur_state = AM_HELICOPTER_TAKEOFF;
				} else {
					v->next_state = AM_TAKEOFF;
				}
			}
		} else {
			/* Aircraft is in the right airport. */
			if (v->cur_state == AM_FLYING) {
				if (!IsAirportTile(v->next_tile)) {
					DEBUG(misc, 0, "Not a tile");
					NOT_REACHED();
					v->next_state = AM_IDLE;
				} else if (IsRunwayStart(v->next_tile)) {
					v->next_state = AM_LANDING;
				} else if (IsAirportTile(v->next_tile) && IsTerminal(v->next_tile)) {
					v->next_state = AM_HELICOPTER_LANDING;
				} else {
					NOT_REACHED();
				}
			} else {
				v->next_state = v->target_state;
			}
		}
	}

	/* Process state. */
	switch (v->cur_state) {
		case AM_IDLE:
		case AM_HANGAR:
		case AM_TERMINAL:
		case AM_HELIPAD:
			break;
		case AM_MOVING:
			if (v->next_state == AM_HANGAR) {
				if (IsHangarTile(v->tile)) {
					HandleAircraftEnterHangar(v);
				} else {
					/* Right now not in hangar. Go to some hangar. */
					v->cur_state = AM_IDLE;
				}
			} else if (v->next_state == AM_TAKEOFF) {
				assert(IsRunwayStart(v->tile));
				assert(HasAirportTrackReserved(v->tile));
				assert(RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir)));
				if (!CanRunwayBeReserved(v->tile)) {
					SetAirportTracksReservation(v->tile, TrackToTrackBits(TrackdirToTrack(v->trackdir)));
					break;
				}
				SetPlatformReservation(v->tile, INVALID_DIAGDIR, true);
				v->cur_state = AM_TAKEOFF;
				v->next_state = AM_FLYING;
				v->next_tile = GetOtherStartPlatformTile(v->tile);
				v->next_trackdir = DiagDirToDiagTrackdir(GetRunwayDirection(v->tile));
				if (DiagDirToDir(GetRunwayDirection(v->tile)) != v->direction) {
					DEBUG(misc, 0, "Needs rotation");
					v->desired_trackdir = DiagDirToDiagTrackdir(GetRunwayDirection(v->tile));
					assert(v->trackdir != v->desired_trackdir);
					break;
				}
			}
			break;
		case AM_TAKEOFF:
			v->cur_state = AM_FLYING;
			AssignLandingTile(v, GetClosestLandingTile(v));
			break;
		case AM_HELICOPTER_TAKEOFF:
			break;
		case AM_FLYING:
			v->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
			if (v->next_state == AM_LANDING) {
				assert(IsRunwayStart(v->tile));
				if (IsAirportClosed(v->tile)) break;
				if (!CanRunwayBeReserved(v->tile)) break;
				SetPlatformReservation(v->tile, INVALID_DIAGDIR, true);
				v->next_trackdir = DiagDirToDiagTrackdir(GetRunwayDirection(v->tile));
				if (v->trackdir != v->next_trackdir) v->desired_trackdir = v->next_trackdir;
				v->cur_state = AM_LANDING;
				v->next_state = AM_IDLE;
				v->next_tile = GetOtherStartPlatformTile(v->tile);
			} else if (v->next_state == AM_HELICOPTER_LANDING) {
				DEBUG(misc, 0, "Trying to initiate the landing procedure for helicopters");
				assert(IsDiagonalDirection(v->direction));
				v->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
				assert(IsDiagonalTrackdir(v->trackdir));

				Trackdir trackdir = INVALID_TRACKDIR;
				if (IsHeliportTile(v->tile)) {
					trackdir = v->trackdir;
				} else if (HasAirportTileTrack(v->tile, TrackdirToTrack(v->trackdir)) &&
						IsAirportPositionFree(v->tile, TrackdirToTrack(v->trackdir))) {
					trackdir = v->trackdir;
				} else if (HasAirportTileTrack(v->tile, TRACK_X) &&
						IsAirportPositionFree(v->tile, TRACK_X)) {
					trackdir = TrackToTrackdir(TRACK_X);
				} else if (HasAirportTileTrack(v->tile, TRACK_Y) &&
						IsAirportPositionFree(v->tile, TRACK_Y)) {
					trackdir = TrackToTrackdir(TRACK_Y);
				}

				assert(trackdir == INVALID_TRACKDIR || IsDiagonalTrackdir(trackdir));

				if (trackdir == INVALID_TRACKDIR) {
					// CHECK ANOTHER POSSIBLE LANDING TILE
					break;
				} else {
					SetAirportTracksReservation(v->tile, TrackToTrackBits(TrackdirToTrack(trackdir)));
					v->next_trackdir = trackdir;
					v->Next()->next_tile = v->tile;
					v->Next()->next_trackdir = trackdir;
					if (v->trackdir != trackdir) v->desired_trackdir = trackdir;
					v->cur_state = v->next_state;
					v->next_state = AM_HELIPAD;
					v->next_tile = v->tile;
				}
			}
			break;
		case AM_LANDING:
			break;
		case AM_HELICOPTER_LANDING:
			break;
		default:
			break;
	}
}

static const byte _aircraft_subcoord[4][6][3] = {
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


void MoveAircraft(Aircraft *v, int count)
{
	assert(v->IsAircraftMoving());
	if (CheckPartialDestination(v)) {
		DEBUG(misc, 0, "CheckPartialDest but error");
		return;
	}

	for (; count > 0; count--) {
		GetNewVehiclePosResult gp = GetNewVehiclePos(v);

		static const Direction _table_dir[3][3] = {{DIR_N, DIR_NE, DIR_E}, {DIR_NW, INVALID_DIR, DIR_SE}, {DIR_W, DIR_SW, DIR_S}};

		if (v->IsAircraftFlying()) {
			uint x = TileX(v->next_tile) * TILE_SIZE + 8;
			uint y = TileY(v->next_tile) * TILE_SIZE + 8;

			byte f_x = GetFakeDelta(v->x_pos, x);
			byte f_y = GetFakeDelta(v->y_pos, y);

			gp.x = v->x_pos + f_x - 1;
			gp.y = v->y_pos + f_y - 1;
			v->direction = _table_dir[f_x][f_y];
		} else if (gp.old_tile != gp.new_tile) {
			assert(CheckAircraftSanity(v));
			LiftAirportPathReservation(v, true, true);
			assert(CheckAircraftSanity(v));

			const byte *b;
			TrackdirBits trackdirs = TrackdirReachesTrackdirs(v->trackdir) &
					TrackBitsToTrackdirBits(GetReservedAirportTracks(gp.new_tile));
			v->trackdir = RemoveFirstTrackdir(&trackdirs);

			DiagDirection diagdir = DiagdirBetweenTiles(gp.old_tile, gp.new_tile);
			b = _aircraft_subcoord[diagdir][TrackdirToTrack(v->trackdir)];
			gp.x = (gp.x & ~0xF) | b[0];
			gp.y = (gp.y & ~0xF) | b[1];

			uint32 r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
			if (HasBit(r, VETS_CANNOT_ENTER)) NOT_REACHED();

			v->direction = (Direction)b[2];
		} else {
			assert(CheckAircraftSanity(v));
			/* Check whether the aircraft must rotate in the middle of the tile. */
			if (GetReservedAirportTracks(gp.new_tile) == TRACK_BIT_CROSS) {
				if ((gp.x & 0xF) == 8 && (gp.y & 0xF) == 8) {
					LiftAirportPathReservation(v, false, false);
					assert(HasAirportTrackReserved(v->tile));
					return;
				}
			}
			assert(CheckAircraftSanity(v));
		}

		Vehicle *u = v->Next();
		v->x_pos = u->x_pos = gp.x;
		v->y_pos = u->y_pos = gp.y;
		v->tile = TileVirtXY(v->x_pos, v->y_pos);
		SetAircraftPosition(v, v->x_pos, v->y_pos, GetAircraftFlightLevel(v));

		assert(CheckAircraftSanity(v));

		/* Check if it has reached a partial destination. */
		if (CheckPartialDestination(v)) return;

		assert(CheckAircraftSanity(v));
	}
}

/**
 * Checks whether an aircraft can land on the next order destination.
 * It checks whether it can land (helipads for helicopters, whether there is a landing runway...).
 * It also checks if the destination is too far.
 */
bool IsReachableDest(Aircraft *v)
{
	if (v->targetairport == GetStationIndex(v->tile)) return true;
	if (v->targetairport == INVALID_STATION) return true;

	TileIndex closest_landing = GetClosestLandingTile(v);
	if (closest_landing == INVALID_TILE) {
		if (!HasBit(v->flags, VAF_CANNOT_LAND_DEST)) {
			SetBit(v->flags, VAF_CANNOT_LAND_DEST);
			v->MarkAsStuck(); // Stuck aircraft so we don't do the check each tick.
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
			// revise: a new event should be created
			//AI::NewEvent(v->owner, new ScriptEventAircraftNoLandDest(v->index));
			if (v->owner == _local_company) {
				/* Post a news message. */
				SetDParam(0, v->index);
				AddVehicleAdviceNewsItem(STR_NEWS_AIRCRAFT_CANNOT_LAND_NEXT_DEST, v->index);
			}
		}
		v->cur_state = AM_IDLE;
		v->dest_tile = v->tile;
		return false;
	} else if (HasBit(v->flags, VAF_CANNOT_LAND_DEST)) {
		/* Aircraft can land now. */
		ClrBit(v->flags, VAF_CANNOT_LAND_DEST);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		DeleteVehicleNews(v->index, STR_NEWS_AIRCRAFT_CANNOT_LAND_NEXT_DEST);
	}

	if (v->acache.cached_max_range_sqr == 0) return true;
	Station *cur_st = Station::GetIfValid(GetStationIndex(v->tile));

	if (DistanceSquare(cur_st->airport.tile, closest_landing) > v->acache.cached_max_range_sqr) {
		if (!HasBit(v->flags, VAF_DEST_TOO_FAR)) {
			SetBit(v->flags, VAF_DEST_TOO_FAR);
			v->MarkAsStuck(); // Stuck aircraft so we don't do the check each tick.
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
			AI::NewEvent(v->owner, new ScriptEventAircraftDestTooFar(v->index));
			if (v->owner == _local_company) {
				/* Post a news message. */
				SetDParam(0, v->index);
				AddVehicleAdviceNewsItem(STR_NEWS_AIRCRAFT_DEST_TOO_FAR, v->index);
			}
		}
		return false;
	}

	if (HasBit(v->flags, VAF_DEST_TOO_FAR)) {
		/* Not too far anymore, clear flag and message. */
		ClrBit(v->flags, VAF_DEST_TOO_FAR);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		DeleteVehicleNews(v->index, STR_NEWS_AIRCRAFT_DEST_TOO_FAR);
	}

	return true;
}

/**
 * Aircraft controller.
 * @param v Aircraft to move.
 * @param mode False during the first call in each tick, true during second call.
 * @return whether the vehicle is still valid.
 */
static void AircraftController(Aircraft *v, bool mode)
{
	assert(CheckAircraftSanity(v));
	v->HandleBreakdown();

	HandleAircraftSmoke(v, mode);

	ProcessOrders(v);
	v->HandleLoading(mode);

	if (v->current_order.IsType(OT_LOADING)) return;

	if (DoesAircraftNeedRotation(v)) return DoRotationStep(v);

	assert(CheckAircraftSanity(v));

	switch (v->current_order.GetType()) {
		case OT_LEAVESTATION:
			/* A leave station order only needs one tick to get processed,
			 * so we can always skip ahead. */
			v->current_order.Free();
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
			v->next_state = AM_IDLE;
			return;

		case OT_GOTO_STATION: {
			/* Destination may have changed. */
			DEBUG(misc, 0, "Gotto station");
			DumpAircraftState(v);
			UpdateTargetState(v);
			DumpAircraftState(v);
			DEBUG(misc, 0, "inside station");

			if (CheckDestination(v) && IsTerminal(v->tile) && v->targetairport == GetStationIndex(v->tile)) {
				assert(HasAirportTrackReserved(v->tile));
				LiftAirportPathReservation(v, true, false);
				v->last_station_visited = v->current_order.GetDestination();
				v->cur_state = IsHelipadTile(v->tile) ? AM_HELIPAD : AM_TERMINAL;
				AircraftArrivesAt(v, Station::GetByTile(v->tile));
				if (_settings_game.order.serviceathelipad && v->subtype == AIR_HELICOPTER &&
							(IsHelipad(v->tile) || IsTerminal(v->tile))) {
						/* an excerpt of ServiceAircraft, without the invisibility stuff */
					v->date_of_last_service = _date;
					v->breakdowns_since_last_service = 0;
					v->reliability = v->GetEngine()->reliability;
					SetWindowDirty(WC_VEHICLE_DETAILS, v->index);
				}
				v->BeginLoading();
				DEBUG(misc, 0, "loading begun");
				return;
			}

			DEBUG(misc, 0, "not started");
		}

		default:
			break;
	}

	assert(CheckAircraftSanity(v));

	DEBUG(misc, 0, "Before check partial dest");

	if (CheckPartialDestination(v)) {
		AircraftArrivesAtPartialDestination(v);
		if (v->IsServicing()) return;
		if (DoesAircraftNeedRotation(v)) return; // revise !flying?
		assert(CheckAircraftSanity(v));
	}

	DEBUG(misc, 0, "Before move");
	DumpAircraftState(v);
	if (v->IsAircraftMoving()) {
		/* The aircraft is moving (it has a reserved path or it is flying). */
		if (v->cur_state == AM_HELICOPTER_TAKEOFF) {
			Aircraft *u = v->Next()->Next();

			/* Make sure the rotors don't rotate too fast */
			if (u->cur_speed > 32) {
				v->cur_speed = 0;
				if (--u->cur_speed == 32) {
					if (!PlayVehicleSound(v, VSE_START)) {
						SndPlayVehicleFx(SND_18_HELICOPTER, v);
					}
				}
			} else {
				u->cur_speed = 32;
				int count = UpdateAircraftSpeed(v);
				if (count > 0) {
					//v->tile = 0;
					int z_dest;
					GetAircraftFlightLevelBounds(v, &z_dest, NULL);

					/* Reached altitude? */
					if (v->z_pos + count >= z_dest) {
						v->cur_speed = 0;
						v->cur_state = AM_FLYING;
						RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
					}
					SetAircraftPosition(v, v->x_pos, v->y_pos, min(v->z_pos + count, z_dest));
				}
			}
			return;
		} else if (v->cur_state == AM_HELICOPTER_LANDING) {
			/* Find altitude of landing position. */
			int z = GetTileMaxPixelZ(v->tile) + 1;
			switch (GetTerminalType(v->tile)) {
				case HTT_TERMINAL:
				case HTT_HELIPAD:
					break;
				case HTT_HELIPORT:
					z += 60;
					break;
				case HTT_BUILTIN_HELIPORT:
					z += 54;
					break;
				default: NOT_REACHED();
			}

			if (z == v->z_pos) {
				Vehicle *u = v->Next()->Next();

				/*  Increase speed of rotors. When speed is 80, we've landed. */
				if (u->cur_speed >= 80) {
					v->cur_speed = 0;
					v->cur_state = AM_HELIPAD;
					v->next_state = AM_IDLE;
					DEBUG(misc, 0, "Finally landed");
					return;
				};
				u->cur_speed += 4;
			} else {
				int count = UpdateAircraftSpeed(v);
				if (count > 0) {
					SetAircraftPosition(v, v->x_pos, v->y_pos, max(v->z_pos - count, z));
				}
			}
			return;
		}

		int count = UpdateAircraftSpeed(v, mode);
		MoveAircraft(v, count);
		assert(CheckAircraftSanity(v));
	} else {
		/* If it has no destination, do nothing. */
		if (v->cur_state == v->next_state) {
			DEBUG(misc, 0, "No destination dumping");
			DumpAircraftState(v);
			if (!v->IsStuck()) v->MarkAsStuck();
			return;
		}

		/* Check if next destination is too far. */
		if (!IsReachableDest(v)) {
			NOT_REACHED();
			if (!v->IsStuck()) v->MarkAsStuck();
			return;
		}

		DEBUG(misc, 0, "Not moving, but reachable dest");
		DumpAircraftState(v);
		/* Check aircraft can reserve a path to next state. */
		if (TryReservePath(v)) {
			if (v->IsStuck()) v->Unstuck();
		} else {
			/* Aircraft cannot reserve a path now. */
			if (!v->IsStuck()) v->MarkAsStuck();
		}

		assert(CheckAircraftSanity(v));
		assert(IsValidTile(v->tile));
	}

	assert(CheckAircraftSanity(v));

	for (Aircraft *u = v; u != NULL; u = u->Next()) {
		if ((u->vehstatus & VS_HIDDEN) != 0) continue;
		v->UpdatePosition();
		u->UpdateViewport(true, true);
	}
}

bool Aircraft::Tick()
{
	if (!this->IsNormalAircraft()) return true;

	/* Handle ticks. */
	this->tick_counter++;
	this->current_order_time++;
	if (!(this->vehstatus & VS_STOPPED)) this->running_ticks++;

	/* Handle rotor for helicopters. */
	if (this->subtype == AIR_HELICOPTER) HelicopterTickHandler(this);

	/* Aircraft has crashed? */
	if (this->vehstatus & VS_CRASHED) {
		return HandleCrashedAircraft(this); // 'this' can be deleted here
	}

	if (this->vehstatus & VS_STOPPED) return true;

	if (this->ContinueServicing()) return true;

	if (this->IsStuck() && !this->TryUnblock()) return true;

	/* Execute the controller twice, like trains do. */
	AircraftController(this, false);

	if (this->vehstatus & VS_STOPPED) return true;
	if (this->IsServicing()) return true;
	if (!this->IsStuck()) AircraftController(this, true);

	return true;
}

