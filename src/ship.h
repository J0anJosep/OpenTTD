/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ship.h Base for ships. */

#ifndef SHIP_H
#define SHIP_H

#include "vehicle_base.h"
#include "water_map.h"
#include "core/enum_type.hpp"

/* States ships may have regarding to locks. */
enum ShipLockState {
	SLS_NO_LOCK = 0,     ///< Ship is not at a lock.
	SLS_PREPARING_LOCK,  ///< Ship has asked to enter a lock, but it needs to prepare the middle tile.
	SLS_SHIP_ENTER,      ///< Middle tile is prepared so ship can move through first tile till reach middle tile.
	SLS_SHIP_UPDOWN,     ///< Move ship up/down.
	SLS_SHIP_MOVE,       ///< Move ship till it reaches the third tile of the lock.
	SLS_RESET_OTHER_END, ///< Reset the other end of the lock while ship is leaving.
	SLS_SHIP_LEAVE,      ///< Move ship till it leaves the lock (and prepare first tile of lock again).
	SLS_END
};
typedef TinyEnumT<Track> ShipLockStateByte;

static const uint SHIP_BLOCKED_TICKS = 8;
static const uint SHIP_PREPARE_LOCK_TICKS = 10;
static const uint SHIP_UPDOWN_LOCK_TICKS = 20;

void GetShipSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type);
WaterClass GetEffectiveWaterClass(TileIndex tile);

/**
 * All ships have this type.
 */
struct Ship FINAL : public SpecializedVehicle<Ship, VEH_SHIP> {
	TrackBitsByte state;     ///< The "track" the ship is following.

	bool stuck;              ///< Is this ship waiting to reserve a track?
	ShipLockStateByte lock;  ///< The state of the movement on a lock.

	/** We don't want GCC to zero our struct! It already is zeroed and has an index! */
	Ship() : SpecializedVehicleBase() {}
	/** We want to 'destruct' the right class. */
	virtual ~Ship();

	bool IsStuck() const { return this->stuck; }

	/** Decrease the wait counter and return if ship can try moving again. */
	bool TryUnblock()
	{
		if (this->wait_counter > 0) {
			this->wait_counter--;
			return false;
		}
		return true;
	}

	void MarkShipAsStuck(bool stop, uint ticks);
	void Unstuck();

	void MarkDirty();
	void UpdateDeltaXY(Direction direction);
	ExpensesType GetExpenseType(bool income) const { return income ? EXPENSES_SHIP_INC : EXPENSES_SHIP_RUN; }
	void PlayLeaveStationSound() const;
	bool IsPrimaryVehicle() const { return true; }
	SpriteID GetImage(Direction direction, EngineImageType image_type) const;
	int GetDisplaySpeed() const { return this->cur_speed / 2; }
	int GetDisplayMaxSpeed() const { return this->vcache.cached_max_speed / 2; }
	int GetCurrentMaxSpeed() const { return min(this->vcache.cached_max_speed, this->current_order.GetMaxSpeed() * 2); }
	Money GetRunningCost() const;
	bool IsInDepot() const { return this->state == TRACK_BIT_DEPOT; }
	bool Tick();
	void OnNewDay();
	Trackdir GetVehicleTrackdir() const;
	TileIndex GetOrderStationLocation(StationID station);
	bool FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse);
	void UpdateCache();
};

/**
 * Iterate over all ships.
 * @param var The variable used for iteration.
 */
#define FOR_ALL_SHIPS(var) FOR_ALL_VEHICLES_OF_TYPE(Ship, var)

#endif /* SHIP_H */
