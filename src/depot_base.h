/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file depot_base.h Base for all depots (except hangars) */

#ifndef DEPOT_BASE_H
#define DEPOT_BASE_H

#include "depot_map.h"
#include "core/pool_type.hpp"
#include "rail_type.h"
#include "road_type.h"

typedef Pool<Depot, DepotID, 64, 64000> DepotPool;
extern DepotPool _depot_pool;

class CommandCost;
struct Vehicle;

struct Depot : DepotPool::PoolItem<&_depot_pool> {
	Town *town;
	std::string name;

	TileIndex xy;
	uint16 town_cn;       ///< The N-1th depot for this town (consecutive number)
	Date build_date;      ///< Date of construction

	CompanyID company;
	VehicleType veh_type;
	byte delete_ctr;      ///< Delete counter. If greater than 0 then it is decremented until it reaches 0; the depot is then deleted.

	union {
		RoadTypes road_types;
		RailTypes rail_types;
	} r_types;

	TileArea ta;
	std::vector<TileIndex> depot_tiles;

	Depot(TileIndex xy = INVALID_TILE, VehicleType type = VEH_INVALID, Owner owner = INVALID_OWNER) :
			xy(xy),
			company(owner),
			veh_type(type),
			ta(xy, 1, 1) {}

	~Depot();

	static inline Depot *GetByTile(TileIndex tile)
	{
		assert(Depot::IsValidID(GetDepotIndex(tile)));
		return Depot::Get(GetDepotIndex(tile));
	}

	TileIndex GetBestDepotTile(Vehicle *v) const;

	/**
	 * Is the "type" of depot the same as the given depot,
	 * i.e. are both a rail, road or ship depots?
	 * @param d The depot to compare to.
	 * @return true iff their types are equal.
	 */
	inline bool IsOfType(const Depot *d) const
	{
		return d->veh_type == this->veh_type;
	}

	/**
	 * Check whether the depot currently is in use; in use means
	 * that it is not scheduled for deletion and that it still has
	 * a building on the map. Otherwise the building is demolished
	 * and the depot awaits to be deleted.
	 * @return true iff still in use
	 * @see Depot::Disuse
	 */
	inline bool IsInUse() const
	{
		return this->delete_ctr == 0;
	}

	void Disuse();

	/* Check we can add some tiles to this depot. */
	CommandCost BeforeAddTiles(TileArea ta);

	/* Add some tiles to this depot and rescan area for depot_tiles. */
	void AfterAddRemove(TileArea ta, bool adding);

	/* Rescan depot_tiles. Done after AfterAddRemove and SaveLoad. */
	void RescanDepotTiles();
};

#endif /* DEPOT_BASE_H */
