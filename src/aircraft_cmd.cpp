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
#include "error_func.h"
#include "strings_func.h"
#include "command_func.h"
#include "window_func.h"
#include "timer/timer_game_calendar.h"
#include "timer/timer_game_economy.h"
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
#include "newgrf_airporttiles.h"
#include "framerate_type.h"
#include "aircraft_cmd.h"
#include "vehicle_cmd.h"
#include "depot_base.h"
#include "air_map.h"
#include "pbs_air.h"

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
				if (IsAirportPositionFree(tile, DiagDirToDiagTrack(GetHangarDirection(tile)))) return tile;
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
		v->SetNext(u);

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

	const Station *st = Station::Get(GetTargetDestination(v->current_order, true));

	assert(st != nullptr);

	/* only goto depot if the target airport has a depot */
	if (st->airport.HasHangar() && CanVehicleUseStation(v, st)) {
		v->current_order.MakeGoToDepot(st->airport.hangar->index, ODTFB_SERVICE);
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

static void PlayAircraftSound(const Vehicle *v)
{
	if (!PlayVehicleSound(v, VSE_START)) {
		SndPlayVehicleFx(AircraftVehInfo(v->engine_type)->sfx, v);
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
	return TileHeight(TileVirtXY(safe_x, safe_y)) * TILE_HEIGHT;
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

uint Aircraft::Crash(bool flooded)
{
	uint victims = Vehicle::Crash(flooded) + 2; // pilots
	this->crashed_counter = flooded ? 9000 : 0; // max 10000, disappear pretty fast when flooded

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
	StringID newsitem;
	TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);
	if (st == nullptr) {
		newsitem = STR_NEWS_AIRCRAFT_CRASH_NO_AIRPORT;
	} else {
		SetDParam(1, st->index);
		newsitem = STR_NEWS_AIRCRAFT_CRASH;
	}

	AI::NewEvent(v->owner, new ScriptEventVehicleCrashed(v->index, vt, st == nullptr ? ScriptEventVehicleCrashed::CRASH_AIRCRAFT_NO_AIRPORT : ScriptEventVehicleCrashed::CRASH_PLANE_LANDING, victims));
	Game::NewEvent(new ScriptEventVehicleCrashed(v->index, vt, st == nullptr ? ScriptEventVehicleCrashed::CRASH_AIRCRAFT_NO_AIRPORT : ScriptEventVehicleCrashed::CRASH_PLANE_LANDING, victims));

	NewsType newstype = NT_ACCIDENT;
	if (v->owner != _local_company) {
		newstype = NT_ACCIDENT_OTHER;
	}

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

	return true;
}


/**
 * Handle Aircraft specific tasks when an Aircraft enters a hangar
 * @param *v Vehicle that enters the hangar
 */
void AircraftEntersHangar(Aircraft *v)
{
	v->subspeed = 0;
	v->progress = 0;

	Aircraft *u = v->Next();
	u->vehstatus |= VS_HIDDEN;
	u = u->Next();
	if (u != nullptr) {
		u->vehstatus |= VS_HIDDEN;
		u->cur_speed = 0;
	}

	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
}

/**
 * Aircraft is about to leave the hangar.
 * @param v Aircraft leaving.
 * @param exit_dir The direction the vehicle leaves the hangar.
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
		if (u != nullptr) {
			u->vehstatus &= ~VS_HIDDEN;
			u->cur_speed = 80;
		}
	}

	VehicleServiceInDepot(v);
	v->LeaveUnbunchingDepot();
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
	if (v->current_order.IsType(OT_GOTO_DEPOT)) return;

	Station *st = Station::Get(v->targetairport);
	v->last_station_visited = v->targetairport;

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

	v->BeginLoading();
}

/**
 * Plane touched down at the landing strip.
 * @param v Aircraft that landed.
 */
static void PlaneLandsOnRunway(Aircraft *v)
{
	Station *st = Station::Get(v->targetairport);

	TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);

	v->UpdateDeltaXY();

	AirportTileAnimationTrigger(st, vt, AAT_STATION_AIRPLANE_LAND);

	if (!PlayVehicleSound(v, VSE_TOUCHDOWN)) {
		SndPlayVehicleFx(SND_17_SKID_PLANE, v);
	}
}

/**
 * Returns aircraft's target station if v->target_airport
 * is a valid station with airport.
 * @param v vehicle to get target airport for
 * @return pointer to target station, nullptr if invalid
 */
Station *GetTargetAirportIfValid(const Aircraft *v)
{
	assert(v->type == VEH_AIRCRAFT);

	Station *st = Station::GetIfValid(v->targetairport);
	if (st == nullptr) return nullptr;

	return st->airport.tile == INVALID_TILE ? nullptr : st;
}

TileIndex Aircraft::GetOrderStationLocation(StationID station)
{
	/* Orders are changed in flight, ensure going to the right station. */
	if (this->state == AS_FLYING) {
		//AircraftNextAirportPos_and_Order(this);
	}

	/* Aircraft do not use dest-tile */
	return 0;
}

/**
 * Find the nearest hangar to v
 * INVALID_STATION is returned, if the company does not have any suitable
 * airports (like helipads only)
 * @param v vehicle looking for a hangar
 * @return the StationID if one is found, INVALID_STATION otherwise
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
				(v->current_order.IsType(OT_GOTO_DEPOT) && (v->current_order.GetDepotActionType() & ODATFB_NEAREST_DEPOT == 0))) {
			last_dest = Station::GetIfValid(v->last_station_visited);
			next_dest = Station::GetIfValid(GetTargetDestination(v, &(v->current_order)));
		} else {
			last_dest = GetTargetAirportIfValid(v);
			next_dest = Station::GetIfValid(v->GetNextStoppingStation().value); // revise getnextstoppingstation could ignore depot orders
		}
	}

	for (const Station *st : Station::Iterate()) {
		if (st->owner != v->owner || !(st->facilities & FACIL_AIRPORT) || !st->airport.HasHangar()) continue;

		/* Planes won't land at any helicopter station. */
		if (!v->IsHelicopter() && st->airport.runways.empty()) continue;

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

ClosestDepot Aircraft::FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse)
{
	const Station *st = GetTargetAirportIfValid(this);
	/* If the station is not a valid airport or if it has no hangars */
	if (st == nullptr || !CanVehicleUseStation(this, st) || !st->airport.HasHangar()) {
		/* the aircraft has to search for a hangar on its own */
		StationID station = FindClosestHangar(this);

		if (station == INVALID_STATION) return ClosestDepot();

		st = Station::Get(station);
		assert(st->airport.depot_id != INVALID_DEPOT);
	}

	assert(st != nullptr);
	return ClosestDepot(st->xy, st->airport.depot_id);
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

	const Station *st = v->GetCurrentAirport();
	//if (st == nullptr || !st->airport.HasHangar()) v->GetTargetAirport();
	if (st == nullptr || !st->airport.HasHangar()) {
		Backup<CompanyID> cur_company(_current_company, v->owner);
		CommandCost ret = Command<CMD_SEND_VEHICLE_TO_DEPOT>::Do(DC_EXEC, v->index, DepotCommand::Service, {});
		cur_company.Restore();

		if (ret.Failed()) CrashAirplane(v);
	} else if (!v->current_order.IsType(OT_GOTO_DEPOT)) {
		v->current_order.Free();
	}
}

/** set the right pos when heading to other airports after takeoff */
void UpdateAircraftState(Aircraft *a)
{
}

/**
 * Controls the movement of an aircraft. This function actually moves the vehicle
 * on the map and takes care of minor things like sound playback.
 * @todo    De-mystify the cur_speed values for helicopter rotors.
 * @param v The vehicle that is moved. Must be the first vehicle of the chain
 * @return  Whether the position requested by the State Machine has been reached
 */
static bool AircraftController(Aircraft *v)
{
	return false;
}

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
		//if (!AircraftEventHandler(this, i)) return false;
	}

	return true;
}
