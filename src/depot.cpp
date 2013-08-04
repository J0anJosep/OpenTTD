/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file depot.cpp Handling of depots. */

#include "stdafx.h"
#include "depot_base.h"
#include "order_backup.h"
#include "order_func.h"
#include "window_func.h"
#include "core/pool_func.hpp"
#include "vehicle_gui.h"
#include "vehiclelist.h"
#include "station_base.h" // revise: included for aircraft; this could change

#include "safeguards.h"

/** All our depots tucked away in a pool. */
DepotPool _depot_pool("Depot");
INSTANTIATE_POOL_METHODS(Depot)

/**
 * Clean up a depot
 */
Depot::~Depot()
{
	if (CleaningPool()) return;

	/* Clear the order backup. */
	OrderBackup::Reset(this->index, false);

	/* Clear the depot from all order-lists */
	RemoveOrderFromAllVehicles(OT_GOTO_DEPOT, this->index);

	/* Delete the depot-window */
	DeleteWindowById(WC_VEHICLE_DEPOT, this->index);

	/* Delete the depot list */
	VehicleType vt = GetDepotVehicleType(this->xy);
	DeleteWindowById(GetWindowClassForVehicleType(vt), VehicleListIdentifier(VL_DEPOT_LIST, vt, GetTileOwner(this->xy), this->index).Pack());
}

/* Check we can add some tiles to this depot. */
bool Depot::BeforeAddTiles(TileArea ta)
{
	assert(ta.tile != INVALID_TILE);

	if (this->ta.tile != INVALID_TILE) {
		/* Important when the old rect is completely inside the new rect, resp. the old one was empty. */
		ta.Add(this->ta.tile);
		ta.Add(TILE_ADDXY(this->ta.tile, this->ta.w - 1, this->ta.h - 1));
	}

	return (ta.w <= _settings_game.station.station_spread) && (ta.h <= _settings_game.station.station_spread);
}

/* Add some tiles to this depot and rescan area for depot_tiles. */
void Depot::AfterAddRemove(TileArea ta, bool adding)
{
	assert(ta.tile != INVALID_TILE);

	if (adding) {
		if (this->ta.tile != INVALID_TILE) {
			ta.Add(this->ta.tile);
			ta.Add(TILE_ADDXY(this->ta.tile, this->ta.w - 1, this->ta.h - 1));
		}
	} else { // removing
		ta.tile = this->ta.tile;
		ta.h = this->ta.h;
		ta.w = this->ta.w;
	}

	this->ta.Clear(); // clear tilearea

	TILE_AREA_LOOP(tile, ta) {
		if (!IsDepotTile(tile)) continue;
		if (GetDepotIndex(tile) != this->index) continue;
		this->ta.Add(tile);
	}

	if (this->ta.tile != INVALID_TILE) {
		this->RescanDepotTiles();
		assert(this->depot_tiles.Length() > 0);
		this->xy = this->depot_tiles[0];
		assert(IsDepotTile(this->xy));
	} else {
		delete this;
	}
}

bool IsDepotDestTile(Depot *dep, TileIndex tile)
{
	switch (dep->veh_type) {
		case VEH_TRAIN:
			return true; // revise: only start and end of platform.
		case VEH_ROAD:
		case VEH_SHIP: // Both ends are considered.
		case VEH_AIRCRAFT:
			return true;
		default: NOT_REACHED();
	}
}

/* Rescan depot_tiles. Done after AfterAddRemove and SaveLoad. */
void Depot::RescanDepotTiles()
{
	this->depot_tiles.Clear();
	RailTypes old_types = this->r_types.rail_types;
	this->r_types.rail_types = (RailTypes)0;

	TILE_AREA_LOOP(tile, this->ta) {
		if (!IsDepotTile(tile)) continue;
		if (GetDepotIndex(tile) != this->index) continue;
		if (IsDepotDestTile(this, tile)) *(this->depot_tiles.Append()) = tile;
		switch (veh_type) {
			case VEH_ROAD:
				this->r_types.road_types |= GetRoadTypes(tile);
				break;
			case VEH_TRAIN:
				this->r_types.rail_types |= (RailTypes)(1 << GetRailType(tile));
			default: break;
		}
	}

	if (old_types != this->r_types.rail_types) InvalidateWindowData(WC_BUILD_VEHICLE, this->index, 0, true);
}
