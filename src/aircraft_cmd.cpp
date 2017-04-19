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

#include "table/strings.h"

#include "safeguards.h"

static const int PLANE_HOLDING_ALTITUDE = 150;  ///< Altitude of planes in holding pattern (= lowest flight altitude).
static const int HELI_FLIGHT_ALTITUDE   = 184;  ///< Normal flight altitude of helicopters.

/** Mark the vehicle as stuck. */
void Aircraft::MarkAsStuck()
{
	assert(!this->IsStuck());

	/* Set the aircraft as stucked. */
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

void Aircraft::UpdateVisibility()
{
	bool is_visible = (this->cur_state != AM_HANGAR) || IsBigHangarTile(this->tile);
	if ((this->vehstatus & VS_HIDDEN) == is_visible) {
		this->vehstatus ^= VS_HIDDEN;
		this->Next()->vehstatus ^= VS_HIDDEN;
	}
	this->UpdateViewport(true, true);
	this->UpdatePosition();
	assert(IsHangarTile(this->tile));
}

void Aircraft::FreeReservation()
{
	if (this->cur_state == AM_HANGAR) {
		if (IsBigHangar(this->tile)) {
			SetAirportTracksReservation(this->tile, TRACK_BIT_NONE);
			if (_settings_client.gui.show_airport_tracks) MarkTileDirtyByTile(this->tile);
		}
	} else {
		// free path
	}
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
			/*
			switch (this->state) {
				default: break;
				case ENDTAKEOFF:
				case LANDING:
				case HELILANDING:
				case FLYING:
					this->x_extent = 24;
					this->y_extent = 24;
					break;
			}
			*/
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

/**
 * Find the nearest hangar to v
 * INVALID_STATION is returned, if the company does not have any suitable
 * airports (like helipads only)
 * @param v vehicle looking for a hangar
 * @return the StationID if one is found, otherwise, INVALID_STATION
 */
static StationID FindNearestHangar(const Aircraft *v)
{
	uint best_dist = UINT_MAX;
	StationID best_airport = INVALID_STATION;

	Station *st;
	FOR_ALL_STATIONS(st) {
		if (st->airport.HasHangar() && st->owner == v->owner) {
			uint dist = DistanceManhattan(st->xy, v->tile);
			if (dist < best_dist) {
				best_dist = dist;
				best_airport = st->index;
			}
		}
	}

	return best_airport;
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

	if (is_custom_sprite(v->spritenum)) {
		GetCustomRotorSprite(v, false, image_type, result);
		if (result->IsValid()) return;
	}


	result->Set(SPR_ROTOR_STOPPED /* + w->state */);
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
		rotor_seq.Draw(preferred_x, y - ScaleGUITrad(5), PAL_NONE, false);
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


TileIndex FindCompatibleHangar(DepotID dep_id)
{
	assert(Depot::IsValidID(dep_id));
	Depot *depot = Depot::Get(dep_id);
	assert(depot->veh_type == VEH_AIRCRAFT);

	for (uint i = depot->depot_tiles.Length(); i--;) {
		TileIndex tile = depot->depot_tiles[i];
		if (IsSmallHangar(tile)) return tile;
		if (!HasAirportTrackReserved(tile)) return tile; // revise: should check next tile
	}

	return INVALID_TILE;
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

		v->owner = u->owner = _current_company;
		v->cur_state = AM_HANGAR;
		v->tile = tile;

		uint x = TileX(tile) * TILE_SIZE + 8;
		uint y = TileY(tile) * TILE_SIZE + 8;

		v->x_pos = u->x_pos = x;
		v->y_pos = u->y_pos = y;

		DiagDirection diag_dir = GetHangarDirection(tile);
		v->direction = u->direction = DiagDirToDir(diag_dir);

		u->z_pos = GetSlopePixelZ(x, y);
		v->z_pos = u->z_pos + 1;

		v->vehstatus = VS_STOPPED | VS_DEFPAL;
		u->vehstatus = VS_UNCLICKABLE | VS_SHADOW;

		v->spritenum = avi->image_index;

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
			w->vehstatus = VS_UNCLICKABLE;
			w->spritenum = 0xFF;
			w->subtype = AIR_ROTOR;
			w->sprite_seq.Set(SPR_ROTOR_STOPPED);
			w->random_bits = VehicleRandomBits();
			/* Use rotor's air.state to store the rotor animation frame */
			//w->state = HRS_ROTOR_STOPPED;
			w->UpdateDeltaXY(INVALID_DIR);

			u->SetNext(w);
			w->UpdatePosition();
		}

		v->UpdateVisibility();
	}

	return CommandCost();
}


bool Aircraft::FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse)
{
	const Station *st = GetTargetAirportIfValid(this);
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
			/*if (u->cur_speed >= 0x80 && u->state == HRS_ROTOR_MOVING_3) {
				u->cur_speed = 0;
			}*/
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
		//u->state = HRS_ROTOR_STOPPED;
		GetRotorImage(v, EIT_ON_MAP, &seq);
		if (u->sprite_seq == seq) return;
	} else if (tick >= spd) {
		u->tick_counter = 0;
		//u->state++;
		//if (u->state > HRS_ROTOR_MOVING_3) u->state = HRS_ROTOR_MOVING_1;
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
static int UpdateAircraftSpeed(Aircraft *v, uint speed_limit = SPEED_LIMIT_NONE, bool hard_limit = true)
{
	/**
	 * 'acceleration' has the unit 3/8 mph/tick. This function is called twice per tick.
	 * So the speed amount we need to accelerate is:
	 *     acceleration * 3 / 16 mph = acceleration * 3 / 16 * 16 / 10 km-ish/h
	 *                               = acceleration * 3 / 10 * 256 * (km-ish/h / 256)
	 *                               ~ acceleration * 77 (km-ish/h / 256)
	 */
	uint spd = v->acceleration * 77;
	byte t;

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

template <class T>
int GetAircraftFlightLevel(T *v, bool takeoff)
{
	/* Aircraft is in flight. We want to enforce it being somewhere
	 * between the minimum and the maximum allowed altitude. */
	int aircraft_min_altitude;
	int aircraft_max_altitude;
	GetAircraftFlightLevelBounds(v, &aircraft_min_altitude, &aircraft_max_altitude);
	int aircraft_middle_altitude = (aircraft_min_altitude + aircraft_max_altitude) / 2;

	/* If those assumptions would be violated, aircrafts would behave fairly strange. */
	assert(aircraft_min_altitude < aircraft_middle_altitude);
	assert(aircraft_middle_altitude < aircraft_max_altitude);

	int z = v->z_pos;
	if (z < aircraft_min_altitude ||
			(HasBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION) && z < aircraft_middle_altitude)) {
		/* Ascend. And don't fly into that mountain right ahead.
		 * And avoid our aircraft become a stairclimber, so if we start
		 * correcting altitude, then we stop correction not too early. */
		SetBit(v->flags, VAF_IN_MIN_HEIGHT_CORRECTION);
		z += takeoff ? 2 : 1;
	} else if (!takeoff && (z > aircraft_max_altitude ||
			(HasBit(v->flags, VAF_IN_MAX_HEIGHT_CORRECTION) && z > aircraft_middle_altitude))) {
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

template int GetAircraftFlightLevel(DisasterVehicle *v, bool takeoff);


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
		return 0;
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
 * Bring the aircraft in a crashed state, create the explosion animation, and create a news item about the crash.
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
 * Aircraft is about to leave the hangar.
 * @param v Aircraft leaving.
 * @param exit_dir The direction the vehicle leaves the hangar.
 * @note This function is called in AfterLoadGame for old savegames, so don't rely
 *       on any data to be valid, especially don't rely on the fact that the vehicle
 *       is actually on the ground inside a depot.
 */
void AircraftLeaveHangar(Aircraft *v, Direction exit_dir)
{
	v->cur_speed = 0;
	v->subspeed = 0;
	v->progress = 0;
	v->direction = exit_dir;
	v->vehstatus &= ~VS_HIDDEN;
	{
		Vehicle *u = v->Next();
		u->vehstatus &= ~VS_HIDDEN;

		/* Rotor blades */
		u = u->Next();
		if (u != NULL) {
			u->vehstatus &= ~VS_HIDDEN;
			u->cur_speed = 80;
		}
	}

	VehicleServiceInDepot(v);
	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
	InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
	SetWindowClassesDirty(WC_AIRCRAFT_LIST);
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

/**
 * Updates target state and the next desired state.
 * @param v aircraft.
 */
static void UpdateState(Aircraft *v)
{
	DEBUG(misc, 0, "Dest tile %d", v->dest_tile);
	assert(!v->IsAircraftMoving() || v->IsAircraftFlying());
	assert(v->dest_tile != 0); // REVISE

	assert(IsTileType(v->dest_tile, MP_STATION));

	/* Update final target. */
	v->targetairport = v->current_order.GetDestination();
	assert(Station::IsValidID(v->targetairport));

	switch (v->current_order.GetType()) {
		case OT_GOTO_STATION:
			v->target_state = v->IsHelicopter() ? AM_HELIPAD : AM_TERMINAL;
			break;
		case OT_GOTO_DEPOT:
			v->target_state = AM_HANGAR;
			DEBUG(misc, 0, "Ok, hangar. Remember station index: %d and destination is %d, and depot index", GetStationIndex(v->tile), v->current_order.GetDestination());
			break;
		case OT_NOTHING:
		case OT_LOADING:
		case OT_LEAVESTATION:
		case OT_DUMMY:
		case OT_GOTO_WAYPOINT:
		case OT_CONDITIONAL:
		case OT_IMPLICIT:
		default:
			break;
	}

	/* Fill in next state, depending of target station. */
	if (GetStationIndex(v->tile) == v->targetairport) {
		// Right airport: look for destination tile.
		v->next_state = v->target_state;
	} else {
		// Wrong airport: leave station.
		v->next_state = AM_TAKEOFF;
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
	if (v->cur_state != v->target_state || v->tile != v->next_tile) return false;
	if ((v->x_pos & 0xF) != 8 || (v->y_pos & 0xF) != 8) return false;

	return true;
}

/**
 * Checks if an aircraft is at its next desired state.
 * @param v aircraft
 * @return true if it is at its desired state, false if it needs to move towards another tile.
 */
static bool CheckDestination(Aircraft *v)
{
	/* Stay quiet if it reached the target state. */
	if (v->cur_state != v->target_state || GetStationIndex(v->tile) != v->targetairport) return false;
	if ((v->x_pos & 0xF) != 8 || (v->y_pos & 0xF) != 8) return false;

	return true;
}

/**
 * Checks is a path reservation can be made towards
 * next target of the aircraft.
 * @param v aircraft for the path.
 * @return whether a path can be reserved.
 */
bool TryReservePath(Aircraft *v)
{
	/* First, assert diagonal diadgir.
	 * We shouldn't start paths in stranger tracks. */
	Trackdir trackdir = v->GetVehicleTrackdir();
	assert(IsDiagonalTrackdir(trackdir));

	/* Then, if current state is hangar and we are in a small hangar, make sure it is not reserved. */
	if (v->cur_state == AM_HANGAR) {
		assert(IsHangarTile(v->tile));
		if (IsSmallHangar(v->tile) && HasAirportTrackReserved(v->tile)) return false;
	}

	TileIndex best_tile = INVALID_TILE;
	Trackdir best_trackdir = INVALID_TRACKDIR;
	uint best_cost = UINT_MAX;

	/* Then, locate all possible next target tiles.
	 * Iterate over all of them and find to which one can it reserve
	 * a path with lowest cost possible. */
	static const Trackdir diag_trackdirs[4] = { TRACKDIR_X_NE, TRACKDIR_Y_SE, TRACKDIR_X_SW, TRACKDIR_Y_NW };
	Station *st = Station::GetByTile(v->tile);
	const Airport *airport = &st->airport;

	switch (v->next_state) {
		case AM_HANGAR: {
			DEBUG(misc, 0, "Looking hangar");
			if (st->airport.depot_id == INVALID_DEPOT) break;
			Depot *dep = Depot::Get(st->airport.depot_id);
			for (uint i = dep->depot_tiles.Length(); i--; ) {
				for (uint j = 0; j < 4; j++) {
					if (IsAirportPositionFree(dep->depot_tiles[i], diag_trackdirs[j]) && 100 < best_cost) {
						best_tile = dep->depot_tiles[i];
						best_trackdir = diag_trackdirs[j];
						best_cost = 100;
					}
				}
			}
			DEBUG(misc, 0, "End looking hangar");
			break;
		}

		case AM_HELIPAD:
			// now we will make no distinction between helipads, heliports, builtin heliports
			for (uint i = airport->helipads.Length(); i--; ) {
				for (uint j = 0; j < 4; j++) {
					if (IsAirportPositionFree(airport->helipads[i], diag_trackdirs[j]) && 100 < best_cost) {
						best_tile = airport->helipads[i];
						best_trackdir = diag_trackdirs[j];
						best_cost = 100;
					}
				}
			}
			/* Fall through. */

		case AM_TERMINAL:
			for (uint i = airport->terminals.Length(); i--; ) {
				for (uint j = 0; j < 4; j++) {
					if (IsAirportPositionFree(airport->terminals[i], diag_trackdirs[j]) && 100 < best_cost) {
						best_tile = airport->terminals[i];
						best_trackdir = diag_trackdirs[j];
						best_cost = 100;
					}
				}
			}
			break;

		case AM_HELIPAD_TAKEOFF:
		case AM_HELIPORT_TAKEOFF:
			// fall through
		case AM_TAKEOFF:
			for (uint i = airport->runways.Length(); i--; ) {
				if (!IsLandingTypeTile(airport->runways[i])) continue;
				for (uint j = 0; j < 4; j++) {
					if (IsAirportPositionFree(airport->runways[i], diag_trackdirs[j]) && 100 < best_cost) {
						best_tile = airport->runways[i];
						best_trackdir = diag_trackdirs[j];
						best_cost = 100;
					}
				}
			}
			break;

		case AM_IDLE:
		default:
			break;
	}

	DEBUG(misc, 0, "Ended looking");
	/* If no path found, return false. */
	if (best_tile == INVALID_TILE) return false;

	/* Reserve the path for the best tile. */
	v->next_tile = best_tile;
	v->next_trackdir = best_trackdir;

	/* If a path is found, service, reserve and return true. */
	if (v->cur_state == AM_HANGAR) {
		// after path
		DepotID depot_id = GetDepotIndex(v->tile);
		assert(depot_id != INVALID_DEPOT);
		if (IsBigHangar(v->tile)) {
			//for (Train *u = v; u != NULL; u = u->Next()) u->track &= ~TRACK_BIT_DEPOT;

			/*v->cur_speed = 0;
			v->UpdateAcceleration();
			assert((v->track & TRACK_BIT_DEPOT) == 0);
			if(CheckReverseTrain(v)) ReverseTrainDirection(v);
			InvalidateWindowData(WC_VEHICLE_DEPOT, depot_id); */
			return false;
		}

		VehicleServiceInDepot(v);
		SetWindowClassesDirty(WC_AIRCRAFT_LIST);
		v->PlayLeaveStationSound();

		/* Get track, direction, trackdir... */
		//v->track = TRACK_BIT_X;
		//if (v->direction & 2) v->track = TRACK_BIT_Y;

		v->vehstatus &= ~VS_HIDDEN;
		v->cur_speed = 0;

		v->UpdateViewport(true, true);
		v->UpdatePosition();
		//v->UpdateAcceleration();
		InvalidateWindowData(WC_VEHICLE_DEPOT, depot_id);
	}

	//assign false movement
	if (v->tile == v->next_tile) {
		DEBUG(misc, 0, "We are not really moving");
	} else {
		DEBUG(misc, 0, "YES moving");
	}

	v->tile = v->next_tile;
	v->trackdir = v->next_trackdir;
	v->cur_state = v->next_state;

	return true;
}

static bool MoveAircraft(Aircraft *v)
{
	if (!v->IsAircraftMoving()) return false;

	/* for testing purposes, move aircraft to the middle of the dest_tile.
	 * mark as it reached the destination tile and mark it idle.
	 * mark as stuck for seeing the aircraft position update.
	 * free path.*/
	v->tile = v->dest_tile;
	uint x = TileX(v->tile) * TILE_SIZE + 5;
	uint y = TileY(v->tile) * TILE_SIZE + 3;

	Vehicle *u = v->Next();
	v->x_pos = u->x_pos = x;
	v->y_pos = u->y_pos = y;

	u->z_pos = GetSlopePixelZ(x, y);
	v->z_pos = u->z_pos + 1;

	v->cur_state = AM_IDLE;
	//v->MarkAsStuck();
	return true;
}

TileIndex GetClosestLandingTile(Aircraft *v)
{
	v->targetairport = v->current_order.GetDestination();

	if (v->targetairport == INVALID_STATION) return INVALID_TILE;

	TileIndex landing_tile = INVALID_TILE;
	uint32 best_dist = UINT32_MAX;
	Station *st = Station::Get(v->targetairport);

	// Make helicopters land in helipads if possible
	if (v->IsHelicopter()) {
		//
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

/**
 * Checks whether an aircraft can land on the next order destination.
 * It checks whether it can land (helipads for helicopters, whether there is a landing runway...).
 * It also checks if the destination is too far.
 */
bool IsReachableDest(Aircraft *v)
{
	if (v->targetairport == GetStationIndex(v->tile)) return false;
	if (v->targetairport == INVALID_STATION) return false;

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
		return true;
	} else if (HasBit(v->flags, VAF_CANNOT_LAND_DEST)) {
		/* Aircraft can land now. */
		ClrBit(v->flags, VAF_CANNOT_LAND_DEST);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		DeleteVehicleNews(v->index, STR_NEWS_AIRCRAFT_CANNOT_LAND_NEXT_DEST);
	}

	if (v->acache.cached_max_range_sqr == 0) return false;
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
		return true;
	}

	if (HasBit(v->flags, VAF_DEST_TOO_FAR)) {
		/* Not too far anymore, clear flag and message. */
		ClrBit(v->flags, VAF_DEST_TOO_FAR);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		DeleteVehicleNews(v->index, STR_NEWS_AIRCRAFT_DEST_TOO_FAR);
	}

	return false;
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
		VehicleEnterDepot(v);
	}
}

static bool CheckAircraftLeaveHangar(Aircraft *v)
{
	if (!v->IsInDepot()) return false;

	/* We are leaving a depot, but have to go to the exact same one; re-enter */
	if (v->cur_state == AM_HANGAR && v->current_order.IsType(OT_GOTO_DEPOT) &&
			IsHangarTile(v->tile) && GetStationIndex(v->tile) == v->targetairport) {
		HandleAircraftEnterHangar(v);
		return true;
	}

	if (IsBigHangar(v->tile)) {

	} else {
		v->vehstatus &= ~VS_HIDDEN;
	}

	v->trackdir = DiagDirToDiagTrackdir(GetHangarDirection(v->tile));
	v->cur_speed = 0;
	v->UpdateViewport(true, true);
	SetWindowDirty(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));

	//PlayShipSound(v);
	VehicleServiceInDepot(v);
	InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
	SetWindowClassesDirty(WC_AIRCRAFT_LIST);

	return false;
}


/**
 * Aircraft controller.
 * @param v Aircraft to move.
 * @param mode False during the first call in each tick, true during second call.
 * @return whether the vehicle is still valid.
 */
static void AircraftController(Aircraft *v, bool mode)
{
	if (!v->current_order.IsType(OT_LOADING) && !v->current_order.IsType(OT_LEAVESTATION)) {
		if (v->wait_counter > 200) {
			v->wait_counter = 0;
			DEBUG(misc, 0, "Asking for a new state (index %d)", v->index);
		} else {
			v->wait_counter++;
			return;
		}
	}

	v->HandleBreakdown();
	if (v->ContinueServicing()) return;

	HandleAircraftSmoke(v, mode);

	ProcessOrders(v);
	v->HandleLoading(mode);
	if (v->current_order.IsType(OT_LOADING)) return;

	if (CheckAircraftLeaveHangar(v)) return;

	v->ShowVisualEffect();

	/* Handle aircraft routing. */
	if (!v->IsAircraftMoving()) {
		/* We will only update next partial destination when
		 * we reached the previous one and we are currently not moving. */
		Station *st = Station::GetByTile(v->tile);
		UpdateState(v); // next state should update target

		switch (v->current_order.GetType()) {
			case OT_LEAVESTATION:
				/* A leave station order only needs one tick to get processed,
				 * so we can always skip ahead. */
				v->current_order.Free();
				SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
				break;

			case OT_GOTO_STATION:
				if (CheckDestination(v)) {
					v->last_station_visited = v->current_order.GetDestination();
					AircraftArrivesAt(v, st);
					if (_settings_game.order.serviceathelipad) {
						if (v->subtype == AIR_HELICOPTER &&
								(IsHelipad(v->tile) || IsTerminal(v->tile))) {
							/* an excerpt of ServiceAircraft, without the invisibility stuff */
							v->date_of_last_service = _date;
							v->breakdowns_since_last_service = 0;
							v->reliability = v->GetEngine()->reliability;
							SetWindowDirty(WC_VEHICLE_DETAILS, v->index);
						}
					}
					v->BeginLoading();
				}
				break;

			default: break;
		}

		DEBUG(misc, 0, "Next state is: %d (takeoff %d)", v->next_state, AM_TAKEOFF);

		/* Check if next destination is too far. */
		if (IsReachableDest(v)) return;

		DEBUG(misc, 0, "Going to reserve path");
		if (TryReservePath(v)) {
			if (v->IsStuck()) v->Unstuck();
		} else {
			DEBUG(misc, 0, "going to mark as stuck");
			if (!v->IsStuck()) v->MarkAsStuck();
			DEBUG(misc, 0, "marked as stuck");
		}
	} else {
		if (CheckPartialDestination(v)) {
			DEBUG(misc, 0 ,"Moving and reached partial destination");
			switch (v->cur_state) {
				case AM_TAKEOFF:
					break;
				default:
					v->cur_state = AM_IDLE;
					break;
			}
		} else {
			/* If the aircraft is moving (it has a reserved path),
			* it should move at least once. */
			//int count = UpdateAircraftSpeed(v);
			//if (count < 1) {
				//not reached
			//	return;
			//}

			//GetNewVehiclePosResult gp = GetNewVehiclePos(v);

			/* Handle aircraft movement. */
			if (v->IsAircraftFlying()) {
				switch (v->cur_state) {
					case AM_TAKEOFF:
					case AM_HELIPAD_TAKEOFF:
					case AM_HELIPORT_TAKEOFF:
					case AM_LANDING:
					case AM_HELIPAD_LANDING:
					case AM_HELIPORT_LANDING:
						break;
					case AM_FLYING:
					default: NOT_REACHED();
				}

				TileIndex landing_tile = GetClosestLandingTile(v);
				if (landing_tile != INVALID_TILE) {
					v->tile = landing_tile;
					v->x_pos = TileX(v->tile) * TILE_SIZE + 8;
					v->y_pos = TileY(v->tile) * TILE_SIZE + 8;
					v->cur_state = AM_IDLE;
				} else {
					v->cur_state = AM_IDLE;
					v->dest_tile = v->tile;
				}

			} else {
				NOT_REACHED(); // not still implemented.
			}
		}
	}

	for (Aircraft *u = v; u != NULL; u = u->Next()) {
		//if ((u->vehstatus & VS_HIDDEN) != 0) continue;
		v->x_pos = TileX(v->tile) * TILE_SIZE + 8;
		v->y_pos = TileY(v->tile) * TILE_SIZE + 8;
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

	if (this->IsStuck() && !this->TryUnblock()) return true;

	if (this->ContinueServicing()) return true;

	/* Execute the controller twice, like trains do. */
	AircraftController(this, false);
	if (!this->IsStuck()) AircraftController(this, true);
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
