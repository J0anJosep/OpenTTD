/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file platform_func.h Functions related with platforms (tiles in a row that are connected somehow). */

#ifndef PLATFORM_FUNC_H
#define PLATFORM_FUNC_H

#include "station_map.h"
#include "depot_map.h"
#include "platform_type.h"

/**
 * Check if a tile is a valid continuation to a railstation tile.
 * The tile \a test_tile is a valid continuation to \a station_tile, if all of the following are true:
 * \li \a test_tile is a rail station tile
 * \li the railtype of \a test_tile is compatible with the railtype of \a station_tile
 * \li the tracks on \a test_tile and \a station_tile are in the same direction
 * \li both tiles belong to the same station
 * \li \a test_tile is not blocked (@see IsStationTileBlocked)
 * @param test_tile Tile to test
 * @param station_tile Station tile to compare with
 * @pre IsRailStationTile(station_tile)
 * @return true if the two tiles are compatible
 */
static inline bool IsCompatibleTrainStationTile(TileIndex test_tile, TileIndex station_tile)
{
	assert(IsRailStationTile(station_tile));
	return IsRailStationTile(test_tile) && IsCompatibleRail(GetRailType(test_tile), GetRailType(station_tile)) &&
			GetRailStationAxis(test_tile) == GetRailStationAxis(station_tile) &&
			GetStationIndex(test_tile) == GetStationIndex(station_tile) &&
			!IsStationTileBlocked(test_tile);
}

/**
 * Check if a tile is a valid continuation to a big rail depot tile.
 * The tile \a test_tile is a valid continuation to \a depot_tile, if all of the following are true:
 * \li \a test_tile is a big depot tile
 * \li \a test_tile and \a depot_tile have the same rail type
 * \li the tracks on \a test_tile and \a depot_tile are in the same direction
 * \li both tiles belong to the same depot
 * @param test_tile Tile to test
 * @param depot_tile Depot tile to compare with
 * @pre IsBigRailDepotTile(depot_tile)
 * @return true if the two tiles are compatible
 */
static inline bool IsCompatibleTrainDepotTile(TileIndex test_tile, TileIndex depot_tile)
{
	assert(IsBigRailDepotTile(depot_tile));
	return IsRailDepotTile(test_tile) &&
			GetRailType(test_tile) == GetRailType(depot_tile) &&
			GetRailDepotTrack(test_tile) == GetRailDepotTrack(depot_tile) &&
			GetDepotIndex(test_tile) == GetDepotIndex(depot_tile);
}

/**
 * Check if a tile is a valid continuation of a runway.
 * The tile \a test_tile is a valid continuation to \a runway, if all of the following are true:
 * \li \a test_tile is an airport tile
 * \li \a test_tile and \a start_tile are in the same station
 * \li the tracks on \a test_tile and \a start_tile are in the same direction
 * @param test_tile Tile to test
 * @param start_tile Depot tile to compare with
 * @pre IsAirport && IsRunwayStart(start_tile)
 * @return true if the two tiles are compatible
 */
static inline bool IsCompatibleRunwayTile(TileIndex test_tile, TileIndex start_tile)
{
	assert(IsAirportTile(start_tile) && IsRunwayStart(start_tile));
	return IsAirportTile(test_tile) &&
			GetStationIndex(test_tile) == GetStationIndex(start_tile) &&
			(GetRunwayTracks(start_tile) & GetRunwayTracks(test_tile)) != TRACK_BIT_NONE;
}

static inline PlatformType GetPlatformType(TileIndex tile)
{
	switch (GetTileType(tile)) {
		case MP_STATION:
			if (IsRailStation(tile)) return PT_RAIL_STATION;
			if (IsRailWaypoint(tile)) return PT_RAIL_WAYPOINT;
			if (IsAirport(tile) && IsRunway(tile)) return PT_RUNWAY;
			break;
		case MP_RAILWAY:
			if (IsBigRailDepotTile(tile)) return PT_RAIL_DEPOT;
		default: break;
	}

	return INVALID_PLATFORM_TYPE;
}

static inline bool IsPlatformTile(TileIndex tile)
{
	return GetPlatformType(tile) != INVALID_PLATFORM_TYPE;
}

static inline bool HasPlatformReservation(TileIndex tile)
{
	switch(GetPlatformType(tile)) {
		case PT_RAIL_STATION:
		case PT_RAIL_WAYPOINT:
			return HasStationReservation(tile);
		case PT_RAIL_DEPOT:
			return HasDepotReservation(tile);
		default: NOT_REACHED();
	}
}

static inline bool IsCompatiblePlatformTile(TileIndex test_tile, TileIndex orig_tile)
{
	switch (GetPlatformType(orig_tile)) {
		case PT_RAIL_STATION:
			return IsCompatibleTrainStationTile(test_tile, orig_tile);
		case PT_RAIL_WAYPOINT:
			return test_tile == orig_tile;
		case PT_RAIL_DEPOT:
			return IsCompatibleTrainDepotTile(test_tile, orig_tile);
		default: NOT_REACHED();
	}
}

void SetPlatformReservation(TileIndex start, DiagDirection dir, bool b);

uint GetPlatformLength(TileIndex tile);
uint GetPlatformLength(TileIndex tile, DiagDirection dir);

TileIndex GetStartPlatformTile(TileIndex tile);
TileIndex GetOtherStartPlatformTile(TileIndex tile);

bool IsStartPlatformTile(TileIndex tile);

DiagDirection GetPlatformDirection(TileIndex tile);
struct Vehicle;
TileIndex FindBestPlatform(TileIndex tile, Vehicle *v);

#endif /* PLATFORM_FUNC_H */
