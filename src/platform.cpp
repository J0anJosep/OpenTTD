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

void SetPlatformReservation(TileIndex start, DiagDirection dir, bool b)
{
	switch (GetPlatformType(start)) {
		case PT_RAIL_STATION:
			SetRailStationPlatformReservation(start, dir, b);
			return;
		case PT_RAIL_WAYPOINT:
			SetRailStationReservation(start, b);
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

uint GetPlatformLength(TileIndex tile)
{
	switch (GetPlatformType(tile)) {
		case PT_RAIL_STATION:
			return GetRailStationPlatformLength(tile);
		case PT_RAIL_WAYPOINT:
			return 1;
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
		default: NOT_REACHED();
	}
}
