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
#include "air.h"
#include "aircraft.h"
#include "news_func.h"
#include "newgrf_sound.h"
#include "spritecache.h"
#include "error_func.h"
#include "strings_func.h"
#include "command_func.h"
#include "window_func.h"
#include "timer/timer_game_calendar.h"
#include "timer/timer_game_economy.h"
#include "strings_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "ai/ai.hpp"
#include "game/game.hpp"
#include "effectvehicle_func.h"
#include "zoom_func.h"
#include "disaster_vehicle.h"
#include "newgrf_airporttiles.h"
#include "framerate_type.h"
#include "aircraft_cmd.h"
#include "vehicle_cmd.h"
#include "air_map.h"
#include "pbs_air.h"

#include "pathfinder/yapf/yapf.h"
#include "pathfinder/follow_track.hpp"

#include "table/strings.h"

#include "safeguards.h"

void Aircraft::UpdateDeltaXY()
{
	this->x_offs = -1;
	this->y_offs = -1;
	this->x_extent = 2;
	this->y_extent = 2;

	switch (this->subtype) {
		default: NOT_REACHED();

		case AIR_AIRCRAFT:
		case AIR_HELICOPTER:
			if (this->IsAircraftFlying()) {
				this->x_extent = 24;
				this->y_extent = 24;
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

void Aircraft::MarkDirty()
{
	this->colourmap = PAL_NONE;
	this->UpdateViewport(true, false);
	if (this->subtype == AIR_HELICOPTER) {
		GetRotorImage(this, EIT_ON_MAP, &this->Next()->Next()->sprite_cache.sprite_seq);
	}
}

/**
 * Sets the visibility of an aircraft when it enters or leaves a hangar.
 * @param v Aircraft
 * @param visible Whether it should be visible or not.
 */
void SetVisibility(Aircraft *v, bool visible)
{
	assert(IsHangarTile(v->tile));

	if (visible) {
		v->vehstatus &= ~VS_HIDDEN;
		v->Next()->vehstatus &= ~VS_HIDDEN;
		if (v->IsHelicopter()) v->Next()->Next()->vehstatus &= ~VS_HIDDEN;
	} else {
		v->vehstatus |= VS_HIDDEN;
		v->Next()->vehstatus |= VS_HIDDEN;
		/* Hide and stop rotor for helicopters. */
		if (v->IsHelicopter()) {
			v->Next()->Next()->vehstatus |= VS_HIDDEN;
			v->Next()->Next()->cur_speed = 0;
		}
	}

	v->UpdateViewport(true, true);
	v->UpdatePosition();
}

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
bool IsValidImageIndex<VEH_AIRCRAFT>(uint8_t image_index)
{
	return image_index < lengthof(_aircraft_sprite);
}

void Aircraft::GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const
{
	uint8_t spritenum = this->spritenum;

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
		GetCustomRotorSprite(v, image_type, result);
		if (result->IsValid()) return;
	}

	/* Return standard rotor sprites if there are no custom sprites for this helicopter */
	result->Set(SPR_ROTOR_STOPPED + w->state);
}

static void GetAircraftIcon(EngineID engine, EngineImageType image_type, VehicleSpriteSeq *result)
{
	const Engine *e = Engine::Get(engine);
	uint8_t spritenum = e->u.air.image_index;

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
		rotor_seq.Draw(preferred_x, y - ScaleSpriteTrad(5), PAL_NONE, false);
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

	width  = UnScaleGUI(rect.Width());
	height = UnScaleGUI(rect.Height());
	xoffs  = UnScaleGUI(rect.left);
	yoffs  = UnScaleGUI(rect.top);
}

/**
 * Get the station ID of the airport where the aircraft is in.
 * @return the current aiport id if the aircraft is in, or INVALID_STATION if the aircraft is flying.
 */
StationID Aircraft::GetCurrentAirportID() const
{
	assert(this->IsPrimaryVehicle());
	if (this->state > AS_MOVING) return INVALID_STATION;

	assert(IsAirportTile(this->tile));
	return GetStationIndex(this->tile);
}

/**
 * Returns aircraft's target station if its target
 * is a valid station with an airport.
 * @param v Aircraft to get target airport for
 * @return pointer to target station, nullptr if invalid
 */
Station *GetTargetAirportIfValid(const Aircraft *v)
{
	Station *st = Station::GetIfValid(v->targetairport);
	if (st == nullptr) return nullptr;

	return st->airport.tile == INVALID_TILE ? nullptr : st;
}

TileIndex Aircraft::GetOrderStationLocation(StationID station)
{
	if (station == this->last_station_visited) this->last_station_visited = INVALID_STATION;

	assert(Station::IsValidID(station));
	Station *st = Station::Get(station);

	if (!CanVehicleUseStation(this, st)) {
		this->IncrementRealOrderIndex();
		this->Next()->dest_tile = INVALID_TILE;
		return 0;
	}

	if (!st->airport.aprons.empty()) return st->airport.aprons[0];

	assert(this->IsHelicopter());

	if (!st->airport.helipads.empty()) return st->airport.helipads[0];

	assert(!st->airport.heliports.empty());
	return st->airport.heliports[0];
}

TileIndex Aircraft::GetOrderHangarLocation(DepotID depot)
{
	assert(Depot::IsValidID(depot));
	Depot *dep = Depot::Get(depot);
	assert(dep->veh_type == VEH_AIRCRAFT);
	if (!dep->depot_tiles.empty()) {
		TileIndex tile = dep->depot_tiles[0];
		assert(IsAirportTile(tile) && IsHangar(tile));
		Station *st = Station::GetByTile(tile);
		if (CanVehicleUseStation(this, st)) return tile;
	}

	this->IncrementRealOrderIndex();
	return 0;
}

/**
 * Find the nearest hangar for an aircraft.
 * @param v vehicle looking for a hangar
 * @return the StationID of the closest airport with a hangar; otherwise, INVALID_STATION.
 */
static StationID FindClosestHangar(const Aircraft *v)
{
	uint best = 0;
	StationID index = INVALID_STATION;
	/* revise: they are not clamped */
	TileIndex vtile = TileVirtXY(v->x_pos, v->y_pos);
	uint max_range = v->acache.cached_max_range_sqr;

	/* Determine destinations where it's coming from and where it's heading to */
	const Station *last_dest = nullptr;
	const Station *next_dest = nullptr;
	if (max_range != 0) {
		if (v->current_order.IsType(OT_GOTO_STATION) ||
				(v->current_order.IsType(OT_GOTO_DEPOT) && (v->current_order.GetDepotActionType() & ODATFB_NEAREST_DEPOT) == 0)) {
			last_dest = Station::GetIfValid(v->last_station_visited);
			next_dest = Station::GetIfValid(GetTargetDestination(v->current_order, true));
		} else {
			last_dest = GetTargetAirportIfValid(v);
			next_dest = Station::GetIfValid(v->GetNextStoppingStation().value); // revise getnextstoppingstation could ignore depot orders
		}
	}

	for (const Station *st : Station::Iterate()) {
		if (st->owner != v->owner || !CanVehicleUseStation(v, st) || !st->airport.HasHangar()) continue;

		/* Check if our last and next destinations can be reached from the depot airport. */
		if (max_range != 0) {
			if (last_dest != nullptr &&
					(last_dest->facilities & FACIL_AIRPORT) &&
					DistanceSquare(st->airport.tile, last_dest->airport.tile) > max_range) continue;
			if (next_dest != nullptr &&
					(next_dest->facilities & FACIL_AIRPORT) &&
					DistanceSquare(st->airport.tile, next_dest->airport.tile) > max_range) continue;
		}

		uint distance = DistanceSquare(vtile, st->airport.tile);
		if (distance < best || index == INVALID_STATION) {
			best = distance;
			index = st->index;
		}
	}
	return index;
}

/**
 * Return a tile for placing a newly bought aircraft.
 * @param depot a depot.
 * @return a hangar tile where the new aircraft can be placed, or INVALID_TILE if no hangar available.
 */
TileIndex GetHangarTileForNewAircraft(const Depot *depot)
{
	assert(depot->veh_type == VEH_AIRCRAFT);

	for (const auto &tile : depot->depot_tiles) {
		switch (GetAirportTileType(tile)) {
			case ATT_HANGAR_STANDARD:
				return tile;
			case ATT_HANGAR_EXTENDED:
				if (!HasAirportTileAnyReservation(tile)) return tile;
				break;
			default:
				NOT_REACHED();
		}
	}

	return INVALID_TILE;
}

/**
 * Build an aircraft.
 * @param flags    type of operation.
 * @param tile     tile of the depot where aircraft is built.
 * @param e        the engine to build.
 * @param[out] ret the vehicle that has been built.
 * @return the cost of this operation or an error.
 */
CommandCost CmdBuildAircraft(DoCommandFlag flags, TileIndex tile, const Engine *e, Vehicle **ret)
{
	const AircraftVehicleInfo *avi = &e->u.air;
	const Station *st = Station::GetByTile(tile);

	/* Prevent building aircraft types at places which can't handle them */
	if (!CanVehicleUseStation(e->index, st)) return CMD_ERROR;

	if (!st->airport.HasHangar()) return CMD_ERROR;

	/* Make sure all aircraft ends up in an appropriate hangar. */
	if ((flags & DC_AUTOREPLACE) == 0) {
		tile = GetHangarTileForNewAircraft(st->airport.hangar);
		if (tile == INVALID_TILE) return_cmd_error(STR_ERROR_NO_FREE_HANGAR);
	}

	bool extended_hangar = IsExtendedHangar(tile);

	if (flags & DC_EXEC) {
		Aircraft *v = new Aircraft(); // aircraft
		Aircraft *u = new Aircraft(); // shadow
		*ret = v;

		v->tile = tile;
		v->dest_tile = 0;
		v->next_trackdir = INVALID_TRACKDIR;
		v->direction = u->direction = DiagDirToDir(GetHangarDirection(tile));
		v->trackdir = DiagDirToDiagTrackdir(GetHangarDirection(tile));
		v->wait_counter = 0;

		v->owner = u->owner = _current_company;
		v->SetNext(u);
		v->UpdateNextTile(tile);

		uint x = TileX(tile) * TILE_SIZE + 8;
		uint y = TileY(tile) * TILE_SIZE + 8;

		v->x_pos = u->x_pos = x;
		v->y_pos = u->y_pos = y;

		u->z_pos = GetSlopePixelZ(x, y);
		v->z_pos = u->z_pos + 1;

		v->vehstatus = VS_STOPPED | VS_DEFPAL;
		u->vehstatus = VS_UNCLICKABLE | VS_SHADOW;

		if (!extended_hangar) {
			v->vehstatus |= VS_HIDDEN;
			u->vehstatus |= VS_HIDDEN;
		} else {
			assert(IsValidTrackdir(v->trackdir));
			SetAirportTrackReservation(tile, TrackdirToTrack(v->trackdir));
		}

		v->spritenum = avi->image_index;

		v->cargo_cap = avi->passenger_capacity;
		v->refit_cap = 0;
		u->refit_cap = 0;

		v->cargo_type = e->GetDefaultCargoType();
		assert(IsValidCargoID(v->cargo_type));

		CargoID mail = GetCargoIDByLabel(CT_MAIL);
		if (IsValidCargoID(mail)) {
			u->cargo_type = mail;
			u->cargo_cap = avi->mail_capacity;
		}

		v->name.clear();
		v->last_station_visited = INVALID_STATION;
		v->last_loading_station = INVALID_STATION;

		v->acceleration = avi->acceleration;
		v->engine_type = e->index;
		u->engine_type = e->index;

		v->subtype = (avi->subtype & AIR_CTOL ? AIR_AIRCRAFT : AIR_HELICOPTER);
		v->UpdateDeltaXY();

		u->subtype = AIR_SHADOW;
		u->UpdateDeltaXY();

		v->reliability = e->reliability;
		v->reliability_spd_dec = e->reliability_spd_dec;
		v->max_age = e->GetLifeLengthInDays();

		v->state = AS_HANGAR;

		v->targetairport = GetStationIndex(tile);
		v->SetServiceInterval(Company::Get(_current_company)->settings.vehicle.servint_aircraft);

		v->date_of_last_service = TimerGameEconomy::date;
		v->date_of_last_service_newgrf = TimerGameCalendar::date;
		v->build_year = u->build_year = TimerGameCalendar::year;

		v->sprite_cache.sprite_seq.Set(SPR_IMG_QUERY);
		u->sprite_cache.sprite_seq.Set(SPR_IMG_QUERY);

		v->random_bits = Random();
		u->random_bits = Random();

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
			if (!extended_hangar) w->vehstatus |= VS_HIDDEN;
			w->spritenum = 0xFF;
			w->subtype = AIR_ROTOR;
			w->sprite_cache.sprite_seq.Set(SPR_ROTOR_STOPPED);
			w->random_bits = Random();
			/* Use rotor's air.state to store the rotor animation frame */
			w->state = HRS_ROTOR_STOPPED;
			w->UpdateDeltaXY();

			u->SetNext(w);
			w->UpdatePosition();
		}

		if (extended_hangar) {
			SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
			v->MarkDirty();
		}
	}

	return CommandCost();
}

/** Check whether the aircrafts needs to visit a hangar.
 * @param v Aircraft
 */
static void CheckIfAircraftNeedsService(Aircraft *v)
{
	if (v->IsAircraftFlying() && !v->IsAircraftFreelyFlying()) return;

	if (Company::Get(v->owner)->settings.vehicle.servint_aircraft == 0 || !v->NeedsAutomaticServicing()) return;
	if (v->IsChainInDepot()) {
		VehicleServiceInDepot(v);
		return;
	}

	/* When we're parsing conditional orders and the like
	 * we don't want to consider going to a depot too. */
	if (!v->current_order.IsType(OT_GOTO_DEPOT) && !v->current_order.IsType(OT_GOTO_STATION)) return;

	const Station *st;
	if (v->state <= AS_RUNNING) {
		st = Station::Get(v->GetCurrentAirportID());
	} else {
		st = Station::Get(GetTargetDestination(v->current_order, true));
	}
	assert(st != nullptr);

	if (st->airport.HasHangar() && CanVehicleUseStation(v, st)) {
		v->current_order.MakeGoToDepot(st->airport.hangar->index, ODTFB_SERVICE);
		v->SetDestTile(v->GetOrderHangarLocation(st->airport.hangar->index));
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	} else if (v->current_order.IsType(OT_GOTO_DEPOT)) {
		v->current_order.MakeDummy();
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	} else {
		/* Try going to another hangar. */
		ClosestDepot closest_hangar = v->FindClosestDepot();
		if (closest_hangar.location != INVALID_TILE) {
			v->current_order.MakeGoToDepot(closest_hangar.destination, ODTFB_SERVICE);
			v->SetDestTile(closest_hangar.location);
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		}
	}
}

Money Aircraft::GetRunningCost() const
{
	const Engine *e = this->GetEngine();
	uint cost_factor = GetVehicleProperty(this, PROP_AIRCRAFT_RUNNING_COST_FACTOR, e->u.air.running_cost);
	return GetPrice(PR_RUNNING_AIRCRAFT, cost_factor, e->GetGRF());
}

/** Calendar day handler */
void Aircraft::OnNewCalendarDay()
{
	if (!this->IsNormalAircraft()) return;
	AgeVehicle(this);
}

/** Economy day handler */
void Aircraft::OnNewEconomyDay()
{
	if (!this->IsNormalAircraft()) return;
	EconomyAgeVehicle(this);

	if ((++this->day_counter & 7) == 0) DecreaseVehicleValue(this);

	CheckOrders(this);

	CheckVehicleBreakdown(this);
	CheckIfAircraftNeedsService(this);

	if (this->running_ticks == 0) return;

	CommandCost cost(EXPENSES_AIRCRAFT_RUN, this->GetRunningCost() * this->running_ticks / (CalendarTime::DAYS_IN_YEAR * Ticks::DAY_TICKS));

	this->profit_this_year -= cost.GetCost();
	this->running_ticks = 0;

	SubtractMoneyFromCompanyFract(this->owner, cost);

	SetWindowDirty(WC_VEHICLE_DETAILS, this->index);
	SetWindowClassesDirty(WC_AIRCRAFT_LIST);
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
		GetRotorImage(v, EIT_ON_MAP, &v->Next()->Next()->sprite_cache.sprite_seq);
	}

	Aircraft *u = v->Next();

	int safe_x = Clamp(x, 0, Map::MaxX() * TILE_SIZE);
	int safe_y = Clamp(y - 1, 0, Map::MaxY() * TILE_SIZE);
	u->x_pos = x;
	u->y_pos = y - ((v->z_pos - GetSlopePixelZ(safe_x, safe_y)) >> 3);

	safe_y = Clamp(u->y_pos, 0, Map::MaxY() * TILE_SIZE);
	u->z_pos = GetSlopePixelZ(safe_x, safe_y);
	u->sprite_cache.sprite_seq.CopyWithoutPalette(v->sprite_cache.sprite_seq); // the shadow is never coloured

	u->UpdatePositionAndViewport();

	u = u->Next();
	if (u != nullptr) {
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
	SPEED_LIMIT_APPROACH =    230,  ///< Maximum speed of an aircraft on finals
	SPEED_LIMIT_BROKEN   =    320,  ///< Maximum speed of an aircraft that is broken
	SPEED_LIMIT_HOLD     =    425,  ///< Maximum speed of an aircraft that flies the holding pattern
	SPEED_LIMIT_NONE     = 0xFFFF,  ///< No environmental speed limit. Speed limit is type dependent
};

/**
 * Sets the new speed for an aircraft
 * @param v The vehicle for which the speed should be obtained
 * @return The number of position updates needed within the tick
 */
static int UpdateAircraftSpeed(Aircraft *v)
{
	assert(v->state >= AS_MOVING);

	/* If true, the limit is directly enforced, otherwise the plane is slowed down gradually. */
	bool hard_limit = true;
	/* The maximum speed the vehicle may have. */
	uint speed_limit = SPEED_LIMIT_NONE;

	hard_limit = !HasBit(v->state, ASB_NO_HARD_LIMIT_SPEED);

	if (!hard_limit) {
		if (HasBit(v->state, ASB_FLYING_ON_AIRPORT)) {
			speed_limit = v->IsAircraftOnHold() ? SPEED_LIMIT_HOLD : SPEED_LIMIT_APPROACH;
		} else if (!v->IsAircraftFlying()){
			speed_limit = GetAirTypeInfo(GetAirType(v->GetNextTile()))->max_speed;
		}
	} else if (v->state == AS_RUNNING) {
		assert(IsAirportTile(v->tile));
		speed_limit = GetAirTypeInfo(GetAirType(v->tile))->max_speed;
	}

	/**
	 * 'acceleration' has the unit 3/8 mph/tick. This function is called twice per tick.
	 * So the speed amount we need to accelerate is:
	 *     acceleration * 3 / 16 mph = acceleration * 3 / 16 * 16 / 10 km-ish/h
	 *                               = acceleration * 3 / 10 * 256 * (km-ish/h / 256)
	 *                               ~ acceleration * 77 (km-ish/h / 256)
	 */
	uint spd = v->acceleration * 77;
	uint8_t t;

	/* Adjust speed limits by plane speed factor to prevent taxiing
	 * and take-off speeds being too low. */
	speed_limit *= _settings_game.vehicle.plane_speed;

	/* adjust speed for broken vehicles */
	if (v->vehstatus & VS_AIRCRAFT_BROKEN) {
		if (SPEED_LIMIT_BROKEN < speed_limit) hard_limit = false;
		speed_limit = std::min<uint>(speed_limit, SPEED_LIMIT_BROKEN);
	}

	if (v->vcache.cached_max_speed < speed_limit) {
		if (v->cur_speed < speed_limit) hard_limit = false;
		speed_limit = v->vcache.cached_max_speed;
	}

	v->subspeed = (t = v->subspeed) + (uint8_t)spd;

	/* Aircraft's current speed is used twice so that very fast planes are
	 * forced to slow down rapidly in the short distance needed. The magic
	 * value 16384 was determined to give similar results to the old speed/48
	 * method at slower speeds. This also results in less reduction at slow
	 * speeds to that aircraft do not get to taxi speed straight after
	 * touchdown. */
	if (!hard_limit && v->cur_speed > speed_limit) {
		speed_limit = v->cur_speed - std::max(1, ((v->cur_speed * v->cur_speed) / 16384) / _settings_game.vehicle.plane_speed);
	}

	spd = std::min(v->cur_speed + (spd >> 8) + (v->subspeed < t), speed_limit);

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
	v->progress = (uint8_t)spd;
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
	int safe_x = Clamp(v->x_pos, 0, Map::MaxX() * TILE_SIZE);
	int safe_y = Clamp(v->y_pos, 0, Map::MaxY() * TILE_SIZE);
	return TilePixelHeight(TileVirtXY(safe_x, safe_y));
}

/**
 * Get the 'flight level' bounds, in pixels from 'z_pos' 0 for a particular
 * vehicle for normal flight situation.
 * When the maximum is reached the vehicle should consider descending.
 * When the minimum is reached the vehicle should consider ascending.
 *
 * @param v              The vehicle to get the flight levels for.
 * @param[out] min_level The minimum bounds for flight level.
 * @param[out] max_level The maximum bounds for flight level.
 */
void GetAircraftFlightLevelBounds(const Vehicle *v, int *min_level, int *max_level)
{
	int base_altitude = GetTileHeightBelowAircraft(v);
	if (v->type == VEH_AIRCRAFT && Aircraft::From(v)->IsHelicopter()) {
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
	base_altitude += std::min(20 * (v->vcache.cached_max_speed / 200) - 90, 0);

	if (min_level != nullptr) *min_level = base_altitude + AIRCRAFT_MIN_FLYING_ALTITUDE;
	if (max_level != nullptr) *max_level = base_altitude + AIRCRAFT_MAX_FLYING_ALTITUDE;
}

/**
 * Gets the maximum 'flight level' for the holding pattern of the aircraft,
 * in pixels 'z_pos' 0, depending on terrain below.
 *
 * @param v The aircraft that may or may not need to decrease its altitude.
 * @return Maximal aircraft holding altitude, while in normal flight, in pixels.
 */
int GetAircraftHoldMaxAltitude(const Aircraft *v)
{
	int tile_height = GetTileHeightBelowAircraft(v);

	return tile_height + (v->IsHelicopter() ? HELICOPTER_HOLD_MAX_FLYING_ALTITUDE : PLANE_HOLD_MAX_FLYING_ALTITUDE);
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
template int GetAircraftFlightLevel(Aircraft *v, bool takeoff);

static void HandleHelicopterRotor(Aircraft *v)
{
	Aircraft *u = v->Next()->Next();

	if (u->vehstatus & VS_HIDDEN) return;

	/* if true, helicopter rotors do not rotate. This should only be the case if a helicopter is
	 * loading/unloading at a terminal or stopped */
	if (v->current_order.IsType(OT_LOADING) || (v->vehstatus & VS_STOPPED)) {
		if (u->cur_speed != 0) {
			u->cur_speed++;
			if (u->cur_speed >= 0x80 && u->state == HRS_ROTOR_MOVING_3) {
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
		u->state = HRS_ROTOR_STOPPED;
		GetRotorImage(v, EIT_ON_MAP, &seq);
		if (u->sprite_cache.sprite_seq == seq) return;
	} else if (tick >= spd) {
		u->tick_counter = 0;
		u->state = (AircraftState)((u->state % HRS_ROTOR_NUM_STATES) + 1);
		GetRotorImage(v, EIT_ON_MAP, &seq);
	} else {
		return;
	}

	u->sprite_cache.sprite_seq = seq;

	u->UpdatePositionAndViewport();
}

/**
 * Handle smoke of broken aircraft.
 * @param v Aircraft
 * @param mode Is this the non-first call for this vehicle in this tick?
 */
static void HandleAircraftSmoke(Aircraft *v, bool mode)
{
	static const struct {
		int8_t x;
		int8_t y;
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

// REVISE
/**
 * Mark an aircraft as falling.
 * @param v aircraft
 */
void AircraftStartsFalling(Aircraft *v)
{
	assert(v->IsAircraftFreelyFlying());

	v->state = AS_FLYING_FALLING;
	v->vehstatus |= VS_AIRCRAFT_BROKEN;
	v->acceleration = 0;
	v->dest_tile = 0;
	v->current_order.MakeDummy();
	// revise: next pos? tile? desttile?
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
}

uint Aircraft::Crash(bool flooded)
{
	if (this->IsAircraftFalling() &&
			HasTileWaterClass(this->tile) &&
			IsTileOnWater(this->tile)) {
		flooded = true;
	}

	uint victims = Vehicle::Crash(flooded) + 2; // pilots
	this->crashed_counter = flooded ? 9000 : 0; // max 10000, disappear pretty fast when flooded

	/* Remove the loading indicators (if any) */
	HideFillingPercent(&this->fill_percent_te_id);

	// revise: what happens if a falling aircraft falls in an airport?
	if (!this->IsAircraftFalling() && !(IsRunway(this->tile) && GetReservationAsRunway(this->tile))) {
		/* Lift reserved path except the first tile. Skip reserved runways. */
		LiftAirportPathReservation(this, true);
	}

	this->dest_tile = 0;

	return victims;
}

/**
 * Bring the aircraft in a crashed state, create the explosion animation, and create a news item about the crash.
 * @param v Aircraft that crashed.
 */
void CrashAircraft(Aircraft *v)
{
	CreateEffectVehicleRel(v, 4, 4, 8, EV_EXPLOSION_LARGE);

	uint victims = v->Crash();
	SetDParam(0, victims);

	v->cargo.Truncate();
	v->Next()->cargo.Truncate();
	const Station *st = GetTargetAirportIfValid(v);

	StringID newsitem  = STR_NEWS_AIRCRAFT_CRASH_NO_AIRPORT;
	if (st != nullptr && !v->IsAircraftFalling()) {
		SetDParam(1, st->index);
		newsitem = STR_NEWS_AIRCRAFT_CRASH;
	}

	TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);
	AI::NewEvent(v->owner, new ScriptEventVehicleCrashed(v->index, vt, st == nullptr ? ScriptEventVehicleCrashed::CRASH_AIRCRAFT_NO_AIRPORT : ScriptEventVehicleCrashed::CRASH_PLANE_LANDING, victims));
	Game::NewEvent(new ScriptEventVehicleCrashed(v->index, vt, st == nullptr ? ScriptEventVehicleCrashed::CRASH_AIRCRAFT_NO_AIRPORT : ScriptEventVehicleCrashed::CRASH_PLANE_LANDING, victims));

	NewsType newstype = v->owner == _local_company ? NT_ACCIDENT : NT_ACCIDENT_OTHER;
	AddTileNewsItem(newsitem, newstype, vt, nullptr, st != nullptr ? st->index : INVALID_STATION);

	ModifyStationRatingAround(vt, v->owner, -160, 30);
	if (_settings_client.sound.disaster) SndPlayVehicleFx(SND_12_EXPLOSION, v);
}

/**
 * Decide whether aircraft \a v should crash.
 * @param v Aircraft to test.
 * @return Whether the plane has crashed.
 */
static bool MaybeCrashAirplane(Aircraft *v)
{
	if (_settings_game.vehicle.plane_crashes == 0) return false;

	uint32_t prob = (0x4000 << _settings_game.vehicle.plane_crashes) / 1500;
	uint32_t rand = GB(Random(), 0, 18);
	if (rand > prob) return false;

	/* Crash the airplane. Remove all goods stored at the station. */
	Station *st = Station::Get(v->targetairport);
	for (GoodsEntry &ge : st->goods) {
		ge.rating = 1;
		ge.cargo.Truncate();
	}

	CrashAircraft(v);
	return true;
}

/**
 * Handle crashed aircraft \a v.
 * @param v Crashed aircraft.
 */
static bool HandleCrashedAircraft(Aircraft *v)
{
	v->crashed_counter += 3;

	if (v->crashed_counter < 650) {
		uint32_t r;
		if (Chance16R(1, 32, r)) {
			static const DirDiff delta[] = {
				DIRDIFF_45LEFT, DIRDIFF_SAME, DIRDIFF_SAME, DIRDIFF_45RIGHT
			};

			v->direction = v->Next()->direction = ChangeDir(v->direction, delta[GB(r, 16, 2)]);
			SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
			r = Random();
			CreateEffectVehicleRel(v,
								   GB(r, 0, 4) - 4,
								   GB(r, 4, 4) - 4,
								   GB(r, 8, 4),
								   EV_EXPLOSION_SMALL);
		}
	} else if (v->crashed_counter >= 10000) {
		if ((v->vehstatus & VS_HIDDEN) != 0 || v->IsAircraftFalling()) {
			/* Deleting a vehicle in a hangar or crashed outside the airport. */
			delete v;
			return false;
		}

		/*  remove rubble of crashed airplane */
		if (HasAirportTrackReserved(v->tile)) {
			assert(!v->IsAircraftFlying());
			assert(HasAirportTrackReserved(v->tile, TrackdirToTrack(v->trackdir)));
			RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
		} else {
			assert(IsAirportTile(v->tile));
			assert(IsRunway(v->tile));
			assert(GetReservationAsRunway(v->tile));
			assert(IsDiagonalTrackdir(v->trackdir));
			DiagDirection diagdir = TrackdirToExitdir(v->trackdir);
			TileIndex start_tile = GetRunwayExtreme(v->tile, ReverseDiagDir(diagdir));
			SetRunwayReservation(start_tile, false);
		}

		delete v;
		return false;
	}

	return true;
}

/**
 * Aircraft \a v cannot find an airport to go to and it will fall until it crashes.
 * @param v Aircraft falling to the ground.
 */
static void HandleAircraftFalling(Aircraft *v)
{
	assert(v->IsAircraftFalling());
	int z = GetSlopePixelZ(Clamp(v->x_pos, 0, Map::MaxX() * TILE_SIZE), Clamp(v->y_pos, 0, Map::MaxY() * TILE_SIZE));
	GetNewVehiclePosResult gp = GetNewVehiclePos(v);

	/* MoveAircraft() is called twice, but handling out of fuel only once. */
	uint count = UpdateAircraftSpeed(v) + UpdateAircraftSpeed(v);
	v->x_pos += count * (gp.x - v->x_pos);
	v->y_pos += count * (gp.y - v->y_pos);

	if (count > 0) v->z_pos -= 1;

	if (v->z_pos == z) {
		v->z_pos++;
		CreateEffectVehicleRel(v, 4, 4, 8, EV_EXPLOSION_LARGE);
		v->vehstatus &= ~VS_AIRCRAFT_BROKEN;
		CrashAircraft(v);
	} else {
		HandleAircraftSmoke(v, false);
		SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
	}
}

/** Structure for aircraft sub-coordinate data for moving into a new tile via a Diagdir onto a Track. */
struct AircraftSubcoordData {
	uint8_t x_subcoord; ///< New X sub-coordinate on the new tile
	uint8_t y_subcoord; ///< New Y sub-coordinate on the new tile
	Direction dir;      ///< New Direction to move in on the new track
};

/** Aircraft sub-coordinate data for moving into a new tile via a Diagdir onto a Track.
 * Array indexes are Diagdir, Track.
 * There will always be three possible tracks going into an adjacent tile via a Diagdir,
 * so each Diagdir sub-array will have three valid and three invalid structures per Track.
 */
static const AircraftSubcoordData _aircraft_subcoord[DIAGDIR_END][TRACK_END] = {
	// DIAGDIR_NE
	{
		{15,  8, DIR_NE},      // TRACK_X
		{ 0,  0, INVALID_DIR}, // TRACK_Y
		{ 0,  0, INVALID_DIR}, // TRACK_UPPER
		{15,  8, DIR_E},       // TRACK_LOWER
		{15,  7, DIR_N},       // TRACK_LEFT
		{ 0,  0, INVALID_DIR}, // TRACK_RIGHT
	},
	// DIAGDIR_SE
	{
		{ 0,  0, INVALID_DIR}, // TRACK_X
		{ 8,  0, DIR_SE},      // TRACK_Y
		{ 7,  0, DIR_E},       // TRACK_UPPER
		{ 0,  0, INVALID_DIR}, // TRACK_LOWER
		{ 8,  0, DIR_S},       // TRACK_LEFT
		{ 0,  0, INVALID_DIR}, // TRACK_RIGHT
	},
	// DIAGDIR_SW
	{
		{ 0,  8, DIR_SW},      // TRACK_X
		{ 0,  0, INVALID_DIR}, // TRACK_Y
		{ 0,  7, DIR_W},       // TRACK_UPPER
		{ 0,  0, INVALID_DIR}, // TRACK_LOWER
		{ 0,  0, INVALID_DIR}, // TRACK_LEFT
		{ 0,  8, DIR_S},       // TRACK_RIGHT
	},
	// DIAGDIR_NW
	{
		{ 0,  0, INVALID_DIR}, // TRACK_X
		{ 8, 15, DIR_NW},      // TRACK_Y
		{ 0,  0, INVALID_DIR}, // TRACK_UPPER
		{ 8, 15, DIR_W},       // TRACK_LOWER
		{ 0,  0, INVALID_DIR}, // TRACK_LEFT
		{ 7, 15, DIR_N},       // TRACK_RIGHT
	}
};

/**
 * Check whether the aircraft needs to rotate its current trackdir.
 * @param v Aircraft
 * @return whether the aircraft needs to rotate its current trackdir.
 */
bool DoesAircraftNeedRotation(Aircraft *v)
{
	assert(v->next_trackdir == INVALID_TRACKDIR || IsValidTrackdir(v->next_trackdir));
	return v->next_trackdir != INVALID_TRACKDIR;
}

const uint16_t AIRCRAFT_ROTATION_STEP_TICKS = 30;
const uint16_t AIRCRAFT_WAIT_FREE_PATH_TICKS = 10;
const uint16_t AIRCRAFT_WAIT_LEAVE_HANGAR_TICKS = 200;
const uint16_t AIRCRAFT_CANT_LEAVE_RUNWAY = 200;

/**
 * Slightly rotate an aircraft towards its desired trackdir.
 * @param v Aircraft
 */
void DoRotationStep(Aircraft *v)
{
	assert(DoesAircraftNeedRotation(v));
	if (v->trackdir == v->next_trackdir) {
		v->next_trackdir = INVALID_TRACKDIR;
		v->ClearWaitTime();
		return;
	} else {
		if (v->cur_speed != 0) {
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
			v->cur_speed = 0;
		}

		Direction desired_direction = TrackdirToDir(v->next_trackdir);
		assert(IsValidDirection(desired_direction));
		assert(v->direction != desired_direction);
		DirDiff difference = DirDifference(v->direction, desired_direction);
		assert(difference != DIRDIFF_SAME);
		difference = difference <= DIRDIFF_REVERSE ? DIRDIFF_45LEFT : DIRDIFF_45RIGHT;
		v->direction = v->Next()->direction = ChangeDir(v->direction, difference);

		if (v->direction == desired_direction) {
			v->trackdir = v->next_trackdir;

			if (IsDiagonalTrackdir(v->trackdir)) {
				/* Amend position when rotating in the middle of the tile. */
				if (DiagDirToAxis(DirToDiagDir(v->direction)) == AXIS_X) {
					v->y_pos = (v->y_pos & ~0xF) | 8;
				} else {
					v->x_pos = (v->x_pos & ~0xF) | 8;
				}
			} else {
				/* Amend position when rotating at the edge of a tile. */
				const AircraftSubcoordData &b = _aircraft_subcoord[TrackdirToEntrydir(v->trackdir)][TrackdirToTrack(v->trackdir)];
				v->x_pos = (v->x_pos & ~0xF) | b.x_subcoord;
				v->y_pos = (v->y_pos & ~0xF) | b.y_subcoord;
			}
		}

		assert(!v->IsWaiting());
		v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
	}

	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
}

/**
 * Check whether a runway can be reserved.
 * @param tile A start or end tile of the runway.
 * @param skip_first_tile whether the first tile is already occupied and should be skipped.
 * @return true if none of the tiles of the runway has a runway or track reservation.
 */
bool CanRunwayBeReserved(TileIndex tile, bool skip_first_tile = false)
{
	if (tile == 0) return false;

	assert(IsTileType(tile, MP_STATION));
	assert(IsAirportTile(tile));
	assert(IsRunwayExtreme(tile));
	DiagDirection dir = GetRunwayExtremeDirection(tile);
	if (IsRunwayEnd(tile)) dir = ReverseDiagDir(dir);
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	TileIndex t = tile;
	if (skip_first_tile) t = TileAdd(t, diff);

	for (;; t = TileAdd(t, diff)) {
		assert(IsAirportTile(t));
		assert(IsRunway(t));
		if (HasAirportTileAnyReservation(t)) return false;
		if (t != tile && IsRunwayExtreme(t)) return true;
	}

	NOT_REACHED();
}

/**
 * Checks if an aircraft is at its next position.
 * @param v aircraft
 * @return whether it is at its next position.
 */
static bool IsAircraftOnNextPosition(const Aircraft *v)
{
	return v->x_pos == v->next_pos.x && v->y_pos == v->next_pos.y;
}

/**
 * Updates state for an aircraft.
 * @param v aircraft.
 */
void UpdateAircraftState(Aircraft *v)
{
	// revise: check conditions
	// revise: is IsAircraftOnNextPosition always true here?
	if (v->state == AS_RUNNING && !IsAircraftOnNextPosition(v)) return;
	if (v->IsAircraftFlying() && !v->IsAircraftFreelyFlying()) return;

	StationID cur_station = v->GetCurrentAirportID();
	StationID cur_dest_station = v->targetairport = GetTargetDestination(v->current_order, true);
	AircraftState next_state = AS_IDLE;
	TileIndex dest_tile = 0;

	switch (v->current_order.GetType()) {
		case OT_GOTO_STATION:
			next_state = AS_APRON;
			dest_tile = v->GetOrderStationLocation(v->current_order.GetDestination());
			break;

		case OT_GOTO_DEPOT:
			next_state = AS_HANGAR;
			dest_tile = v->GetOrderHangarLocation(v->current_order.GetDestination());
			break;

		case OT_NOTHING:
			if (cur_station == INVALID_STATION) {
				/* If flying, find closest airport and go there. */
				ClosestDepot closestHangar = v->FindClosestDepot();
				cur_dest_station = closestHangar.st_destination;
				dest_tile = v->GetOrderHangarLocation(closestHangar.destination);
			} else {
				/* If aircraft is in an airport, go to its hangar or aprons. */
				Station *st = Station::Get(cur_station);
				if (st->airport.HasHangar()) {
					next_state = AS_HANGAR;
					dest_tile = v->GetOrderHangarLocation(st->airport.hangar->index);
				} else {
					next_state = AS_APRON;
					dest_tile = v->GetOrderStationLocation(st->index);
				}
			}
			break;

		default:
			Debug(misc, 0, "Unhandled order type");
			break;
	}

	v->dest_tile = dest_tile;

	if (cur_station == INVALID_STATION) {
		if (cur_dest_station == INVALID_STATION && v->IsAircraftFreelyFlying()) AircraftStartsFalling(v);
		return;
	}

	if (cur_station != cur_dest_station) {
		/* Aircraft has to leave current airport. */
		next_state = AS_START_TAKEOFF;
	}

	if (v->state == next_state) return;

	switch (next_state) {
		case AS_START_TAKEOFF:
			if (v->IsHelicopter()) {
				if (IsApron(v->tile)) {
					v->state = AS_START_TAKEOFF;
				}
			} else {
				if (IsRunwayStart(v->tile)) {
					v->state = AS_START_TAKEOFF;
					v->UpdateNextTile(v->tile);
				} else {
					v->UpdateNextTile(INVALID_TILE);
				}
			}
			break;
		case AS_APRON:
			if (v->state == AS_HANGAR) break;
			if (!IsApron(v->tile) || (!v->IsHelicopter() && !IsPlaneApron(v->tile))) {
				// Current tile is not a valid terminal.
				v->state = AS_IDLE;
				v->UpdateNextTile(INVALID_TILE);
			}
			break;
		case AS_HANGAR:
			if (!IsHangarTile(v->tile)) {
				if (IsHeliportTile(v->tile)) {
					/* Take off, as it is not possible to reach the hangar. */
					v->state = AS_START_TAKEOFF;
					v->Next()->Next()->cur_speed = 0;
					break;
				}
			} else if (v->current_order.IsType(OT_GOTO_DEPOT) && v->current_order.GetDestination() == GetDepotIndex(v->tile)) {
				v->UpdateNextTile(v->tile);
			}
			break;
		default:
			break;
	}
}

/**
 * Handle Aircraft specific tasks when an Aircraft enters a hangar.
 * @param v Vehicle that enters the hangar.
 */
void AircraftEntersHangar(Aircraft *v)
{
	v->subspeed = 0;
	v->progress = 0;
	v->cur_speed = 0;
	v->state = AS_HANGAR;

	if (IsExtendedDepot(v->tile)) {
		v->UpdateViewport(true, true);
		SetWindowClassesDirty(WC_AIRCRAFT_LIST);
		SetWindowDirty(WC_VEHICLE_VIEW, v->index);

		InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
		assert(!v->IsServicing());
		v->StartService();
	} else {
		assert(IsValidTrackdir(v->trackdir));
		assert(TrackdirToTrack(v->trackdir) == DiagDirToDiagTrack(GetHangarDirection(v->tile)));
		if ((v->vehstatus & VS_HIDDEN) == 0) {
			v->direction = v->Next()->direction = DiagDirToDir(GetHangarDirection(v->tile));
			RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));

			/* Hide vehicle. */
			SetVisibility(v, false);
			SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
		}
		VehicleEnterDepot(v);
	}
}

/**
 * Aircraft is about to leave the hangar.
 * @param v Aircraft leaving.
 */
void AircraftLeavesHangar(Aircraft *v)
{
	assert(IsHangarTile(v->tile));
	v->cur_speed = 0;
	v->subspeed = 0;
	v->progress = 0;

	Aircraft *u = v->Next();
	u->direction = v->direction;
	u->trackdir = v->trackdir;

	/* Rotor blades */
	u = u->Next();
	if (u != nullptr) u->cur_speed = 80;

	VehicleServiceInDepot(v);
	v->LeaveUnbunchingDepot();
	if (!IsExtendedHangar(v->tile)) SetVisibility(v, true);

	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
	InvalidateWindowData(WC_VEHICLE_DEPOT, GetDepotIndex(v->tile));
	SetWindowClassesDirty(WC_AIRCRAFT_LIST);
}

/**
 * Aircraft arrives at a terminal. If it is the first aircraft, throw a party.
 * Start loading cargo.
 * @param v Aircraft that arrived.
 */
static void AircraftEntersTerminal(Aircraft *v)
{
	assert(HasAirportTrackReserved(v->tile));
	assert(CountBits(GetReservedAirportTracks(v->tile)) == 1);
	assert(IsDiagonalTrackdir(v->trackdir));
	assert(Station::IsValidID(v->targetairport));

	Station *st = Station::Get(v->targetairport);
	v->last_station_visited = st->index;

	v->state = AS_APRON + (AircraftState)GetApronType(v->tile);

	/* Check if station was ever visited before */
	if (!(st->had_vehicle_of_type & HVOT_AIRCRAFT)) {
		st->had_vehicle_of_type |= HVOT_AIRCRAFT;
		SetDParam(0, st->index);
		/* show newsitem of celebrating citizens */
		AddVehicleNewsItem(
			STR_NEWS_FIRST_AIRCRAFT_ARRIVAL,
			(v->owner == _local_company) ? NT_ARRIVAL_COMPANY : NT_ARRIVAL_OTHER,
			v->index,
			st->index
		);
		AI::NewEvent(v->owner, new ScriptEventStationFirstVehicle(st->index, v->index));
		Game::NewEvent(new ScriptEventStationFirstVehicle(st->index, v->index));
	}

	if (_settings_game.order.serviceathelipad && v->IsHelicopter() && IsHelipad(v->tile)) {
		/* an excerpt of ServiceAircraft, without the invisibility stuff */
		v->date_of_last_service = TimerGameEconomy::date;
		v->breakdowns_since_last_service = 0;
		v->reliability = v->GetEngine()->reliability;
		SetWindowDirty(WC_VEHICLE_DETAILS, v->index);
	}

	v->BeginLoading();
}

void HandleAircraftLanding(Aircraft *v);

/**
 * Raises or lowers the helicopter.
 * @param v The helicopter.
 * @return Whether the helicopter is taking off or landing.
 * @pre v->IsHelicopter()
 */
bool RaiseLowerHelicopter(Aircraft *v)
{
	assert(v->IsHelicopter());

	switch (v->state) {
		case AS_FLYING_HELICOPTER_TAKEOFF:
		case AS_START_TAKEOFF: {
			Aircraft *u = v->Next()->Next();

			/* Make sure the rotors don't rotate too fast */
			if (u->cur_speed > 32) {
				v->cur_speed = 0;
				if (--u->cur_speed == 32) {
					if (!PlayVehicleSound(v, VSE_START)) {
						SoundID sfx = AircraftVehInfo(v->engine_type)->sfx;
						/* For compatibility with old NewGRF we ignore the sfx property, unless a NewGRF-defined sound is used.
						 * The baseset has only one helicopter sound, so this only limits using plane or cow sounds. */
						if (sfx < ORIGINAL_SAMPLE_COUNT) sfx = SND_18_TAKEOFF_HELICOPTER;
						SndPlayVehicleFx(sfx, v);
					}
					v->state = AS_FLYING_HELICOPTER_TAKEOFF;
				}
			} else {
				u->cur_speed = 32;
				int count = UpdateAircraftSpeed(v);
				if (count > 0) {
					int z_dest;
					GetAircraftFlightLevelBounds(v, &z_dest, nullptr);

					/* Reached altitude? */
					if (v->z_pos + count >= z_dest) {
						if (v->cur_speed != 0) SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
						v->cur_speed = 0;
						if (v->NeedsAutomaticServicing()) {
							Backup<CompanyID> cur_company(_current_company, v->owner);
							Command<CMD_SEND_VEHICLE_TO_DEPOT>::Do(DC_EXEC, v->index, DepotCommand::Service | DepotCommand::LocateHangar, {});
							cur_company.Restore();
						}
						RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
						v->state = AS_FLYING;
						AircraftUpdateNextPos(v);
					}
					v->z_pos = std::min(v->z_pos + count, z_dest);
				}
			}
			SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
			return true;
		}

		case AS_FLYING_HELICOPTER_LANDING: {
			/* Find altitude of landing position. */
			int z = GetTileMaxPixelZ(v->tile) + 1;
			z += GetLandingHeight(v->GetNextTile());

			if (z == v->z_pos) {
				Vehicle *u = v->Next()->Next();

				/*  Increase speed of rotors. When speed is 80, we've landed. */
				if (u->cur_speed >= 80) {
					if (v->cur_speed != 0) SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
					v->cur_speed = 0;
					SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
					v->state = AS_LANDED;
					HandleAircraftLanding(v);
					return true;
				}
				u->cur_speed += 4;
			} else {
				int count = UpdateAircraftSpeed(v);
				if (count > 0) {
					SetAircraftPosition(v, v->x_pos, v->y_pos, std::max(v->z_pos - count, z));
				}
			}
			return true;
		}

		default:
			return false;
	}
}

static void PlayAircraftTakeoffSound(const Vehicle *v)
{
	if (PlayVehicleSound(v, VSE_START)) return;
	SndPlayVehicleFx(AircraftVehInfo(v->engine_type)->sfx, v);
}

/**
 * Aircraft is at a position where it can start taking off.
 * Check whether it should start taking off, change its mind or wait till the runway is free.
 * @param v Vehicle ready to take off.
 * @return whether it should stop moving this tick.
 */
bool HandleAircraftReadyToTakeoff(Aircraft *v)
{
	assert(v->state == AS_START_TAKEOFF);

	if (v->IsHelicopter()) {
		assert(IsAirportTile(v->tile));
		assert(IsApron(v->tile));

		if (v->targetairport == v->GetCurrentAirportID()) {
			/* Trying to go to the same airport. */
			// revise
			NOT_REACHED();
			v->state = AS_IDLE;
			return true;
		}

		RaiseLowerHelicopter(v);
		return true;
	}

	assert(IsRunwayStart(v->tile));

	if (v->targetairport == v->GetCurrentAirportID()) {
		/* Trying to go to the same airport. */
		v->state = AS_IDLE;
		return true;
	}

	/* Aircraft tries to take off using a runway. */
	if (!CanRunwayBeReserved(v->tile, true)) return true;

	RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
	SetRunwayReservation(v->tile, true);

	v->state = AS_TAKEOFF_BEFORE_FLYING;
	v->next_trackdir = v->Next()->next_trackdir = DiagDirToDiagTrackdir(GetRunwayExtremeDirection(v->tile));
	v->UpdateNextTile(GetRunwayExtreme(v->tile, GetRunwayExtremeDirection(v->tile)));

	if (v->trackdir != v->next_trackdir) {
		/* If plane needs to rotate, rotate first and then play the take off sound. */
		v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
	} else {
		/* Plane doesn't need to rotate. Play the take off sound right now. */
		PlayAircraftTakeoffSound(v);
	}

	return false;
}

/**
 * Aircraft is taking off accelerating on runway, starting its flight or leaving the airport.
 * @param v Vehicle that is taking off.
 */
void HandleAircraftTakingoff(Aircraft *v)
{
	switch (v->state) {
		case AS_TAKEOFF_BEFORE_FLYING:
			assert(!v->IsHelicopter());
			v->state = AS_FLYING_TAKEOFF;
			v->UpdateNextTile(v->GetNextTile());
			break;

		case AS_FLYING_TAKEOFF: {
			assert(!v->IsHelicopter());
			/* Next tile contains the runway end, so it can be unreserved. */
			TileIndex old_runway_tile = v->GetNextTile();
			SetRunwayReservation(old_runway_tile, false);
			v->state = AS_FLYING_LEAVING_AIRPORT;
			v->UpdateNextTile(old_runway_tile);
			break;
		}

		case AS_FLYING_HELICOPTER_TAKEOFF:
			RaiseLowerHelicopter(v);
			break;

		case AS_FLYING_LEAVING_AIRPORT: {
			v->state = AS_FLYING;
			v->UpdateNextTile(FindClosestLandingTile(v));
			break;
		}

		default:
			Debug(misc, 0, "Shouldnt be reached, state {}", v->state);
			break;
	}
}

TileIndex FindClosestFreeLandingTile(Aircraft *v);

/**
 * Handle Aircraft flying outside any airport or keeping a holding pattern
 * on its target airport.
 * @param v Vehicle that is flying towards its next target station, if any.
 */
void HandleAircraftFlying(Aircraft *v)
{
	switch (v->state) {
		case AS_ON_HOLD_WAITING: {
			bool can_land = !(Station::Get(v->targetairport))->airport.IsClosed();
			if (v->IsHelicopter()) {
				// revise
				//if (!IsAirportTile(v->tile) || !IsApron(v->tile)) return;
				assert(v->IsAircraftFlying());
				TileIndex landing_tile;
				Trackdir trackdir;
				if (can_land) {
					landing_tile = FindClosestFreeLandingTile(v);
					trackdir = GetFreeAirportTrackdir(landing_tile, DiagDirToDiagTrackdir(DirToDiagDir(v->direction)));
					can_land = trackdir != INVALID_TRACKDIR;
				}

				if (can_land) {
					assert(IsValidTrackdir(trackdir));
					SetAirportTrackReservation(landing_tile, TrackdirToTrack(trackdir));
					v->state = AS_ON_HOLD_APPROACHING;
					v->tile = landing_tile;
					v->UpdateNextTile(landing_tile);
					v->Next()->next_trackdir = trackdir;
				} else {
					v->UpdateNextTile(v->GetNextTile());
				}
			} else {
				assert(IsValidTrackdir(v->trackdir));
				TileIndex landing_tile = FindClosestLandingTile(v);
				if (can_land && CanRunwayBeReserved(landing_tile)) {
					assert(IsRunwayStart(landing_tile));
					v->trackdir = DiagDirToDiagTrackdir(GetRunwayExtremeDirection(landing_tile));
					SetRunwayReservation(landing_tile, true);
					v->state = AS_ON_HOLD_APPROACHING;
					v->UpdateNextTile(landing_tile);
				} else {
					v->UpdateNextTile(v->GetNextTile());
				}
			}
			break;
		}

		case AS_ON_HOLD_APPROACHING:
			if (v->IsHelicopter()) {
				v->state = AS_DESCENDING;
				assert(HasAirportTrackReserved(v->GetNextTile()));
				assert(HasAirportTrackReserved(v->GetNextTile(), TrackdirToTrack(v->Next()->next_trackdir)));
				v->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
				if (v->trackdir != v->Next()->next_trackdir) {
					v->next_trackdir = v->Next()->next_trackdir;
				}
			} else {
				if (v->next_pos.pos == AP_PLANE_HOLD_3) {
					v->state = AS_DESCENDING;
				}
			}
			v->UpdateNextTile(v->GetNextTile());
			break;

		case AS_FLYING:
			v->state = AS_ON_HOLD_WAITING;
			v->UpdateNextTile(v->GetNextTile());
			break;

		case AS_FLYING_NO_DEST:
			break;

		default:
			Debug(misc, 0, "Shouldnt be reached, state {}", v->state);
			break;
	}
}

/**
 * Handle Aircraft landing on an airport.
 * @param v Landing aircraft.
 */
void HandleAircraftLanding(Aircraft *v)
{
	switch (v->state) {
		case AS_LANDED: {
			if (v->IsHelicopter()) {
				assert(IsAirportTile(v->tile));
				assert(IsApron(v->tile));
				v->state = (AircraftState)((uint8_t)GetApronType(v->tile) + (uint8_t)AS_APRON);
				break;
			}

			assert(IsAirportTile(v->tile));
			assert(IsRunwayExtreme(v->tile));
			assert(IsRunwayEnd(v->tile));

			/* Free platform and reserve a track and set it to next trackdir. */
			Trackdir trackdir = DiagDirToDiagTrackdir(GetRunwayExtremeDirection(v->tile));
			Trackdir next_trackdir = GetFreeAirportTrackdir(v->tile, trackdir);
			if (!IsValidTrackdir(next_trackdir)) {
				v->SetWaitTime(AIRCRAFT_CANT_LEAVE_RUNWAY);
				break;
			}

			v->trackdir = trackdir;
			SetRunwayReservation(v->tile, false);
			if (next_trackdir != v->trackdir) {
				v->next_trackdir = next_trackdir;
				v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
			}
			SetAirportTrackReservation(v->tile, TrackdirToTrack(next_trackdir));
			v->state = AS_IDLE;
			v->cur_speed = 0;
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
			v->UpdateNextTile(INVALID_TILE);
			break;
		}

		case AS_FLYING_LANDING:
			Debug(misc, 0, "Aircraft reached landed runway end still flying. Error of controller. Crashing aircraft unitnumber {} on air", v->unitnumber);
			v->state = AS_LANDED;
			CrashAircraft(v);
			return;

		case AS_DESCENDING:
			if (v->IsHelicopter()) {
				v->state = AS_FLYING_HELICOPTER_LANDING;
			} else {
				v->state = AS_FLYING_LANDING;
				TileIndex tile = GetRunwayExtreme(v->tile, GetRunwayExtremeDirection(v->tile));
				v->UpdateNextTile(tile);
			}
			break;

		default:
			Debug(misc, 0, "Shouldnt be reached, state {}", v->state);
			break;
	}
}

/**
 * Plane touched down at the landing strip.
 * @param v Aircraft that landed.
 */
static void HandlePlaneLandsOnRunway(Aircraft *v)
{
	assert(!v->IsHelicopter());
	assert(v->state == AS_FLYING_LANDING);
	Station *st = Station::Get(v->targetairport);

	TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);

	v->state = AS_LANDED;
	v->UpdateNextTile(v->GetNextTile());

	/* Check if the aircraft needs to be replaced or renewed and send it to a hangar if needed. */
	if (v->NeedsAutomaticServicing()) {
		Backup<CompanyID> cur_company(_current_company, v->owner);
		Command<CMD_SEND_VEHICLE_TO_DEPOT>::Do(DC_EXEC, v->index, DepotCommand::Service, {});
		cur_company.Restore();
	}

	v->UpdateDeltaXY();

	AirportTileAnimationTrigger(st, vt, AAT_STATION_AIRPLANE_LAND);

	if (!PlayVehicleSound(v, VSE_TOUCHDOWN)) {
		SndPlayVehicleFx(SND_17_SKID_PLANE, v);
	}
}

/**
 * Given the current state of an aircraft, get which is the next
 * state to reach its target.
 * @param a Aircraft
 * @return the next state \a a should try to reach.
 */
AircraftState GetNextAircraftState(const Aircraft &a)
{
	assert(!a.IsAircraftFlying());

	if (GetStationIndex(a.tile) != a.targetairport) {
		/* Aircraft has to leave current airport. */
		return AS_START_TAKEOFF;
	}

	if (a.state != AS_RUNNING && IsRunwayEnd(a.tile)) {
		Airport *airport = &Station::GetByTile(a.tile)->airport;
		bool free_terminal = false;
		for (TileIndex tile : airport->aprons) {
			if (HasAirportTrackReserved(tile)) continue;
			free_terminal = true;
			break;
		}

		if (!free_terminal) {
			return airport->HasHangar() ? AS_HANGAR : AS_IDLE;
		}
	}

	switch (a.current_order.GetType()) {
		case OT_GOTO_STATION:
			return a.IsHelicopter() ? AS_HELIPAD : AS_APRON;

		case OT_GOTO_DEPOT:
			return AS_HANGAR;

		case OT_NOTHING:
			/* If aircraft is in an airport, go to its hangar or aprons. */
			if (Station::Get(a.targetairport)->airport.HasHangar()) {
				return AS_HANGAR;
			} else {
				return a.IsHelicopter() ? AS_HELIPAD : AS_APRON;
			}
			return AS_IDLE;

		default:
			return AS_IDLE;
	}
}

/**
 * Aircraft reached a position where it may change to another state.
 * Decide what to do.
 * @param v aircraft.
 * @return whether it should stop moving this tick.
 */
bool HandleAircraftState(Aircraft *v)
{
	if (!IsAircraftOnNextPosition(v)) return false;

	if (!v->IsAircraftFlying()) {
		switch (v->current_order.GetType()) {
			case OT_LEAVESTATION:
				/* A leave station order only needs one tick to get processed,
				* so we can always skip ahead. */
				v->current_order.Free();
				SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
				ProcessOrders(v);
				v->state = AS_IDLE;
				v->UpdateNextTile(INVALID_TILE);
				UpdateAircraftState(v);
				return true;

			case OT_GOTO_STATION:
				if (IsAirportTile(v->tile) && IsApron(v->tile)) {
					if (v->targetairport == GetStationIndex(v->tile)) {
						AircraftEntersTerminal(v);
					} else {
						v->state = GetNextAircraftState(*v);
						v->UpdateNextTile(INVALID_TILE);
					}
					return true;
				}
				break;

			case OT_NOTHING:
			case OT_GOTO_DEPOT:
				if (IsHangarTile(v->tile) && v->state != AS_HANGAR) {
					AircraftEntersHangar(v);
					v->UpdateNextTile(INVALID_TILE);
					return true;
				} else if (v->state == AS_RUNNING) {
					v->state = AS_IDLE;
				}
				break;

			default:
				break;
		}
	}

	UpdateAircraftState(v);

	switch (v->state) {
		case AS_APRON:
		case AS_HELIPAD:
		case AS_HELIPORT:
		case AS_BUILTIN_HELIPORT:
			/* Helicopter takeoff. */
			AircraftEntersTerminal(v);
			return true;

		case AS_START_TAKEOFF:
			return HandleAircraftReadyToTakeoff(v);

		case AS_TAKEOFF_BEFORE_FLYING:
		case AS_FLYING_TAKEOFF:
		case AS_FLYING_LEAVING_AIRPORT:
		case AS_FLYING_HELICOPTER_TAKEOFF:
			HandleAircraftTakingoff(v);
			return false;

		case AS_FLYING_NO_DEST:
		case AS_FLYING:
		case AS_ON_HOLD_WAITING:
		case AS_ON_HOLD_APPROACHING:
			HandleAircraftFlying(v);
			return false;

		case AS_FLYING_LANDING:
		case AS_DESCENDING:
		case AS_LANDED:
			HandleAircraftLanding(v);
			return v->IsHelicopter() || v->state == AS_IDLE;

		case AS_FLYING_HELICOPTER_LANDING:
			RaiseLowerHelicopter(v);
			return true;

		case AS_IDLE:
			return false;

		case AS_HANGAR:
			if (IsHangarTile(v->tile)) {
				AircraftEntersHangar(v);
				v->UpdateNextTile(INVALID_TILE);
				UpdateAircraftState(v);
				return true;
			}
			break;
		case AS_RUNNING:
			Debug(misc, 0, "Moving aircraft {}, shouldn't reach this point. Probably there will be a crash soon.", v->unitnumber);
			break;
		default:
			Debug(misc, 0, "Unhandled state {}, next {}", v->state, v->Next()->state);
			NOT_REACHED();
			break;
	}

	return false;
}

/**
 * Update the aircraft flight level according to aircraft state and position.
 * @param v Aircraft.
 * @pre v->IsAircraftFlying()
 */
void HandleAircraftFlightLevel(Aircraft *v)
{
	assert(v->IsAircraftFlying());

	switch(v->state) {
		case AS_ON_HOLD_WAITING:
		case AS_ON_HOLD_APPROACHING:
			if (v->z_pos > GetAircraftHoldMaxAltitude(v)) v->z_pos--;
			break;
		case AS_DESCENDING: {
			assert(IsValidTile(v->GetNextTile()));
			int z = GetTileHeightBelowAircraft(v) + 1;
			/* Runway may be in a higher tile than the current one. */
			z = std::max(z, GetTileMaxPixelZ(v->GetNextTile()) + 1);
			z = v->z_pos - z;

			if (z > 32) {
				v->z_pos = v->z_pos - 2;
			} else if (z > 16 || (v->tile == v->GetNextTile() && z > 8)) {
				v->z_pos = v->z_pos - 1;
			}
			break;
		}

		case AS_FLYING_LANDING: {
			int z = GetTileHeightBelowAircraft(v) + 1;
			assert(z < v->z_pos);
			v->z_pos -= 1;
			if (v->z_pos == z) HandlePlaneLandsOnRunway(v);
			break;
		}

		default:
			v->z_pos = GetAircraftFlightLevel(v, v->state == AS_FLYING_TAKEOFF);
			break;
	}
}

AircraftPosition _aircraft_pos_offsets[AP_END] = {
	{ AP_DEFAULT                ,   8                      ,   8                       }, // default: middle of the tile
	{ AP_HELICOPTER_HOLD_2      ,   8 +  1 * (int)TILE_SIZE,   0 +  0 * (int)TILE_SIZE }, // on helicopter hold start           respect the apron tile
	{ AP_HELICOPTER_HOLD_3      ,   0 +  0 * (int)TILE_SIZE,   8 +  1 * (int)TILE_SIZE }, // on helicopter hold, pos 2
	{ AP_HELICOPTER_HOLD_4      ,   0 -  1 * (int)TILE_SIZE,   8 +  1 * (int)TILE_SIZE }, // on helicopter hold, pos 3
	{ AP_HELICOPTER_HOLD_5      ,  -8 -  2 * (int)TILE_SIZE,   0 +  0 * (int)TILE_SIZE }, // on helicopter hold, pos 4
	{ AP_HELICOPTER_HOLD_6      ,  -8 -  2 * (int)TILE_SIZE,   0 -  1 * (int)TILE_SIZE }, // on helicopter hold, pos 5
	{ AP_HELICOPTER_HOLD_7      ,   0 -  1 * (int)TILE_SIZE,  -8 -  2 * (int)TILE_SIZE }, // on helicopter hold, pos 6
	{ AP_HELICOPTER_HOLD_END    ,   0 +  0 * (int)TILE_SIZE,  -8 -  2 * (int)TILE_SIZE }, // on helicopter hold, pos 7
	{ AP_HELICOPTER_HOLD_START  ,   8 +  1 * (int)TILE_SIZE,   0 -  1 * (int)TILE_SIZE }, // on helicopter hold, pos 8 and last
	{ AP_HELIPORT_DEST          ,   6                      ,   8                       }, // heliport landing dest              respect the apron tile
	{ AP_BUILTIN_HELIPORT_DEST  , - 2 +  2 * (int)TILE_SIZE,   8                       }, // builtin heliport dest              respect the tile containing the airport
	{ AP_PLANE_BEFORE_FLYING    ,   8                      ,   8                       }, // default: middle of the tile
	{ AP_PLANE_START_FLYING     ,   8 +  1 * (int)TILE_SIZE,   8                       }, // start flying on runway             respect the runway end
	{ AP_PLANE_LEAVE_AIRPORT    ,   8 +  0 * (int)TILE_SIZE,   8                       }, // remove runway reservation          respect the runway end
	{ AP_PLANE_HOLD_START       ,  -8 -  5 * (int)TILE_SIZE,   8                       }, // leaving airport                    respect the runway end
	{ AP_PLANE_HOLD_2           ,   0 +  8 * (int)TILE_SIZE,   8                       }, // on hold start and also descending  respect the runway start tile
	{ AP_PLANE_HOLD_3           ,   0 +  4 * (int)TILE_SIZE,   8                       }, // on hold, pos 2
	{ AP_PLANE_HOLD_4           ,   0 -  8 * (int)TILE_SIZE,   8                       }, // on hold, pos 3
	{ AP_PLANE_HOLD_5           ,  -8 - 11 * (int)TILE_SIZE,   0 -  3 * (int)TILE_SIZE }, // on hold, pos 4
	{ AP_PLANE_HOLD_6           ,  -8 - 11 * (int)TILE_SIZE,   0 -  7 * (int)TILE_SIZE }, // on hold, pos 5
	{ AP_PLANE_HOLD_7           ,   0 -  8 * (int)TILE_SIZE,   8 - 11 * (int)TILE_SIZE }, // on hold, pos 6
	{ AP_PLANE_HOLD_8           ,   0 +  8 * (int)TILE_SIZE,   8 - 11 * (int)TILE_SIZE }, // on hold, pos 7
	{ AP_PLANE_HOLD_END         ,  -8 + 12 * (int)TILE_SIZE,   0 -  7 * (int)TILE_SIZE }, // on hold, pos 8
	{ AP_PLANE_HOLD_START       ,  -8 + 12 * (int)TILE_SIZE,   0 -  3 * (int)TILE_SIZE }, // on hold, pos 9 and last
	{ AP_PLANE_LANDING          ,   8                      ,   8                       }, // descending
	{ AP_DEFAULT                ,   8                      ,   8                       }, // landing

};

/**
 * Get the position for a given position type and rotation.
 * @param pos Position type
 * @param dir DiagDirection indicating the rotation to apply.
 * @return aircraft position for the position type and rotation.
 */
AircraftPosition RotatedAircraftPosition(AircraftPos pos, DiagDirection dir)
{
	AircraftPosition rotated_pos;
	rotated_pos = _aircraft_pos_offsets[pos];
	switch (dir) {
		case DIAGDIR_NE:
			break;
		case DIAGDIR_SE:
			rotated_pos.x = _aircraft_pos_offsets[pos].y;
			rotated_pos.y = TILE_SIZE - _aircraft_pos_offsets[pos].x;
			break;
		case DIAGDIR_SW:
			rotated_pos.x = TILE_SIZE - _aircraft_pos_offsets[pos].x;
			rotated_pos.y = TILE_SIZE - _aircraft_pos_offsets[pos].y;
			break;
		case DIAGDIR_NW:
			rotated_pos.x = TILE_SIZE - _aircraft_pos_offsets[pos].y;
			rotated_pos.y = _aircraft_pos_offsets[pos].x;
			break;
		default:
			NOT_REACHED();
	}

	return rotated_pos;
}

AircraftPos helicopter_entry_point[8] = {
	AP_HELICOPTER_HOLD_2,
	AP_HELICOPTER_HOLD_7,
	AP_HELICOPTER_HOLD_3,
	AP_HELICOPTER_HOLD_6,
	AP_HELICOPTER_HOLD_START,
	AP_HELICOPTER_HOLD_END,
	AP_HELICOPTER_HOLD_4,
	AP_HELICOPTER_HOLD_5,
};

AircraftPos plane_entry_pos[DIAGDIR_END][4] = {
	{AP_PLANE_HOLD_START, AP_PLANE_HOLD_7, AP_PLANE_HOLD_5, AP_PLANE_HOLD_3},
	{AP_PLANE_HOLD_5, AP_PLANE_HOLD_3, AP_PLANE_HOLD_7, AP_PLANE_HOLD_START},
	{AP_PLANE_HOLD_7, AP_PLANE_HOLD_START, AP_PLANE_HOLD_3, AP_PLANE_HOLD_5},
	{AP_PLANE_HOLD_3, AP_PLANE_HOLD_5, AP_PLANE_HOLD_START, AP_PLANE_HOLD_7},
};

/**
 * Get the offset position an aircraft must get respect a tile or its next position.
 * @param tile The tile the aircraft tries to reach.
 * @param next_pos The next position type the aircraft is trying to reach.
 * @return the destination position of the aircraft.
 */
AircraftPosition GetAircraftPositionByTile(TileIndex tile, AircraftPos next_pos) {
	assert(IsAirportTile(tile));

	switch (GetAirportTileType(tile)) {
		case ATT_APRON_NORMAL:
		case ATT_APRON_HELIPAD:
			return _aircraft_pos_offsets[AP_DEFAULT];
		case ATT_APRON_HELIPORT: {
			DiagDirection diagdir = GetAirportTileRotation(tile);
			return RotatedAircraftPosition(AP_HELIPORT_DEST, diagdir);
		}
		case ATT_APRON_BUILTIN_HELIPORT:
			return _aircraft_pos_offsets[AP_BUILTIN_HELIPORT_DEST];
		case ATT_RUNWAY_START_NO_LANDING:
		case ATT_RUNWAY_START_ALLOW_LANDING:
			return _aircraft_pos_offsets[AP_DEFAULT];
		default:
			return _aircraft_pos_offsets[next_pos];
	}
}


/**
 * @param v Aircraft
 */
void SetNextAircraftPosition(Aircraft &v)
{
	TileIndex tile = v.GetNextTile();
	AircraftPos next_pos = v.next_pos.pos;
	DiagDirection diagdir = DIAGDIR_NE;

	switch (v.state) {
		case AS_START_TAKEOFF:
			next_pos = AP_START_TAKE_OFF;
			[[fallthrough]];
		case AS_DESCENDING:
		case AS_FLYING_LEAVING_AIRPORT:
		case AS_TAKEOFF_BEFORE_FLYING:
		case AS_FLYING_TAKEOFF:
			if (!IsValidTile(tile)) break;
			assert(IsAirportTile(tile));
			if (v.IsHelicopter()) {
				assert(IsApron(tile));
				v.next_pos = GetAircraftPositionByTile(tile, AP_DEFAULT);
			} else {
				if (v.state == AS_DESCENDING) next_pos = AP_PLANE_DESCENDING;
				assert(IsRunwayExtreme(tile));
				diagdir = GetRunwayExtremeDirection(tile);
				v.next_pos = RotatedAircraftPosition(next_pos, diagdir);
			}
			break;

		case AS_FLYING_NO_DEST:
			if (next_pos == AP_DEFAULT) {
				diagdir = DirToDiagDir(v.direction);
				next_pos = AP_PLANE_HOLD_START;
			}
			v.next_pos = RotatedAircraftPosition(next_pos, diagdir);
			break;

		case AS_FLYING_LANDING:
			assert(IsRunwayEnd(tile) && IsLandingTypeTile(tile));
			diagdir = GetRunwayExtremeDirection(tile);
			diagdir = ReverseDiagDir(diagdir);
			v.next_pos = RotatedAircraftPosition(AP_PLANE_LANDING, diagdir);
			break;

		case AS_LANDED:
			assert(IsValidTile(tile));
			assert(IsAirportTile(tile));
			assert(!v.IsHelicopter());
			assert(IsRunwayEnd(tile) && IsLandingTypeTile(tile));
			diagdir = GetRunwayExtremeDirection(tile);
			v.next_pos = RotatedAircraftPosition(AP_PLANE_LANDING, diagdir);
			break;

		case AS_ON_HOLD_APPROACHING:
			if (v.IsHelicopter()) {
				assert(IsAirportTile(tile));
				v.next_pos = GetAircraftPositionByTile(tile, AP_DEFAULT);

				break;
			}
			[[fallthrough]];

		case AS_ON_HOLD_WAITING:
			if (!v.IsHelicopter()) {
				assert(IsAirportTile(tile));
				assert(IsRunwayExtreme(tile));
				/* If heading to the same runway, but it is occupied, try rotating. */
				diagdir = GetRunwayExtremeDirection(tile);
				v.next_pos = RotatedAircraftPosition(next_pos, diagdir);
				break;
			}
			[[fallthrough]];

		case AS_FLYING: {
			// Decide the entry point
			AircraftPosition origin_offset;
			assert(IsAirportTile(tile));
			if (v.IsHelicopter()) {
				origin_offset = GetAircraftPositionByTile(tile, AP_DEFAULT);
			} else {
				assert(IsAirportTile(tile));
				assert(IsRunwayStart(tile) && IsLandingTypeTile(tile));
				diagdir = GetRunwayExtremeDirection(tile);
				origin_offset = RotatedAircraftPosition(AP_PLANE_HOLD_START, diagdir);
			}

			int delta_x = v.x_pos - TileX(tile) * TILE_SIZE - origin_offset.x;
			int delta_y = v.y_pos - TileY(tile) * TILE_SIZE - origin_offset.y;

			uint entry_num;
			AircraftPos entry_pos;
			if (v.IsHelicopter()) {
				entry_num = (delta_y < 0) +
						((delta_x < 0) << 1) +
						((abs(delta_y) < abs(delta_x)) << 2);
				entry_pos = helicopter_entry_point[entry_num];
			} else {
				if (abs(delta_y) < abs(delta_x)) {
					/* We are northeast or southwest of the airport */
					entry_pos = plane_entry_pos[diagdir][delta_x < 0];
				} else {
					/* We are northwest or southeast of the airport */
					entry_pos = plane_entry_pos[diagdir][(delta_y < 0) + 2];
				}
			}

			v.next_pos = RotatedAircraftPosition(entry_pos, diagdir);
			if (v.IsHelicopter()) {
				v.next_pos.x += origin_offset.x;
				v.next_pos.y += origin_offset.y;
			}
			break;
		}

		default:
			if (!IsValidTile(tile)) break;
			v.next_pos = GetAircraftPositionByTile(tile, next_pos);
			break;
	}
}

/**
 * Update this->next_pos and next path tile (this->Next()->dest_tile).
 * Use it after updating next_tile or when next desired position changes
 * (i.e. when flying and approaching a runway).
 * @param tile next tile
 */
void Aircraft::UpdateNextTile(TileIndex tile)
{
	if (tile == 0) return;

	/* Update next path tile. */
	this->Next()->dest_tile = tile;

	SetNextAircraftPosition(*this);

	this->next_pos.x += TileX(tile) * TILE_SIZE;
	this->next_pos.y += TileY(tile) * TILE_SIZE;
}

/** Set the right pos when heading to other airports after takeoff.
 * @param v Aircraft.
 */
void AircraftUpdateNextPos(Aircraft *v)
{
	assert(v->IsAircraftFreelyFlying());

	TileIndex tile = v->GetNextTile();
	if (IsValidTile(tile) && IsAirportTile(tile) &&
			IsRunwayStart(tile) && v->targetairport == GetStationIndex(tile)) {
		return;
	}

	AssignLandingTile(v, FindClosestLandingTile(v));
	v->UpdateNextTile(v->GetNextTile());
}

/**
 * Get a tile where aircraft can land. For helicopters, it will check helipads, heliports
 * and aprons, in this ordrer, and finally runways. For normal aircraft, it will check runways.
 * @param v The aircraft trying to land.
 * @return a valid tile where to land, or 0 otherwise.
 */
TileIndex FindClosestLandingTile(Aircraft *v)
{
	v->targetairport = GetTargetDestination(v->current_order, true);
	assert(Station::IsValidID(v->targetairport));
	Station *st = Station::GetIfValid(v->targetairport);

	if (!CanVehicleUseStation(v, st)) return 0;

	TileIndex landing_tile = 0;
	TileIndex free_landing_tile = 0;
	uint32_t best_dist = UINT32_MAX;
	uint32_t free_best_dist = UINT32_MAX;

	if (v->IsHelicopter()) {
		for (auto &it : st->airport.helipads) {
			if (DistanceSquare(it, v->tile) < best_dist) {
				landing_tile = it;
				best_dist = DistanceSquare(it, v->tile);
			}
			if (!HasAirportTrackReserved(it) && DistanceSquare(it, v->tile) < free_best_dist) {
				free_landing_tile = it;
				free_best_dist = DistanceSquare(it, v->tile);
			}
		}

		if (free_landing_tile != 0) return free_landing_tile;

		if (v->current_order.GetType() != OT_GOTO_DEPOT) {
			for (auto &it : st->airport.heliports) {
				if (DistanceSquare(it, v->tile) < best_dist) {
					landing_tile = it;
					best_dist = DistanceSquare(it, v->tile);
				}
				if (!HasAirportTrackReserved(it) && DistanceSquare(it, v->tile) < free_best_dist) {
					free_landing_tile = it;
					free_best_dist = DistanceSquare(it, v->tile);
				}
			}
		}

		if (free_landing_tile != 0) return free_landing_tile;

		for (auto &it : st->airport.aprons) {
			if (DistanceSquare(it, v->tile) < best_dist) {
				landing_tile = it;
				best_dist = DistanceSquare(it, v->tile);
			}
			if (!HasAirportTrackReserved(it) && DistanceSquare(it, v->tile) < free_best_dist) {
				free_landing_tile = it;
				free_best_dist = DistanceSquare(it, v->tile);
			}
		}

		if (free_landing_tile != 0) return free_landing_tile;

		for (auto &it : st->airport.aprons) {
			if (DistanceSquare(it, v->tile) < best_dist) {
				landing_tile = it;
				best_dist = DistanceSquare(it, v->tile);
			}
			if (!HasAirportTrackReserved(it) && DistanceSquare(it, v->tile) < free_best_dist) {
				free_landing_tile = it;
				free_best_dist = DistanceSquare(it, v->tile);
			}
		}

		return landing_tile;
	}

	for (auto &it : st->airport.runways) {
		if (!IsLandingTypeTile(it)) continue;
		if (DistanceSquare(it, v->tile) < best_dist) {
			landing_tile = it;
			best_dist = DistanceSquare(it, v->tile);
		}
		if (CanRunwayBeReserved(it) &&
				DistanceSquare(it, v->tile) < free_best_dist) {
			free_landing_tile = it;
			free_best_dist = DistanceSquare(it, v->tile);
		}
	}

	if (free_landing_tile != 0) return free_landing_tile;

	return landing_tile;
}

/**
 * Get a tile where aircraft can land. For helicopters, it will check helipads, heliports
 * and aprons, in this ordrer, and finally runways. For normal aircraft, it will check runways.
 * @param v The aircraft trying to land.
 * @return a valid tile where to land, or INVALID_TILE otherwise.
 */
TileIndex FindClosestFreeLandingTile(Aircraft *v) {
	TileIndex tile = FindClosestLandingTile(v);
	if (tile == 0) return INVALID_TILE;
	if (HasAirportTrackReserved(tile)) return INVALID_TILE;
	return tile;
}


ClosestDepot Aircraft::FindClosestDepot()
{
	const Station *st = Station::GetIfValid(this->GetCurrentAirportID());
	if (st == nullptr || !st->airport.HasHangar()) st = GetTargetAirportIfValid(this);
	/* If the station is not a valid airport or if it has no hangars */
	if (st == nullptr || !CanVehicleUseStation(this, st) || !st->airport.HasHangar()) {
		/* the aircraft has to search for a hangar on its own */
		StationID station = FindClosestHangar(this);

		if (station == INVALID_STATION) return ClosestDepot();

		st = Station::Get(station);
	}

	return ClosestDepot(st->airport.hangar->depot_tiles[0], st->airport.hangar->index, st->index);
}

/**
 * Checks whether an aircraft can land on the next targetairport.
 * It checks whether it can land (helipads for helicopters, whether there is a landing runway...).
 * It also checks if the destination is too far.
 * @param v Aircraft
 * @return whether it can reach its targetairport
 */
bool IsReachableDest(Aircraft *v)
{
	assert(IsAirportTile(v->tile));
	assert(!v->IsAircraftFlying());
	if (v->targetairport == GetStationIndex(v->tile)) return true;
	if (v->targetairport == INVALID_STATION) return false;

	assert(Station::IsValidID(v->targetairport));
	Station *st = Station::Get(v->targetairport);

	TileIndex closest_landing = FindClosestLandingTile(v);
	if (closest_landing == 0 || !CanVehicleUseStation(v, st)) {
		if (!HasBit(v->flags, VAF_CAN_T_LAND)) {
			SetBit(v->flags, VAF_CAN_T_LAND);
			v->SetWaitTime(AIRCRAFT_WAIT_FREE_PATH_TICKS);
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
			AI::NewEvent(v->owner, new ScriptEventAircraftNoLandDest(v->index));
			if (v->owner == _local_company) {
				/* Post a news message. */
				SetDParam(0, v->index);
				AddVehicleAdviceNewsItem(STR_NEWS_AIRCRAFT_CAN_T_LAND, v->index);
			}
		}
		if (v->state != AS_HANGAR) {
			v->state = AS_IDLE;
			v->UpdateNextTile(v->tile);
		}
		return false;
	} else if (HasBit(v->flags, VAF_CAN_T_LAND)) {
		/* Aircraft can land now. */
		ClrBit(v->flags, VAF_CAN_T_LAND);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		DeleteVehicleNews(v->index, STR_NEWS_AIRCRAFT_CAN_T_LAND);
	}

	if (v->acache.cached_max_range_sqr == 0) return true;
	Station *cur_st = Station::GetIfValid(GetStationIndex(v->tile));

	if (DistanceSquare(cur_st->airport.tile, closest_landing) > v->acache.cached_max_range_sqr) {
		if (!HasBit(v->flags, VAF_DEST_TOO_FAR)) {
			SetBit(v->flags, VAF_DEST_TOO_FAR);
			v->SetWaitTime(AIRCRAFT_WAIT_FREE_PATH_TICKS);
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

void AssignLandingTile(Aircraft *v, TileIndex tile)
{
	assert(v->IsAircraftFreelyFlying());

	if (tile != 0 && IsValidTile(tile)) {
		assert(IsAirportTile(tile));
		assert((IsRunwayStart(tile) && IsLandingTypeTile(tile)) || (v->IsHelicopter() && IsApron(tile)));
		v->state = AS_FLYING;
		v->UpdateNextTile(tile);
	} else {
		v->state = AS_FLYING_NO_DEST;
		v->next_pos.pos = AP_DEFAULT;
		v->UpdateNextTile(v->tile);
	}

	v->next_pos.pos = v->IsHelicopter() ? AP_HELICOPTER_HOLD_START : AP_PLANE_HOLD_START;
}

/**
 * Handle aircraft with missing orders.
 * @param v An aircraft with missing orders.
 */
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
	if (st == nullptr) {
		Backup<CompanyID> cur_company(_current_company, v->owner);
		CommandCost ret = Command<CMD_SEND_VEHICLE_TO_DEPOT>::Do(DC_EXEC, v->index, DepotCommand::None, {});
		cur_company.Restore();

		if (ret.Failed()) HandleAircraftFalling(v);
	} else if (!v->current_order.IsType(OT_GOTO_DEPOT)) {
		v->current_order.Free();
	}
}

/**
 * Set a destination tile. For aircraft, it won't be assigned directly to this->dest_tile.
 * @param tile hangar or apron tile of destination airport
 *             (hangar/apron depending on current order type being GOTO_DEPOT/GOTO_STATION).
 * @pre tile == 0 || (IsAirportTile(tile) && (IsHangar(tile) || IsApron(tile)))
 */
void Aircraft::SetDestTile(TileIndex tile)
{
	if (tile != 0) {
		assert(IsValidTile(tile));
		assert(IsAirportTile(tile));
		assert(IsHangar(tile) || IsApron(tile));
	}

	if (this->dest_tile == tile) return;

	this->dest_tile = tile;
	this->targetairport = GetTargetDestination(this->current_order, true);

	if (this->IsAircraftFreelyFlying()) {
		this->state = AS_FLYING;
		AircraftUpdateNextPos(this);
	}

	SetWindowWidgetDirty(WC_VEHICLE_VIEW, this->index, WID_VV_START_STOP);
}

/**
 * For moving aircraft, it lifts its current path
 * and looks for the best path. It will find the same
 * starting path or a best one.
 * @param v A moving aircraft.
 */
void UpdatePath(Aircraft *v)
{
	assert(v->state == AS_RUNNING);
	assert(v->next_trackdir == INVALID_TRACKDIR);
	LiftAirportPathReservation(v, false);

	/* Look for a path again with the same destination. */
	PBSTileInfo best_dest;
	bool path_found;
	Trackdir first_trackdir = YapfAircraftFindPath(v, best_dest, path_found, v->Next()->state, v->path);

	/* If a reservable path existed, a reservable path must exist. */
	assert(path_found);
	assert(first_trackdir != INVALID_TRACKDIR);
	assert(best_dest.okay);
	assert(IsValidTile(best_dest.tile));
	v->UpdateNextTile(best_dest.tile);

	if (v->trackdir != first_trackdir) {
		v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
		v->next_trackdir = first_trackdir;
	}
}

/**
 * Checks if a path reservation can be made towards
 * next target of the aircraft.
 * @param v Aircraft to check.
 * @return Whether a path can be reserved.
 */
bool TryReservePath(Aircraft *v)
{
	assert(v->state < AS_MOVING);

	/* First, assert diagonal diadgir.
	 * We shouldn't start paths in stranger tracks. */
	assert(IsDiagonalTrackdir(v->GetVehicleTrackdir()));

	v->UpdateNextTile(INVALID_TILE);

	/* Then, if inside a standard hangar, make sure it is not reserved. */
	if (v->vehstatus & VS_HIDDEN) {
		assert(IsHangarTile(v->tile));
		if (IsStandardHangar(v->tile) && HasAirportTrackReserved(v->tile)) return false;
	}

	if (IsApron(v->tile) &&
			v->targetairport == GetStationIndex(v->tile) &&
			IsTerminalState(v->state)) {
		return false;
	}

	PBSTileInfo best_dest;
	bool path_found;
	AircraftState dest_state = GetNextAircraftState(*v);
	Trackdir first_trackdir = YapfAircraftFindPath(v, best_dest, path_found, dest_state, v->path);
	v->HandlePathfindingResult(path_found);

	if (!path_found) return false;

	assert(first_trackdir != INVALID_TRACKDIR);
	assert(IsValidTile(best_dest.tile));

	/* A path exists but right now cannot be reserved. */
	if (!best_dest.okay) return false;

	// revise possible unneeded servicing here
	if (v->state != AS_HANGAR && dest_state == AS_HANGAR && !v->current_order.IsType(OT_GOTO_DEPOT)) {
		/* Create the hangar order. */
		// revise
		Depot *hangar = Station::GetByTile(v->tile)->airport.hangar;
		assert(hangar != nullptr);
		v->current_order.MakeGoToDepot(hangar->index, ODTFB_SERVICE);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}

	v->UpdateNextTile(best_dest.tile);

	/* If a path is found, service, reserve and return true. */
	if (IsHangarTile(v->tile)) {
		assert(IsValidTrackdir(first_trackdir));
		SetAirportTracksReservation(v->tile, TrackToTrackBits(TrackdirToTrack(first_trackdir)));

		if (v->cur_speed != 0) SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		v->cur_speed = 0;
		v->subspeed = 0;
		v->progress = 0;

		/* Rotor blades */
		if (v->Next()->Next() != nullptr) {
			v->Next()->Next()->cur_speed = 80;
		}

		if (!IsExtendedHangar(v->tile)) {
			v->trackdir = v->Next()->trackdir = first_trackdir;
			SetVisibility(v, true);
		}

		AircraftLeavesHangar(v);
		v->PlayLeaveStationSound();
	}

	assert(IsDiagonalTrackdir(first_trackdir));
	if (first_trackdir != v->GetVehicleTrackdir()) {
		v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
		v->next_trackdir = first_trackdir;
		if (GetReservedAirportTracks(v->tile) == TRACK_BIT_CROSS) {
			assert(IsValidTrackdir(v->trackdir));
			RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
		}
	}

	if (v->tile != v->GetNextTile() && v->GetNextTile() != INVALID_TILE) {
		v->state = AS_RUNNING;
		v->Next()->state = dest_state;
	}

	return true;
}

/**
 * While aircraft is on land and moving through an airport,
 * check whether it is in the middle of a tile. If it is the middle of
 * a tile, try updating the path and the next trackdir, if needed.
 * @param v Aircraft to check.
 * @param gp New position of the aircraft.
 * @return Whether it needs to rotate.
 */
bool TryRotateInMiddleOfTile(Aircraft *v, const GetNewVehiclePosResult &gp) {
	assert(v->state == AS_RUNNING);
	assert(IsAirportTile(gp.new_tile));
	assert(MayHaveAirTracks(gp.new_tile));

	if ((gp.x & 0xF) != 8 || (gp.y & 0xF) != 8) return false;

	/* Check whether the aircraft must rotate in the middle of the tile. */
	if (GetReservedAirportTracks(gp.new_tile) != TRACK_BIT_CROSS) return false;

	assert(IsValidTrackdir(v->trackdir));
	assert(v->next_trackdir == INVALID_TRACKDIR);

	/* A good moment to update the path. */
	//UpdatePath(v);

	if (DoesAircraftNeedRotation(v)) return true;

	if (GetReservedAirportTracks(gp.new_tile) == TRACK_BIT_CROSS) {
		RemoveAirportTrackReservation(gp.new_tile, TrackdirToTrack(v->trackdir));
		assert(!v->path.empty());
		assert(v->path.tile.front() == gp.new_tile);
		v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
		v->next_trackdir = v->path.td.front();
		v->path.pop_front();
	}

	return true;
}

/**
 * Moves an aircraft one time.
 * @param v Aircraft to move.
 * @param nudge_towards_target Indicates whether v is flying and close to its target.
 */
void MoveAircraft(Aircraft *v, const bool nudge_towards_target)
{
	GetNewVehiclePosResult gp;

	if (nudge_towards_target) {
		/* Move vehicle one pixel towards target. */
		gp.x = (v->x_pos != v->next_pos.x) ? v->x_pos + ((v->next_pos.x > v->x_pos) ? 1 : -1) : v->x_pos;
		gp.y = (v->y_pos != v->next_pos.y) ? v->y_pos + ((v->next_pos.y > v->y_pos) ? 1 : -1) : v->y_pos;

		/* Builtin heliports keep v->tile as the terminal tile, since the landing pad is in a non-airport tile. */
		gp.new_tile = IsBuiltInHeliportTile(v->GetNextTile()) ? v->GetNextTile() : TileVirtXY(gp.x, gp.y);
	} else if (v->state > AS_RUNNING) {
		/* Aircraft is flying or moving in a runway. */
		assert(!v->IsHelicopter() || ((v->state != AS_LANDED && v->state != AS_START_TAKEOFF)));

		/* Turn. Do it slowly if in the air. */
		if (v->turn_counter != 0) v->turn_counter--;
		Direction newdir = GetDirectionTowards(v, v->next_pos.x, v->next_pos.y);
		if (newdir == v->direction) {
			v->number_consecutive_turns = 0;
		} else if (v->turn_counter == 0 || newdir == v->last_direction) {
			if (newdir == v->last_direction) {
				v->number_consecutive_turns = 0;
			} else {
				v->number_consecutive_turns++;
			}
			v->turn_counter = v->IsHelicopter() ? 0 : (2 * _settings_game.vehicle.plane_speed);
			v->last_direction = v->direction;
			v->direction = v->Next()->direction = newdir;
		}

		gp = GetNewVehiclePos(v);
	} else {
		/* Aircraft is taxiing on the airport. */
		assert(v->state == AS_RUNNING);

		gp = GetNewVehiclePos(v);

		if (gp.old_tile == gp.new_tile) {
			if (TryRotateInMiddleOfTile(v, gp)) return;
		} else {
			/* Entering a new tile. */
			assert(IsTileType(gp.new_tile, MP_STATION));
			assert(IsAirportTile(gp.new_tile));
			assert(MayHaveAirTracks(gp.new_tile));
			assert(IsValidTrackdir(v->trackdir));
			assert(v->next_trackdir == INVALID_TRACKDIR);

			//UpdatePath(v);

			if (DoesAircraftNeedRotation(v)) return;

			RemoveAirportTrackReservation(gp.old_tile, TrackdirToTrack(v->trackdir));
			TrackdirBits trackdirs = TrackdirReachesTrackdirs(v->trackdir) &
					TrackBitsToTrackdirBits(GetReservedAirportTracks(gp.new_tile));

			if (trackdirs == TRACKDIR_BIT_NONE) {
				/* Rotate at the end of the tile. */
				DiagDirection exit_dir = TrackdirToExitdir(v->trackdir);
				trackdirs = DiagdirReachesTrackdirs(ReverseDiagDir(exit_dir)) &
						TrackBitsToTrackdirBits(GetReservedAirportTracks(gp.old_tile));

				/* Must reverse now and rotate in the middle of the tile. */
				if (CountBits(trackdirs) == 0) {
					[[maybe_unused]] TrackBits reserved_tracks = GetReservedAirportTracks(gp.old_tile);
					assert(CountBits(reserved_tracks) == 1);
					assert(IsDiagonalTrack(RemoveFirstTrack(&reserved_tracks)));
					v->SetWaitTime(AIRCRAFT_ROTATION_STEP_TICKS);
					v->next_trackdir = ReverseTrackdir(v->trackdir);
					SetAirportTrackReservation(gp.old_tile, TrackdirToTrack(v->next_trackdir));
					return;
				}

				assert(CountBits(trackdirs) == 1);
				v->next_trackdir = RemoveFirstTrackdir(&trackdirs);
				assert(trackdirs == TRACKDIR_BIT_NONE);
				SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
				return;
			}

			v->trackdir = v->Next()->trackdir = RemoveFirstTrackdir(&trackdirs);
			assert(IsValidTrackdir(v->trackdir));

			DiagDirection diagdir = DiagdirBetweenTiles(gp.old_tile, gp.new_tile);
			const AircraftSubcoordData &b = _aircraft_subcoord[diagdir][TrackdirToTrack(v->trackdir)];
			gp.x = (gp.x & ~0xF) | b.x_subcoord;
			gp.y = (gp.y & ~0xF) | b.y_subcoord;

			uint32_t r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
			if (HasBit(r, VETS_CANNOT_ENTER)) NOT_REACHED();

			v->direction = v->Next()->direction = b.dir;
		}
	}

	v->tile = gp.new_tile;
	v->x_pos = gp.x;
	v->y_pos = gp.y;

	if (v->IsAircraftFlying()) HandleAircraftFlightLevel(v);
}

/**
 * Moves the aircraft one time.
 * @param v Aircraft to move.
 * @return whether the vehicle can move more times during this tick.
 */
bool HandleAircraftMovement(Aircraft *v)
{
	if (v->IsAircraftFalling()) {
		HandleAircraftFalling(v);
		return true;
	}

	if (DoesAircraftNeedRotation(v)) {
		DoRotationStep(v);
		if (v->state == AS_START_TAKEOFF && !DoesAircraftNeedRotation(v)) {
			/* Take off starts right now. */
			PlayAircraftTakeoffSound(v);
		}
		return true;
	}

	if (v->IsHelicopter() && RaiseLowerHelicopter(v)) return true;

	if (HandleAircraftState(v)) return true;

	if (v->state < AS_MOVING) return false;

	/* Maybe crash the airplane if landing too fast. */
	assert(v->state != AS_LANDED || IsAirportTile(v->tile));
	if (v->state == AS_LANDED &&
			v->cur_speed > GetAirTypeInfo(GetAirType(v->tile))->max_speed * _settings_game.vehicle.plane_speed) {
		if (MaybeCrashAirplane(v)) return true;
	}

	int count = UpdateAircraftSpeed(v);

	if (v->next_trackdir != INVALID_TRACKDIR) return true;

	/* If the plane will be a few subpixels away from the destination after
	 * this movement loop, start nudging it towards the exact position for
	 * the whole loop. Otherwise, heavily depending on the speed of the plane,
	 * it is possible we totally overshoot the target, causing the plane to
	 * make a loop, and trying again, and again, and again .. */
	bool nudge_towards_target = v->IsAircraftFlying() &&
			count + 3 > abs(v->next_pos.x - v->x_pos) +  abs(v->next_pos.y - v->y_pos);

	for (; count > 0; count--) {
		MoveAircraft(v, nudge_towards_target);
		if (HandleAircraftState(v)) break;
	}

	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
	return true;
}

/**
 * Aircraft controller.
 * @param v Aircraft to move.
 * @param mode False during the first call in each tick, true during second call.
 * @return whether the vehicle is still valid.
 */
static bool AircraftController(Aircraft *v, bool mode)
{
	/* Aircraft crashed? */
	if (v->vehstatus & VS_CRASHED) {
		return mode ? true : HandleCrashedAircraft(v); // 'v' can be deleted here
	}

	if ((v->vehstatus & VS_STOPPED) && v->cur_speed == 0) return true;

	if (v->IsServicing()) {
		if (mode) v->ContinueServicing();
		return true;
	}

	v->HandleBreakdown();

	HandleAircraftSmoke(v, mode);

	if (v->IsWaiting()) {
		if (mode) v->AdvanceWaitTime();
		return true;
	}

	ProcessOrders(v);

	v->HandleLoading(mode);
	if (v->current_order.IsType(OT_LOADING)) return true;

	/* Check if we should wait here for unbunching. */
	if (v->state == AS_HANGAR && v->IsWaitingForUnbunching()) return true;

	if (HandleAircraftMovement(v)) return true;

	/* Check if next destination is too far. */
	if (!IsReachableDest(v)) {
		if (!v->IsWaiting()) v->SetWaitTime(AIRCRAFT_WAIT_FREE_PATH_TICKS);
		return true;
	}

	/* Check whether aircraft can reserve a path towards its next target. */
	if (!TryReservePath(v)) {
		/* Aircraft cannot reserve a path now. */
		v->SetWaitTime(v->state == AS_HANGAR ? AIRCRAFT_WAIT_LEAVE_HANGAR_TICKS : AIRCRAFT_WAIT_FREE_PATH_TICKS);
	}

	return true;
}

/**
 * Update aircraft vehicle data for a tick.
 * @return True if the vehicle still exists, false if it has ceased to exist (normal aircraft only).
 */
bool Aircraft::Tick()
{
	if (!this->IsNormalAircraft()) return true;

	PerformanceAccumulator framerate(PFE_GL_AIRCRAFT);

	this->tick_counter++;

	if (!(this->vehstatus & VS_STOPPED)) this->running_ticks++;

	if (this->IsHelicopter()) HandleHelicopterRotor(this);

	this->current_order_time++;

	for (uint i = 0; i != 2; i++) {
		/* stop if the aircraft was deleted */
		if (!AircraftController(this, i)) return false;
	}

	return true;
}
