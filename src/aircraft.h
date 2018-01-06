/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file aircraft.h Base for aircraft. */

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "station_map.h"
#include "vehicle_base.h"

/**
 * Base values for flight levels above ground level for 'normal' flight and holding patterns.
 * Due to speed and direction, the actual flight level may be higher.
 */
enum AircraftFlyingAltitude {
	AIRCRAFT_MIN_FLYING_ALTITUDE        = 120, ///< Minimum flying altitude above tile.
	AIRCRAFT_MAX_FLYING_ALTITUDE        = 360, ///< Maximum flying altitude above tile.
	PLANE_HOLD_MAX_FLYING_ALTITUDE      = 150, ///< holding flying altitude above tile of planes.
	HELICOPTER_HOLD_MAX_FLYING_ALTITUDE = 184  ///< holding flying altitude above tile of helicopters.
};

struct Aircraft;

/** An aircraft can be one of those types. */
enum AircraftSubType {
	AIR_HELICOPTER = 0, ///< an helicopter
	AIR_AIRCRAFT   = 2, ///< an airplane
	AIR_SHADOW     = 4, ///< shadow of the aircraft
	AIR_ROTOR      = 6, ///< rotor of an helicopter
};

/** Flags for air vehicles; shared with disaster vehicles. */
enum AirVehicleFlags {
	VAF_DEST_TOO_FAR             = 0, ///< Next destination is too far away.

	/* The next two flags are to prevent stair climbing of the aircraft. The idea is that the aircraft
	 * will ascend or descend multiple flight levels at a time instead of following the contours of the
	 * landscape at a fixed altitude. This only has effect when there are more than 15 height levels. */
	VAF_IN_MAX_HEIGHT_CORRECTION = 1, ///< The vehicle is currently lowering its altitude because it hit the upper bound.
	VAF_IN_MIN_HEIGHT_CORRECTION = 2, ///< The vehicle is currently raising its altitude because it hit the lower bound.
};

static const int ROTOR_Z_OFFSET         = 5;    ///< Z Offset between helicopter- and rotorsprite.

void HandleAircraftEnterHangar(Aircraft *v);
void GetAircraftSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type);
void UpdateAirplanesOnNewStation(const Station *st);
void UpdateAircraftCache(Aircraft *v, bool update_range = false);

void AircraftLeaveHangar(Aircraft *v, Direction exit_dir);
void AircraftNextAirportPos_and_Order(Aircraft *v);
void SetAircraftPosition(Aircraft *v, int x, int y, int z);

void GetAircraftFlightLevelBounds(const Vehicle *v, int *min, int *max);
template <class T>
int GetAircraftFlightLevel(T *v, bool takeoff = false);

/** Variables that are cached to improve performance and such. */
struct AircraftCache {
	uint32 cached_max_range_sqr;   ///< Cached squared maximum range.
	uint16 cached_max_range;       ///< Cached maximum range.
};

/** Movements types an aircraft can do. */
enum AircraftMovement {
	AM_BEGIN = 0,
	AM_IDLE = 0,
	AM_HANGAR,
	AM_TERMINAL,
	AM_HELIPAD,
	AM_MOVING,
	AM_MOVE = AM_MOVING,

	AM_TAKEOFF,
	AM_HELICOPTER_TAKEOFF,
	AM_FLYING,
	AM_LANDING,
	AM_HELICOPTER_LANDING,

	AM_END,
	INVALID_AM = 0xFF,

	/* Helicopter rotor animation states. */
	HRS_ROTOR_STOPPED  = 0,
	HRS_ROTOR_MOVING_1 = 1,
	HRS_ROTOR_MOVING_2 = 2,
	HRS_ROTOR_MOVING_3 = 3,
};

/** Define basic enum properties */
template <> struct EnumPropsT<AircraftMovement> : MakeEnumPropsT<AircraftMovement, byte, AM_BEGIN, AM_END, INVALID_AM> {};
typedef TinyEnumT<AircraftMovement> AircraftMovementByte;

/**
 * Aircraft, helicopters, rotors and their shadows belong to this class.
 */
struct Aircraft FINAL : public SpecializedVehicle<Aircraft, VEH_AIRCRAFT> {
	TrackdirByte trackdir;             ///< The track direction the aircraft is following.
	TrackdirByte desired_trackdir;     ///< The track direction the aircraft should rotate to.
	AircraftMovementByte cur_state;    ///< The type of movement the aircraft is doing. @see AircraftMovement revise replace state

	TileIndex next_tile;               ///< The tile that must be reached while the aircraft is moving.
	TrackdirByte next_trackdir;        ///< The track direction the aircraft must follow when reaching next destination.
	AircraftMovementByte next_state;   ///< The type of movement the aircraft is trying to get when reaching next destination.

	StationID targetairport;           ///< Airport to go to next.
	AircraftMovementByte target_state; ///< What to do when reaching the station.
	uint16 crashed_counter;            ///< Timer for handling crash animations.
	DirectionByte last_direction;
	byte number_consecutive_turns;     ///< Protection to prevent the aircraft of making a lot of turns in order to reach a specific point.
	byte turn_counter;                 ///< Ticks between each turn to prevent > 45 degree turns.
	byte flags;                        ///< Aircraft flags. @see AirVehicleFlags

	AircraftCache acache;

	/** We don't want GCC to zero our struct! It already is zeroed and has an index! */
	Aircraft() : SpecializedVehicleBase() {}
	/** We want to 'destruct' the right class. */
	virtual ~Aircraft() { this->PreDestructor(); }

	void MarkDirty();
	void UpdateDeltaXY(Direction direction);
	ExpensesType GetExpenseType(bool income) const { return income ? EXPENSES_AIRCRAFT_INC : EXPENSES_AIRCRAFT_RUN; }
	bool IsPrimaryVehicle() const                  { return this->IsNormalAircraft(); }
	void GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const;
	int GetDisplaySpeed() const    { return this->cur_speed; }
	int GetDisplayMaxSpeed() const { return this->vcache.cached_max_speed; }
	int GetSpeedOldUnits() const   { return this->vcache.cached_max_speed * 10 / 128; }
	int GetCurrentMaxSpeed() const { return this->GetSpeedOldUnits(); }
	Trackdir GetVehicleTrackdir() const { return this->trackdir; }
	Money GetRunningCost() const;

	bool IsInDepot() const
	{
		assert(this->IsPrimaryVehicle());
		return (this->vehstatus & VS_HIDDEN) != 0 && IsHangarTile(this->tile);
	}

	bool Tick();
	void OnNewDay();
	uint Crash(bool flooded = false);
	TileIndex GetOrderStationLocation(StationID station);
	bool FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse);

	/**
	 * Check if the aircraft type is a normal flying device; eg
	 * not a rotor or a shadow
	 * @return Returns true if the aircraft is a helicopter/airplane and
	 * false if it is a shadow or a rotor
	 */
	inline bool IsNormalAircraft() const
	{
		/* To be fully correct the commented out functionality is the proper one,
		 * but since value can only be 0 or 2, it is sufficient to only check <= 2
		 * return (this->subtype == AIR_HELICOPTER) || (this->subtype == AIR_AIRCRAFT); */
		return this->subtype <= AIR_AIRCRAFT;
	}

	inline bool IsHelicopter() const
	{
		return this->subtype == AIR_HELICOPTER;
	}

	/**
	 * Get the range of this aircraft.
	 * @return Range in tiles or 0 if unlimited range.
	 */
	uint16 GetRange() const
	{
		return this->acache.cached_max_range;
	}
};

/**
 * Macro for iterating over all aircraft.
 */
#define FOR_ALL_AIRCRAFT(var) FOR_ALL_VEHICLES_OF_TYPE(Aircraft, var)

void GetRotorImage(const Aircraft *v, EngineImageType image_type, VehicleSpriteSeq *result);

Station *GetTargetAirportIfValid(const Aircraft *v);

#endif /* AIRCRAFT_H */
