/* $Id$ */

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
#include "tile_list.h"
#include "rail_type.h"
#include "road_type.h"

typedef Pool<Depot, DepotID, 64, 64000> DepotPool;
extern DepotPool _depot_pool;

struct Vehicle;

struct Depot : DepotPool::PoolItem<&_depot_pool> {
	Town *town;
	char *name;

	TileIndex xy;
	uint16 town_cn;    ///< The N-1th depot for this town (consecutive number)
	Date build_date;   ///< Date of construction

	bool is_big_depot;
	CompanyByte company;
	VehicleTypeByte veh_type;

	union {
		RoadTypes road_types;
		RailTypes rail_types;
	} r_types;

	TileArea ta;
	TileIDVector depot_tiles;

	Depot(TileIndex xy = INVALID_TILE, bool big = false) : xy(xy), is_big_depot(big), ta(xy, 1, 1) {}

	~Depot();

	void SetCompanyAndType(CompanyID company, VehicleType veh_type) {
		this->company = company;
		this->veh_type = veh_type;
	}

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
		return GetTileType(d->xy) == GetTileType(this->xy);
	}

	/* Check we can add some tiles to this depot. */
	bool BeforeAddTiles(TileArea ta);

	/* Add some tiles to this depot and rescan area for depot_tiles. */
	void AfterAddRemove(TileArea ta, bool adding);

	/* Rescan depot_tiles. Done after AfterAddRemove and SaveLoad. */
	void RescanDepotTiles();
};

#define FOR_ALL_DEPOTS_FROM(var, start) FOR_ALL_ITEMS_FROM(Depot, depot_index, var, start)
#define FOR_ALL_DEPOTS(var) FOR_ALL_DEPOTS_FROM(var, 0)

#endif /* DEPOT_BASE_H */
