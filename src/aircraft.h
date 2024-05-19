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
	AIR_ROTOR      = 6, ///< rotor of a helicopter
};

/** Flags for air vehicles; shared with disaster vehicles. */
enum AirVehicleFlags : uint8_t {
	VAF_DEST_TOO_FAR             = 0, ///< Next destination is too far away.

	/* The next two flags are to prevent stair climbing of the aircraft. The idea is that the aircraft
	 * will ascend or descend multiple flight levels at a time instead of following the contours of the
	 * landscape at a fixed altitude. This only has effect when there are more than 15 height levels. */
	VAF_IN_MAX_HEIGHT_CORRECTION = 1, ///< The vehicle is currently lowering its altitude because it hit the upper bound.
	VAF_IN_MIN_HEIGHT_CORRECTION = 2, ///< The vehicle is currently raising its altitude because it hit the lower bound.

	VAF_CAN_T_LAND               = 3, ///< The vehicle cannot land on destination airport.
};

static const int ROTOR_Z_OFFSET         = 5;    ///< Z Offset between helicopter- and rotorsprite.

void HandleAircraftEnterHangar(Aircraft *v);
void GetAircraftSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type);
void UpdateAircraftCache(Aircraft *v, bool update_range = false);

void UpdateAircraftLandingTile(Aircraft *v);
void SetAircraftPosition(Aircraft *v, int x, int y, int z);

void GetAircraftFlightLevelBounds(const Vehicle *v, int *min, int *max);
template <class T>
int GetAircraftFlightLevel(T *v, bool takeoff = false);

/** Variables that are cached to improve performance and such. */
struct AircraftCache {
	uint32_t cached_max_range_sqr;   ///< Cached squared maximum range.
	uint16_t cached_max_range;       ///< Cached maximum range.
};

enum AircraftStateBits : uint8_t {
	ASB_FLYING_CRASHING = 3,
	ASB_FLYING_ON_AIRPORT = 4,
	ASB_FREE_FLIGHT = 5,
	ASB_ON_HOLD = 6,
	ASB_NO_HARD_LIMIT_SPEED = 7,
};

/** States of aircraft. */
enum AircraftState : uint8_t {
	AS_FLYING_CRASHING     = 1 << ASB_FLYING_CRASHING,
	AS_FLYING_FREE_FLIGHT  = 1 << ASB_FREE_FLIGHT,
	AS_FLYING_ON_AIRPORT   = 1 << ASB_FLYING_ON_AIRPORT,
	AS_ON_HOLD             = 1 << ASB_ON_HOLD,
	AS_NO_HARD_LIMIT_SPEED = 1 << ASB_NO_HARD_LIMIT_SPEED,

	AS_BEGIN = 0,
	AS_HANGAR = AS_BEGIN,
	AS_IDLE = 1,
	AS_TERMINAL_BEGIN,
	AS_APRON = AS_TERMINAL_BEGIN,
	AS_HELIPAD = 3,
	AS_HELIPORT = 4,
	AS_BUILTIN_HELIPORT = 5,
	AS_TERMINAL_END = AS_BUILTIN_HELIPORT,
	AS_MOVING = 6,
	AS_RUNNING = AS_MOVING,

	AS_START_TAKEOFF = 7,
	AS_TAKEOFF_BEFORE_FLYING = 8,
	AS_LANDED = 9 | AS_NO_HARD_LIMIT_SPEED,

	/* Flying while keeping some reserved track on the airport. */
	AS_FLYING_TAKEOFF = AS_FLYING_ON_AIRPORT,
	AS_FLYING_HELICOPTER_TAKEOFF,
	AS_DESCENDING = AS_FLYING_ON_AIRPORT | AS_NO_HARD_LIMIT_SPEED,
	AS_FLYING_LANDING,
	AS_FLYING_HELICOPTER_LANDING,
	AS_ON_HOLD_APPROACHING = AS_FLYING_ON_AIRPORT | AS_NO_HARD_LIMIT_SPEED | AS_ON_HOLD,

	/* Flying free with no reservation on any airport tile. */
	AS_FLYING = AS_FLYING_FREE_FLIGHT | AS_NO_HARD_LIMIT_SPEED,
	AS_FLYING_FALLING,
	AS_FLYING_NO_DEST,
	AS_FLYING_LEAVING_AIRPORT,
	AS_ON_HOLD_WAITING = AS_FLYING_FREE_FLIGHT | AS_FLYING_ON_AIRPORT | AS_NO_HARD_LIMIT_SPEED | AS_ON_HOLD,

	AS_FLYING_MASK = AS_FLYING_FREE_FLIGHT | AS_FLYING_ON_AIRPORT,

	INVALID_AS = 0xFF,

	/* Helicopter rotor animation states. */
	HRS_ROTOR_STOPPED  = 0,
	HRS_ROTOR_MOVING_1 = 1,
	HRS_ROTOR_MOVING_2 = 2,
	HRS_ROTOR_MOVING_3 = 3,
	HRS_ROTOR_NUM_STATES = 3,
};
DECLARE_ENUM_AS_ADDABLE(AircraftState)

inline bool IsTerminalState(AircraftState as)
{
	return as >= AS_TERMINAL_BEGIN && as <= AS_TERMINAL_END;
}

enum AircraftPos : uint8_t {
	AP_BEGIN = 0,
	AP_DEFAULT = AP_BEGIN,
	AP_HELICOPTER_HOLD_START,
	AP_HELICOPTER_HOLD_2,
	AP_HELICOPTER_HOLD_3,
	AP_HELICOPTER_HOLD_4,
	AP_HELICOPTER_HOLD_5,
	AP_HELICOPTER_HOLD_6,
	AP_HELICOPTER_HOLD_7,
	AP_HELICOPTER_HOLD_END,
	AP_HELIPORT_DEST,
	AP_BUILTIN_HELIPORT_DEST,
	AP_START_TAKE_OFF,
	AP_PLANE_BEFORE_FLYING,
	AP_PLANE_START_FLYING,
	AP_PLANE_LEAVE_AIRPORT,
	AP_PLANE_HOLD_START,
	AP_PLANE_HOLD_2,
	AP_PLANE_HOLD_3,
	AP_PLANE_HOLD_4,
	AP_PLANE_HOLD_5,
	AP_PLANE_HOLD_6,
	AP_PLANE_HOLD_7,
	AP_PLANE_HOLD_8,
	AP_PLANE_HOLD_END,
	AP_PLANE_DESCENDING,
	AP_PLANE_LANDING,

	AP_END
};
DECLARE_ENUM_AS_ADDABLE(AircraftPos)

/**
 * Struct that contains the offsets in x and y of a position
 * that an aircraft must reach calculated from its destination tile.
 */
struct AircraftPosition {
	AircraftPos pos;
	int x;
	int y;
};

struct AircraftPathChoice {
	std::deque<Trackdir> td;
	std::deque<TileIndex> tile;  ///< Kept for debugging purposes. Should be removed in the future.

	inline bool empty() const { return this->td.empty(); }

	inline size_t size() const
	{
		assert(this->td.size() == this->tile.size());
		return this->td.size();
	}

	inline void clear()
	{
		this->td.clear();
		this->tile.clear();
	}

	inline void pop_front()
	{
		assert(!this->empty());
		this->td.pop_front();
		this->tile.pop_front();
	}
};

/**
 * Aircraft, helicopters, rotors and their shadows belong to this class.
 */
struct Aircraft final : public SpecializedVehicle<Aircraft, VEH_AIRCRAFT> {
	AircraftPathChoice path;       ///< Cached path choices
	uint16_t crashed_counter;      ///< Timer for handling crash animations.
	Trackdir trackdir;             ///< Current trackdir while aircraft is on land.
	AircraftState state;           ///< Current aircraft state. @see AircraftState
	StationID targetairport;       ///< Airport to go to next.

	Trackdir next_trackdir;        ///< Desired trackdir when rotating at airport, or entry trackdir to an airport while flying.
	AircraftPosition next_pos;     ///< next x_pos and y_pos coordinate.

	Direction last_direction;
	uint8_t number_consecutive_turns; ///< Protection to prevent the aircraft of making a lot of turns in order to reach a specific point.
	uint8_t turn_counter;             ///< Ticks between each turn to prevent > 45 degree turns.
	uint8_t flags;                    ///< Aircraft flags. @see AirVehicleFlags

	AircraftCache acache;

	/** We don't want GCC to zero our struct! It already is zeroed and has an index! */
	Aircraft() : SpecializedVehicleBase() {}
	/** We want to 'destruct' the right class. */
	virtual ~Aircraft() { this->PreDestructor(); }

	void MarkDirty() override;
	void UpdateDeltaXY() override;
	ExpensesType GetExpenseType(bool income) const override { return income ? EXPENSES_AIRCRAFT_REVENUE : EXPENSES_AIRCRAFT_RUN; }
	bool IsPrimaryVehicle() const override                  { return this->IsNormalAircraft(); }
	void GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const override;
	int GetDisplaySpeed() const override    { return this->cur_speed; }
	int GetDisplayMaxSpeed() const override { return this->vcache.cached_max_speed; }
	int GetSpeedOldUnits() const            { return this->vcache.cached_max_speed * 10 / 128; }
	int GetCurrentMaxSpeed() const override { return this->GetSpeedOldUnits(); }
	Money GetRunningCost() const override;

	bool IsInDepot() const override
	{
		assert(this->IsPrimaryVehicle());
		return this->state == AS_HANGAR;
	}

	Trackdir GetVehicleTrackdir() const override
	{
		assert(this->IsPrimaryVehicle());
		return this->trackdir;
	}

	TileIndex GetNextTile() const
	{
		assert(this->IsPrimaryVehicle());
		return this->Next()->dest_tile;
	}

	void UpdateNextTile(TileIndex tile);
	void SetDestTile(TileIndex tile) override;

	bool Tick() override;
	void OnNewCalendarDay() override;
	void OnNewEconomyDay() override;
	uint Crash(bool flooded = false) override;
	TileIndex GetOrderStationLocation(StationID station) override;
	TileIndex GetCargoTile() const override { return this->First()->tile; }
	ClosestDepot FindClosestDepot() override;

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
	uint16_t GetRange() const
	{
		return this->acache.cached_max_range;
	}

	/**
	 * Check whether the vehicle is flying.
	 * @return True if the vehicle is currently flying: from taking off until landing.
	 */
	bool IsAircraftFlying() const
	{
		assert(this->IsNormalAircraft());
		return (this->state & AS_FLYING_MASK) != 0;
	}

	/**
	 * Check whether the vehicle is flying and has no reserved tile on any airport.
	 * @return True if the vehicle is currently freely flying.
	 */
	bool IsAircraftFreelyFlying() const
	{
		assert(this->IsNormalAircraft());
		return HasBit(this->state, ASB_FREE_FLIGHT);
	}

	/**
	 * Check whether the vehicle is flying and falling, about to crash.
	 * @return True if the vehicle is currently flying and falling.
	 */
	bool IsAircraftFalling() const
	{
		assert(this->IsNormalAircraft());
		return (this->state == AS_FLYING_FALLING);
	}

	/**
	 * Check whether the vehicle is flying rotating around its destination.
	 * @return True if the vehicle is currently flying around its destination.
	 */
	bool IsAircraftOnHold() const
	{
		assert(this->IsNormalAircraft());
		return HasBit(this->state, ASB_ON_HOLD);
	}

	void SetWaitTime(uint16_t wait_counter)
	{
		this->wait_counter = wait_counter;
	}

	void ClearWaitTime()
	{
		this->SetWaitTime(0);
	}

	bool IsWaiting() const
	{
		return this->wait_counter > 0;
	}

	void AdvanceWaitTime()
	{
		assert (this->IsWaiting());
		this->wait_counter--;
	}
};

void GetRotorImage(const Aircraft *v, EngineImageType image_type, VehicleSpriteSeq *result);

Station *GetTargetAirportIfValid(const Aircraft *v);
void HandleMissingAircraftOrders(Aircraft *v);

#endif /* AIRCRAFT_H */
