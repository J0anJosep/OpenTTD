/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file air_map.h Hides the direct accesses to the map array with map accessors */

#ifndef AIR_MAP_H
#define AIR_MAP_H

#include "air_type.h"
#include "depot_type.h"
#include "track_func.h"
#include "tile_map.h"
#include "station_map.h"
#include "viewport_func.h"
#include "table/airporttile_ids.h"

/**
 * Is this station tile an airport?
 * @param t the tile to get the information from
 * @pre IsTileType(t, MP_STATION)
 * @return true if and only if the tile is an airport
 */
static inline bool IsAirport(TileIndex t)
{
	assert(IsTileType(t, MP_STATION));
	return GetStationType(t) == STATION_AIRPORT;
}

/**
 * Is this tile a station tile and an airport tile?
 * @param t the tile to get the information from
 * @return true if and only if the tile is an airport
 */
static inline bool IsAirportTile(TileIndex t)
{
	return IsTileType(t, MP_STATION) && IsAirport(t);
}

/**
 * Set the airport type of an airport tile.
 * @param t Tile to modify.
 * @param type New type for the tile: gravel, asphalt, ...
 * @pre IsAirportTile
 */
static inline void SetAirportType(TileIndex t, AirType type)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(type < AIRTYPE_END);
	SB(_m[t].m3, 0, 4, type);
}

/**
 * Get the airport type of an airport tile.
 * @param t Tile to get the type of.
 * @return The type of the tile: gravel, asphalt, ...
 * @pre IsAirportTile
 */
static inline AirType GetAirportType(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	AirType type = (AirType)GB(_m[t].m3, 0, 4);
	assert(type < AIRTYPE_END);
	return type;
}

/**
 * Set the airport tile type of an airport tile.
 * @param t Tile to modify.
 * @param type Type for the tile: hangar, runway, ...
 * @pre IsAirportTile
 */
static inline void SetAirportTileType(TileIndex t, AirportTileType type)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(type < ATT_END);
	SB(_me[t].m6, 2, 1, GB(type, 0, 1));
	SB(_me[t].m6, 6, 2, GB(type, 1, 2));
}

/**
 * Get the airport tile type of an airport tile.
 * @param t Tile to get the type of.
 * @return The type of the tile.
 * @pre IsAirportTile
 */
static inline AirportTileType GetAirportTileType(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	AirportTileType type = (AirportTileType)((GB(_me[t].m6, 6, 2) << 1) | GB(_me[t].m6, 2, 1));
	assert(type < ATT_END);
	return type;
}

/**
 * Check if a tile is a plain airport tile.
 * @param t Tile to check.
 * @return The type of the tile.
 * @pre IsAirportTile
 */
static inline bool IsSimpleTrack(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_SIMPLE_TRACK;
}

/**
 * Check if a tile is infrastructure of an airport.
 * @param t Tile to check.
 * @return The type of the tile.
 * @pre IsAirportTile
 */
static inline bool IsInfrastructure(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_INFRASTRUCTURE;
}

/**
 * Check if a tile can contain tracks for aircraft.
 * @param t Tile to check.
 * @return Whether the tile may contain airport tracks.
 * @pre IsAirportTile
 */
static inline bool MayHaveAirTracks(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) != ATT_INFRASTRUCTURE;
}

/**
 * Infrastructure may be part of the catchment tiles of the station or not (buildings/radars).
 * @param t Tile to modify.
 * @param catchment Whether the tile should be marked as getting/delivering cargo.
 * @pre IsInfrastructure
 */
static inline void SetCatchmentAirportType(TileIndex t, bool catchment)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsInfrastructure(t));

	SB(_m[t].m4, 0, 1, catchment);
}

/**
 * Get whether the tile has catchment or not.
 * @param t Tile to get the accessibility of.
 * @return Whether the tile is marked as getting/delivering cargo.
 * @pre IsInfrastructure
 */
static inline bool GetCatchmentAirportType(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsInfrastructure(t));

	return GB(_m[t].m4, 0, 1);
}

/**
 * Check if a tile can be the last of an aircraft path.
 * @param t Tile to get the accessibility of.
 * @return Whether the tile is marked as possible end of an aircraft path.
 * @pre IsAirportTile
 */
static inline bool IsSafeWaitingPosition(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return (GetAirportTileType(t) & ATT_SAFEPOINT_MASK) != 0;
}

/**
 * Check if an airport tile is a hangar.
 * @param t Tile to check.
 * @return Whether the tile is a hangar.
 * @pre IsAirportTile
 */
static inline bool IsHangar(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_HANGAR;
}

/**
 * Check if an airport tile is a generic terminal.
 * (terminal, heliport, helipad, built-in heliport).
 * @param t Tile to check.
 * @return Whether the tile is a terminal.
 * @pre IsAirportTile
 */
static inline bool IsTerminal(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_TERMINAL;
}

/**
 * Set the terminal type of an airport tile.
 * @param t Tile to modify.
 * @param type Type of terminal.
 * @pre IsTerminal
 */
static inline void SetTerminalType(TileIndex t, TerminalType type)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	assert(IsTerminal(t));

	assert(type < HTT_TERMINAL_END);

	SB(_m[t].m4, 4, 4, type);
}

/**
 * Get the type of hangar or terminal.
 * @param t Tile to get the type of.
 * @return The type of the hangar or terminal.
 * @pre IsTerminal
 */
static inline TerminalType GetTerminalType(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsTerminal(t));

	TerminalType type = (TerminalType)GB(_m[t].m4, 4, 4);

	assert(type < HTT_TERMINAL_END);

	return type;
}

/**
 * Is a given tile a plane terminal?
 * @param t Tile to get the type of.
 * @return True if it is a plane terminal.
 * @pre IsTerminal
 */
static inline bool IsPlaneTerminal(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsTerminal(t));

	return GetTerminalType(t) == HTT_TERMINAL;
}

/**
 * Is this tile a basic plane terminal?
 * @param t the tile to get the information from.
 * @return true if and only if the tile is a plane terminal.
 */
static inline bool IsPlaneTerminalTile(TileIndex t)
{
	return IsTileType(t, MP_STATION) &&
			IsAirport(t) &&
			IsTerminal(t) &&
			IsPlaneTerminal(t);
}

/**
 * Is a given tile a heliport or a built-in heliport?
 * @param t Tile to get the type of.
 * @return True if it is a heliport.
 * @pre IsTerminal
 */
static inline bool IsHeliport(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsTerminal(t));

	TerminalType type = GetTerminalType(t);

	return type == HTT_HELIPORT || type == HTT_BUILTIN_HELIPORT;
}

/**
 * Is a given tile a heliport or a built-in heliport?
 * @param t Tile to get the type of.
 * @return True if it is a heliport.
 */
static inline bool IsHeliportTile(TileIndex t)
{
	assert(IsValidTile(t));

	return IsTileType(t, MP_STATION) &&
			IsAirport(t) &&
			IsTerminal(t) &&
			IsHeliport(t);
}

/**
 * Is a given tile a helipad?
 * @param t Tile to get the type of.
 * @return True if it is a helipad.
 * @pre IsTerminal
 */
static inline bool IsHelipad(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsTerminal(t));

	return GetTerminalType(t) == HTT_HELIPAD;
}

/**
 * Is this tile a helipad?
 * @param t the tile to get the information from.
 * @return true if and only if the tile is a helipad.
 */
static inline bool IsHelipadTile(TileIndex t)
{
	return IsTileType(t, MP_STATION) &&
			IsAirport(t) &&
			IsTerminal(t) &&
			IsHelipad(t);
}

/**
 * Is a given tile a built-in heliport?
 * @param t Tile to get the type of.
 * @return True if it is a built-in heliport.
 * @pre IsTerminal
 */
static inline bool IsBuiltInHeliport(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsTerminal(t));

	return GetTerminalType(t) == HTT_BUILTIN_HELIPORT;
}

/**
 * Is this tile a built-in heliport?
 * @param t the tile to get the information from.
 * @return true if and only if the tile is a built-in heliport.
 */
static inline bool IsBuiltInHeliportTile(TileIndex t)
{
	return IsTileType(t, MP_STATION) &&
			IsAirport(t) &&
			IsTerminal(t) &&
			IsBuiltInHeliport(t);
}

/**
 * Get landing height for aircraft.
 * @param t the tile to get the information from.
 * @return landing height for aircraft.
 */
static inline int32 GetLandingHeight(TileIndex t)
{
	assert(IsTileType(t, MP_STATION) && IsAirport(t));

	if (!IsTerminal(t)) return 0;

	switch (GetTerminalType(t)) {
		case HTT_HELIPORT:		return 60;
		case HTT_BUILTIN_HELIPORT:	return 54;
		default:			return  0;
	}
}

/**
 * Has this tile airport catchment?
 * @param t the tile to get the information from.
 * @return true if and only if the tile adds for airport catchment.
 * @pre IsAirportTile
 */
static inline bool HasAirportCatchment(TileIndex t)
{
	assert(IsAirportTile(t));

	return ((IsInfrastructure(t) && GetCatchmentAirportType(t)) || IsHeliportTile(t));
}

/**
 * Is a given tile a runway?
 * @param t Tile to check.
 * @return True if it is a runway.
 * @pre IsAirportTile
 */
static inline bool IsRunway(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	AirportTileType air_tile_type = GetAirportTileType(t);
	return air_tile_type >= ATT_RUNWAY && air_tile_type <= ATT_RUNWAY_START;
}

/**
 * Is a given tile a runway extreme?
 * @param t Tile to check.
 * @return True if it is a runway extreme.
 * @pre IsAirportTile
 */
static inline bool IsRunwayExtreme(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	AirportTileType air_tile_type = GetAirportTileType(t);
	return air_tile_type == ATT_RUNWAY_START || air_tile_type == ATT_RUNWAY_END;
}

/**
 * Is a given tile a starting runway?
 * @param t Tile to check.
 * @return True if it is the start of a runway.
 * @pre IsAirportTile
 */
static inline bool IsRunwayStart(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_RUNWAY_START;
}

/**
 * Is a given tile an ending runway?
 * @param t Tile to check.
 * @return True if it is the ending of a runway.
 * @pre IsAirportTile
 */
static inline bool IsRunwayEnd(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_RUNWAY_END;
}

/**
 * Is a given tile the middle section of a runway?
 * @param t Tile to check.
 * @return True if it is a runway middle tile.
 * @pre IsAirportTile
 */
static inline bool IsPlainRunway(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GetAirportTileType(t) == ATT_RUNWAY;
}

/**
 * Set the runway reservation bit.
 * @param t Tile to set.
 * @param reserve new state for the runway reservation.
 * @pre IsRunway
 */
static inline void SetReservationAsRunway(TileIndex t, bool reserve)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunway(t));

	SB(_m[t].m4, 7, 1, reserve);
}

/**
 * Check if a runway is reserved (as a runway).
 * @param t Tile to check.
 * @return True iff it is reserved.
 * @pre IsRunway
 */
static inline bool GetReservationAsRunway(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunway(t));

	return HasBit(_m[t].m4, 7);
}

/**
 * Set the allow landing bit on a runway start/end.
 * @param t Tile to check.
 * @param landing True iff runway should allow landing planes.
 * @pre IsRunwayExtreme
 */
static inline void SetLandingType(TileIndex t, bool landing)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	SB(_m[t].m4, 6, 1, landing);
}

/**
 * Is a given tile a starting runway where landing is allowed?
 * @param t Tile to check.
 * @return True if landing is allowed.
 * @pre IsRunwayExtreme
 */
static inline bool IsLandingTypeTile(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	return HasBit(_m[t].m4, 6);
}


/**
 * Get the direction of a runway.
 * @param t Tile to check.
 * @return Direction of the runway.
 * @pre IsRunwayExtreme
 */
static inline DiagDirection GetRunwayExtremeDirection(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	return (DiagDirection)GB(_m[t].m4, 4, 2);
}

/**
 * Set the two bits for a runway middle section.
 * @param t Tile to set.
 * @param dir the direction of the added runway.
 * @pre IsPlainRunway
 */
static inline Direction GetPlainRunwayDirections(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsPlainRunway(t));

	return (Direction)GB(_m[t].m4, 4, 3);
}

/**
 * Get the runway tracks of a tile.
 * @param t Tile to get the type of.
 * @return Runway tracks of this tile.
 * @pre IsRunway
 */
static inline TrackdirBits GetRunwayTrackdirs(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunway(t));

	Direction dir;
	if (IsPlainRunway(t)) {
		dir = GetPlainRunwayDirections(t);
	} else {
		assert(IsRunwayExtreme(t));
		dir = DiagDirToDir(GetRunwayExtremeDirection(t));
	}

	static TrackdirBits dir_to_trackdirbits[] = {
			TRACKDIR_BIT_Y_NW | TRACKDIR_BIT_X_NE,
			TRACKDIR_BIT_X_NE,
			TRACKDIR_BIT_X_NE | TRACKDIR_BIT_Y_SE,
			TRACKDIR_BIT_Y_SE,
			TRACKDIR_BIT_Y_SE | TRACKDIR_BIT_X_SW,
			TRACKDIR_BIT_X_SW,
			TRACKDIR_BIT_X_SW | TRACKDIR_BIT_Y_NW,
			TRACKDIR_BIT_Y_NW,
	};
	return dir_to_trackdirbits[dir];
}

static inline TrackBits GetRunwayTracks(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunway(t));

	TrackdirBits trackdirs = GetRunwayTrackdirs(t);

	return TrackdirBitsToTrackBits(trackdirs);
}

/**
 * Set the direction of a runway.
 * @param t Tile to check.
 * @param dir Direction of the runway.
 * @pre IsRunwayExtreme
 */
static inline void SetRunwayExtremeDirection(TileIndex t, DiagDirection dir)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	SB(_m[t].m4, 4, 2, dir);
}

/**
 * Set the directions for a runway middle section.
 * @param t Tile to set.
 * @param dir the directions for the runway tile.
 * @pre IsPlainRunway
 */
static inline void AddPlainRunwayDirections(TileIndex t, DiagDirection dir, bool first)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsPlainRunway(t));

	if (first) {
		SB(_m[t].m4, 4, 3, DiagDirToDir(dir));
	} else {
		Direction pre_dir = GetPlainRunwayDirections(t);
		Direction add_dir = DiagDirToDir(dir);
		assert(IsDiagonalDirection(pre_dir));
		if (pre_dir < add_dir) Swap(add_dir, pre_dir);
		assert((DirToDiagDir(pre_dir) - DirToDiagDir(add_dir)) % 2 == 1);
		if (add_dir + 2 == pre_dir) {
			SB(_m[t].m4, 4, 3, add_dir + 1);
		} else if (pre_dir == DIR_NW && add_dir == DIR_NE) {
			SB(_m[t].m4, 4, 3, DIR_N);
		} else {
			NOT_REACHED();
		}
	}
}

/**
 * Set the directions for a runway middle section.
 * @param t Tile to set.
 * @param dir the directions for the runway tile.
 * @return true if tile is no more a runway.
 * @pre IsPlainRunway
 */
static inline bool RemovePlainRunwayDirections(TileIndex t, DiagDirection dir)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsPlainRunway(t));

	Direction cur_dir = GetPlainRunwayDirections(t);
	Direction remove_dir = DiagDirToDir(dir);

	if (remove_dir == cur_dir) {
		SetAirportTileType(t, ATT_SIMPLE_TRACK);
		SB(_m[t].m4, 4, 4, 0);
		return true;
	} else if ((cur_dir + 1) % DIR_END == remove_dir) {
		SB(_m[t].m4, 4, 3, (cur_dir - 1) % DIR_END);
		return false;
	} else if (cur_dir == (remove_dir + 1) % DIR_END) {
		SB(_m[t].m4, 4, 3, (cur_dir + 1) % DIR_END);
		return false;
	}

	NOT_REACHED();
}

/**
 * Set the airport tracks a given tile has
 * (runway tracks are stored in another place).
 * @param t Tile to modify.
 * @param tracks Tracks this tile has.
 * @pre MayHaveAirTracks and !IsHangar
 */
static inline void SetAirportTileTracks(TileIndex t, TrackBits tracks)
{
	assert(MayHaveAirTracks(t));

	SB(_m[t].m5, 0, 6, tracks);
}

/**
 * Set the hangar direction.
 * @param t Tile to modify.
 * @param dir Exit direction of the hangar.
 * @pre IsHangar
 */
static inline void SetHangarDirection(TileIndex t, DiagDirection dir)
{
	assert(IsHangar(t));
	SB(_m[t].m4, 4, 2, dir);
}

/**
 * Set the hangar direction.
 * @param t Tile to modify.
 * @param dir Exit direction of the hangar.
 * @pre IsHangar
 */
static inline DiagDirection GetHangarDirection(TileIndex t)
{
	assert(IsHangar(t));
	return (DiagDirection)GB(_m[t].m4, 4, 2);

}

/**
 * Set whether the hangar is big.
 * @param t Tile to modify.
 * @param is_big Whether the hangar is a big one.
 * @pre IsHangar
 */
static inline void SetBigHangar(TileIndex t, bool is_big)
{
	assert(IsHangar(t));
	SB(_m[t].m4, 7, 1, is_big);
}

/**
 * Get the hangar type.
 * @param t Tile.
 * @return true if big hangar.
 * @pre IsHangar
 */
static inline bool IsBigHangar(TileIndex t)
{
	assert(IsHangar(t));
	return GB(_m[t].m4, 7, 1);
}

/**
 * Get the hangar type.
 * @param t Tile.
 * @return true if big hangar.
 * @pre IsHangar
 */
static inline bool IsSmallHangar(TileIndex t)
{
	assert(IsHangar(t));
	return !IsBigHangar(t);
}

/**
 * Return true if tile is a big hangar.
 * @param t Tile.
 * @return true if big hangar.
 * @pre IsHangar
 */
static inline bool IsBigHangarTile(TileIndex t)
{
	assert(IsAirport(t));
	return IsHangar(t) && IsBigHangar(t);
}

/**
 * Return true if tile is a small hangar.
 * @param t Tile.
 * @return true if small hangar.
 * @pre IsHangar
 */
static inline bool IsSmallHangarTile(TileIndex t)
{
	assert(IsAirport(t));
	return IsHangar(t) && !IsBigHangar(t);
}

/**
 * Get the tracks a given tile has.
 * @param t Tile to get the tracks of.
 * @pre MayHaveAirTracks
 */
static inline TrackBits GetAirportTileTracks(TileIndex t)
{
	assert(MayHaveAirTracks(t));

	return (TrackBits)GB(_m[t].m5, 0, 6);
}

/**
 * Get the tracks a given tile has.
 * @param t Tile to get the tracks of.
 * @pre MayHaveAirTracks
 */
static inline bool HasAirportTileSomeTrack(TileIndex t)
{
	assert(MayHaveAirTracks(t));

	return GetAirportTileTracks(t) != TRACK_BIT_NONE;
}

/**
 * Check if a tile has a given airport track.
 * @param t Tile to check.
 * @param track Track to check.
 * @return True iff tile has given track.
 * @pre MayHaveAirTracks
 */
static inline bool HasAirportTileTrack(TileIndex t, Track track)
{
	assert(MayHaveAirTracks(t));
	return HasTrack(GetAirportTileTracks(t), track);
}

/**
 * Return the reserved airport track bits of the tile.
 * @param t Tile to query.
 * @return Reserved trackbits.
 * @pre MayHaveAirTracks
 */
static inline TrackBits GetReservedAirportTracks(TileIndex t)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	return (TrackBits)(GB(_m[t].m5, 6, 2) | GB(_m[t].m4, 0, 4) << 2);
}

/**
 * Check if a given track is reserved.
 * @param t Tile to query.
 * @param track Track to check.
 * @return True iff the track is reserved on tile t.
 * @pre MayHaveAirTracks
 */
static inline bool HasAirportTrackReserved(TileIndex t, Track track)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	return HasTrack(GetReservedAirportTracks(t), track);
}

/**
 * Check if a given track is reserved.
 * @param t Tile to query.
 * @return True iff the track is reserved on tile t.
 * @pre MayHaveAirTracks
 */
static inline bool HasAirportTrackReserved(TileIndex t)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	return GetReservedAirportTracks(t) != 0;
}

/**
 * Are some of this tracks reserved?
 * @param t Tile to check.
 * @param tracks Tracks to check.
 * @return True if any of the given tracks \a tracks is reserved on tile \a t.
 */
static inline bool HasAirportTracksReserved(TileIndex t, TrackBits tracks)
{
	assert(IsAirportTile(t));
	assert(MayHaveAirTracks(t));
	return (GetReservedAirportTracks(t) & tracks) != TRACK_BIT_NONE;
}

/**
 * Set the reserved tracks of an airport tile.
 * @param t Tile where to reserve.
 * @param trackbits The tracks that will be reserved on tile \a tile
 * @pre MayHaveAirTracks
 */
static inline bool SetAirportTracksReservation(TileIndex t, TrackBits tracks)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	TrackBits already_set = GetReservedAirportTracks(t);
	if ((tracks & ~already_set) == 0) return false;

	tracks |= already_set;

	SB(_m[t].m5, 6, 2, tracks);
	SB(_m[t].m4, 0, 4, tracks >> 2);

	return true;
}

/**
 * Reserve an airport track on a tile.
 * @param t Tile where to reserve.
 * @param track The track that will be reserved on tile \a tile
 * @pre MayHaveAirTracks
 */
static inline void SetAirportTrackReservation(TileIndex t, Track track)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	SetAirportTracksReservation(t, TrackToTrackBits(track));
}

/**
 * Remove an airport track on a tile.
 * @param t Tile where to free the reserved track.
 * @param track The track that will be freed on tile \a tile
 * @pre MayHaveAirTracks
 */
static inline bool RemoveAirportTrackReservation(TileIndex t, Track track)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	TrackBits already_set = GetReservedAirportTracks(t);
	TrackBits tracks = TrackToTrackBits(track);
	if ((tracks & already_set) == 0) return false;
	already_set &= ~tracks;

	SB(_m[t].m5, 6, 2, already_set);
	SB(_m[t].m4, 0, 4, already_set >> 2);

	return true;
}

/**
 * Are some of this tracks reserved?
 * @param t Tile to check.
 * @param tracks Tracks to check.
 * @return True if any of the given tracks \a tracks is reserved on tile \a t.
 */
static inline bool HasAirportTileAnyReservation(TileIndex t)
{
	assert(IsAirportTile(t));
	assert(MayHaveAirTracks(t));
	return (GetReservedAirportTracks(t) != TRACK_BIT_NONE) ||
			(IsRunway(t) && GetReservationAsRunway(t));
}

/**
 * Get the gfx_id a given tile has.
 * @param t Tile to get the tracks of.
 * @pre IsInfrastructure
 */
static inline AirportTiles GetAirportGfxFromTile(TileIndex t)
{
	assert(IsAirportTile(t));
	assert(!MayHaveAirTracks(t));
	assert(IsInfrastructure(t));

	return (AirportTiles)_m[t].m5;
}

/**
 * Get the sprite for an airport tile.
 * @param t Tile to get the sprite of.
 * @return AirportTile ID.
 */
AirportTiles GetAirportGfx(TileIndex t);

/**
 * Return whether is an airport tile of a given station.
 * @param t tile.
 * @param st_id ID of the station.
 */
static inline bool IsAirportTileOfStation(TileIndex t, StationID st_id)
{
	assert(IsValidTile(t));

	return IsAirportTile(t) && st_id == GetStationIndex(t);
}

#endif /* AIR_MAP_H */
