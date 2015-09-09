/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file platform.cpp Implementation of platform functions. */

#include "stdafx.h"
#include "station_map.h"
#include "platform_func.h"
#include "viewport_func.h"
#include "depot_base.h"
#include "vehicle_base.h"
#include "engine_base.h"

/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(start)
 * @param start starting tile of the platform
 * @param dir the direction in which to follow the platform
 * @param b the state the reservation should be set to
 */
void SetRailStationPlatformReservation(TileIndex start, DiagDirection dir, bool b)
{
	TileIndex     tile = start;
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	assert(IsRailStationTile(start));
	assert(GetRailStationAxis(start) == DiagDirToAxis(dir));

	do {
		SetRailStationReservation(tile, b);
		MarkTileDirtyByTile(tile);
		tile = TILE_ADD(tile, diff);
	} while (IsCompatibleTrainStationTile(tile, start));
}


/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(start)
 * @param start starting tile of the platform
 * @param dir the direction in which to follow the platform
 * @param b the state the reservation should be set to
 */
void SetRailDepotPlatformReservation(TileIndex start, DiagDirection dir, bool b)
{
	TileIndex     tile = start;
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	assert(IsBigRailDepotTile(start));
	assert(GetRailDepotTrack(start) == DiagDirToDiagTrack(dir));

	do {
		SetDepotReservation(tile, b);
		MarkTileDirtyByTile(tile);
		tile = TILE_ADD(tile, diff);
	} while (IsCompatibleTrainDepotTile(tile, start));
}

/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(start)
 * @param start starting tile of the platform
 * @param dir the direction in which to follow the platform
 * @param b the state the reservation should be set to
 */
void SetRunwayReservation(TileIndex tile, bool b)
{
	assert(IsRunwayExtreme(tile));
	DiagDirection dir = GetRunwayDirection(tile);
	if (IsRunwayEnd(tile)) dir = ReverseDiagDir(dir);
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	do {
		assert(!HasAirportTrackReserved(tile));
		SetReservationAsRunway(tile, b);
		MarkTileDirtyByTile(tile);
		tile = TILE_ADD(tile, diff);
	} while (!IsRunwayExtreme(tile));

	SetReservationAsRunway(tile, b);
	MarkTileDirtyByTile(tile);
}

void SetPlatformReservation(TileIndex start, DiagDirection dir, bool b)
{
	switch (GetPlatformType(start)) {
		case PT_RAIL_STATION:
			SetRailStationPlatformReservation(start, dir, b);
			return;
		case PT_RAIL_WAYPOINT:
			SetRailStationReservation(start, b);
			return;
		case PT_RAIL_DEPOT:
			SetRailDepotPlatformReservation(start, dir, b);
			return;
		case PT_RUNWAY:
			SetRunwayReservation(start, b);
			return;
		default: NOT_REACHED();
	}
}

uint GetRailStationPlatformLength(TileIndex tile)
{
	assert(IsRailStationTile(tile));

	TileIndexDiff delta = (GetRailStationAxis(tile) == AXIS_X ? TileDiffXY(1, 0) : TileDiffXY(0, 1));

	TileIndex t = tile;
	uint len = 0;
	do {
		t -= delta;
		len++;
	} while (IsCompatibleTrainStationTile(t, tile));

	t = tile;
	do {
		t += delta;
		len++;
	} while (IsCompatibleTrainStationTile(t, tile));

	return len - 1;
}

uint GetRailStationPlatformLength(TileIndex tile, DiagDirection dir)
{
	TileIndex start_tile = tile;
	uint length = 0;
	assert(IsRailStationTile(tile));
	assert(dir < DIAGDIR_END);

	do {
		length++;
		tile += TileOffsByDiagDir(dir);
	} while (IsCompatibleTrainStationTile(tile, start_tile));

	return length;
}

uint GetRailDepotPlatformLength(TileIndex tile)
{
	assert(IsDepotTypeTile(tile, TRANSPORT_RAIL));

	TileIndexDiff delta = (GetRailDepotTrack(tile) == TRACK_X ? TileDiffXY(1, 0) : TileDiffXY(0, 1));

	TileIndex t = tile;
	uint len = 0;
	do {
		t -= delta;
		len++;
	} while (IsCompatibleTrainDepotTile(t, tile));

	t = tile;
	do {
		t += delta;
		len++;
	} while (IsCompatibleTrainDepotTile(t, tile));

	return len - 1;
}

uint GetRailDepotPlatformLength(TileIndex tile, DiagDirection dir)
{
	TileIndex start_tile = tile;
	uint length = 0;
	assert(IsRailDepotTile(tile));
	assert(dir < DIAGDIR_END);

	do {
		length++;
		tile += TileOffsByDiagDir(dir);
	} while (IsCompatibleTrainDepotTile(tile, start_tile));

	return length;
}

uint GetRunwayLength(TileIndex tile)
{
	TileIndex start_tile = tile;
	uint length = 0;
	assert(IsAirport(tile) && IsRunwayStart(tile));
	DiagDirection dir = GetRunwayDirection(tile);
	assert(dir < DIAGDIR_END);

	do {
		length++;
		tile += TileOffsByDiagDir(dir);
	} while (IsCompatibleRunwayTile(tile, start_tile) && !IsRunwayEnd(tile));

	assert(IsRunwayEnd(tile));

	return length;
}

uint GetPlatformLength(TileIndex tile)
{
	switch (GetPlatformType(tile)) {
		case PT_RAIL_STATION:
			return GetRailStationPlatformLength(tile);
		case PT_RAIL_WAYPOINT:
			return 1;
		case PT_RAIL_DEPOT:
			return GetRailDepotPlatformLength(tile);
		case PT_RUNWAY:
			return GetRunwayLength(tile);
		default: NOT_REACHED();
	}
}

uint GetPlatformLength(TileIndex tile, DiagDirection dir)
{
	switch (GetPlatformType(tile)) {
		case PT_RAIL_STATION:
			return GetRailStationPlatformLength(tile, dir);
		case PT_RAIL_WAYPOINT:
			return 1;
		case PT_RAIL_DEPOT:
			return GetRailDepotPlatformLength(tile, dir);
		default: NOT_REACHED();
	}
}

TileIndex GetRailDepotExtreme(TileIndex tile, bool start)
{
	assert(IsBigDepotTile(tile));
	TileIndexDiff delta = (GetRailDepotTrack(tile) == TRACK_X ? TileDiffXY(start ? 1 : -1, 0) : TileDiffXY(0, start ? 1 : -1));

	TileIndex t = tile;
	uint len = 0;
	do {
		t -= delta;
		len++;
	} while (IsCompatibleTrainDepotTile(t, tile));

	return t + delta;
}

bool IsWestern(TileIndex tile, DiagDirection *dir)
{
	assert(IsRunwayExtreme(tile));
	*dir = GetRunwayDirection(tile);
	if (IsRunwayEnd(tile))
		return *dir == DIAGDIR_NW || *dir == DIAGDIR_SW;
	return *dir == DIAGDIR_NE || *dir == DIAGDIR_SE;
}

/**
 * Get the start or end of a runway.
 * @param tile Tile belonging a runway.
 * @param end If true, it will return the end tile of the runway.
 *                 If false, it will return the start tile of the runway.
 */
TileIndex GetRunwayExtreme(TileIndex tile, bool end)
{
	assert(IsAirportTile(tile) && IsRunway(tile));
	DiagDirection dir;
	if (IsRunwayExtreme(tile)) {
		if (IsRunwayEnd(tile) == end) return tile;
		dir = GetRunwayDirection(tile);
		if (!end) dir = ReverseDiagDir(dir);
	} else {
		assert(IsPlainRunway(tile));
		dir = ((GetRunwayTracks(tile) & TRACK_X) != 0 ? DIAGDIR_SE : DIAGDIR_NE);
	}

	for (;;) {
		tile = TileAddByDiagDir(tile, dir);
		assert(IsRunway(tile));
		if (IsRunwayExtreme(tile)) {
			if (IsRunwayEnd(tile) == end) return tile;
			dir = ReverseDiagDir(dir);
		}
	}
}

TileIndex GetStartPlatformTile(TileIndex tile)
{
	switch (GetPlatformType(tile)) {
		case PT_RAIL_STATION:
			NOT_REACHED();
		case PT_RAIL_WAYPOINT:
			NOT_REACHED();
		case PT_RAIL_DEPOT:
			return GetRailDepotExtreme(tile, true);
		case PT_RUNWAY:
			return GetRunwayExtreme(tile, false);
		default: NOT_REACHED();
	}
}

TileIndex GetOtherStartPlatformTile(TileIndex tile)
{
	switch (GetPlatformType(tile)) {
		case PT_RAIL_STATION:
			NOT_REACHED();
		case PT_RAIL_WAYPOINT:
			NOT_REACHED();
		case PT_RAIL_DEPOT:
			return GetRailDepotExtreme(tile, false);
		case PT_RUNWAY:
			return GetRunwayExtreme(tile, true);
		default: NOT_REACHED();
	}
}

bool IsStartPlatformTile(TileIndex tile)
{
	return tile == GetStartPlatformTile(tile) || tile == GetOtherStartPlatformTile(tile);
}

DiagDirection GetPlatformDirection(TileIndex tile)
{
	assert(IsStartPlatformTile(tile));
	DiagDirection dir = GetRailDepotDirection(tile);
	if (!IsCompatibleTrainDepotTile(TileAddByDiagDir(tile, dir), tile)) {
		dir = ReverseDiagDir(dir);
	}

	return dir;
}

bool IsVehicleCompatibleWithPlatform(TileIndex t, Vehicle *v)
{
	for (Vehicle *u = v; u != NULL; u = u->Next()) {
		RailType rail_type = Engine::Get(u->engine_type)->u.rail.railtype;
		if (!IsCompatibleRail(rail_type, GetRailType(t))) return false;
	}

	return true;
}

TileIndex FindBestPlatform(TileIndex tile, Vehicle *v)
{
	assert(IsBigDepotTile(tile));

	Depot *dep = Depot::Get(GetDepotIndex(tile));
	assert(dep != NULL);
	TileIndex best_tile = INVALID_TILE;
	uint best_length = UINT_MAX;

	for (uint i = dep->depot_tiles.Length(); i--;) {
		if (!IsVehicleCompatibleWithPlatform(dep->depot_tiles[i], v)) continue;
		//if (!Free) continue;

		uint new_length = GetPlatformLength(dep->depot_tiles[i]);
		if (new_length < best_length) {
			best_length = new_length;
			best_tile = dep->depot_tiles[i];
		}
	}

	return best_tile;
}
