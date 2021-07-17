/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file air_map.h Hides the direct accesses to the map array with map accessors. */

#ifndef AIR_MAP_H
#define AIR_MAP_H

#include "air_type.h"
#include "depot_type.h"
#include "track_func.h"
#include "tile_map.h"
#include "station_map.h"
#include "viewport_func.h"
#include "table/airporttile_ids.h"

extern bool _show_airport_tracks;

/**
 * Set the airport type of an airport tile.
 * @param t Tile to modify.
 * @param type New type for the tile: gravel, asphalt, ...
 * @pre IsAirportTile
 */
static inline void SetAirType(Tile t, AirType type)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(type < AIRTYPE_END);
	SB(t.m3(), 0, 4, type);
}

/**
 * Get the airport type of an airport tile.
 * @param t Tile to get the type of.
 * @return The type of the tile: gravel, asphalt, ...
 * @pre IsAirportTile
 */
static inline AirType GetAirType(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	AirType type = (AirType)GB(t.m3(), 0, 4);
	assert(type < AIRTYPE_END);
	return type;
}

/**
 * Set the airport tile type of an airport tile.
 * @param t Tile to modify.
 * @param type Type for the tile: hangar, runway, ...
 * @pre IsAirportTile
 */
static inline void SetAirportTileType(Tile t, AirportTileType type)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(type < ATT_END);
	SB(t.m5(), 4, ATT_NUM_BITS, type);
}

/**
 * Get the airport tile type of an airport tile.
 * @param t Tile to get the type of.
 * @return The type of the tile.
 * @pre IsAirportTile
 */
static inline AirportTileType GetAirportTileType(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	AirportTileType type = (AirportTileType)(GB(t.m5(), 4, ATT_NUM_BITS));
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
static inline bool IsInfrastructure(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GB(t.m5(), 8 - ATT_INFRA_LAYOUT_NUM_BITS, ATT_INFRA_LAYOUT_NUM_BITS) == ATT_INFRA_LAYOUT_BITS;
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

	return !IsInfrastructure(t);
}

/**
 * Infrastructure may be part of the catchment tiles of the station or not (buildings/radars).
 * @param t Tile to modify.
 * @param catchment Whether the tile should be marked as getting/delivering cargo.
 * @pre IsInfrastructure
 */
static inline void SetCatchmentAirportType(Tile t, bool catchment)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsInfrastructure(t));

	SB(t.m5(), 4, 1, catchment);
}

/**
 * Get whether the tile has catchment or not.
 * @param t Tile to get the accessibility of.
 * @return Whether the tile is marked as getting/delivering cargo.
 * @pre IsInfrastructure
 */
static inline bool GetCatchmentAirportType(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsInfrastructure(t));

	return GB(t.m5(), 4, 1);
}

/**
 * Set the apron type of an airport tile.
 * @param t Tile to modify.
 * @param type Type of apron.
 * @pre IsApron
 */
static inline void SetApronType(Tile t, ApronType type)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	assert(IsApron(t));

	assert(type < APRON_END);

	SB(t.m5(), 4, 2, type);
}

/**
 * Is a given tile a plane apron?
 * @param t Tile to get the type of.
 * @return True if it is a plane apron.
 * @pre IsApron
 */
static inline bool IsPlaneApron(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsApron(t));

	return GetApronType(t) == APRON_APRON;
}

/**
 * Is this tile a basic plane apron?
 * @param t the tile to get the information from.
 * @return true if and only if the tile is a plane apron.
 */
static inline bool IsPlaneApronTile(TileIndex t)
{
	return IsTileType(t, MP_STATION) &&
			IsAirport(t) &&
			IsApron(t) &&
			IsPlaneApron(t);
}

/**
 * Is a given tile a heliport or a built-in heliport?
 * @param t Tile to get the type of.
 * @return True if it is a heliport.
 * @pre IsApron
 */
static inline bool IsHeliport(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsApron(t));

	ApronType type = GetApronType(t);

	return type == APRON_HELIPORT || type == APRON_BUILTIN_HELIPORT;
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
			IsApron(t) &&
			IsHeliport(t);
}

/**
 * Is a given tile a helipad?
 * @param t Tile to get the type of.
 * @return True if it is a helipad.
 * @pre IsApron
 */
static inline bool IsHelipad(TileIndex t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsApron(t));

	return GetApronType(t) == APRON_HELIPAD;
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
			IsApron(t) &&
			IsHelipad(t);
}

/**
 * Get landing height for aircraft.
 * @param t the tile to get the information from.
 * @return landing height for aircraft.
 */
static inline int GetLandingHeight(TileIndex t)
{
	assert(IsTileType(t, MP_STATION) && IsAirport(t));

	if (!IsApron(t)) return 0;

	switch (GetApronType(t)) {
		case APRON_HELIPORT:
			return 60;
		case APRON_BUILTIN_HELIPORT:
			return 54;
		default:
			return  0;
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
 * Set the rotation of an airport tile (see SetHangarDirection).
 * @param t Tile to modify.
 * @param dir Rotation.
 * @pre IsAirportTile && (IsInfrastructure || IsApron)
 */
static inline void SetAirportTileRotation(Tile t, DiagDirection dir)
{
	assert(IsAirportTile(t));
	assert(IsApron(t) || IsInfrastructure(t));
	SB(t.m8(), 14, 2, dir);
}

/**
 * Get the hangar direction.
 * @param t Tile to check.
 * @return The exit direction of the hangar.
 * @pre IsAirportTile && (IsInfrastructure || IsApron)
 */
static inline DiagDirection GetAirportTileRotation(Tile t)
{
	assert(IsAirportTile(t));
	assert(IsApron(t) || IsInfrastructure(t));
	return (DiagDirection)GB(t.m8(), 14, 2);

}

/**
 * Is a given tile a runway?
 * @param t Tile to check.
 * @return True if it is a runway.
 * @pre IsAirportTile
 */
static inline bool IsRunway(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GB(t.m5(), 8 - ATT_RUNWAY_LAYOUT_NUM_BITS, ATT_RUNWAY_LAYOUT_NUM_BITS) == ATT_RUNWAY_LAYOUT_BITS;
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

	return IsRunway(t) && GetAirportTileType(t) != ATT_RUNWAY_MIDDLE;
}

/**
 * Is a given tile a starting runway?
 * @param t Tile to check.
 * @return True if it is the start of a runway.
 * @pre IsAirportTile
 */
static inline bool IsRunwayStart(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	return GB(t.m5(), 8 - ATT_RUNWAY_START_LAYOUT_NUM_BITS, ATT_RUNWAY_START_LAYOUT_NUM_BITS) == ATT_RUNWAY_START_LAYOUT_BITS;
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

	return IsRunway(t) && GetAirportTileType(t) == ATT_RUNWAY_MIDDLE;
}

/**
 * Set the runway reservation bit.
 * @param t Tile to set.
 * @param reserve new state for the runway reservation.
 * @pre IsRunway
 */
static inline void SetReservationAsRunway(Tile t, bool reserve)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunway(t));

	SB(t.m8(), 15, 1, reserve);
}

/**
 * Check if a runway is reserved (as a runway).
 * @param t Tile to check.
 * @return True iff it is reserved.
 * @pre IsRunway
 */
static inline bool GetReservationAsRunway(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunway(t));

	return HasBit(t.m8(), 15);
}

/**
 * Set the allow landing bit on a runway start/end.
 * @param t Tile to check.
 * @param landing True iff runway should allow landing planes.
 * @pre IsRunwayExtreme
 */
static inline void SetLandingType(Tile t, bool landing)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	SB(t.m5(), 4, 1, landing);
}

/**
 * Is a given tile a starting runway where landing is allowed?
 * @param t Tile to check.
 * @return True if landing is allowed.
 * @pre IsRunwayExtreme
 */
static inline bool IsLandingTypeTile(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	return HasBit(t.m5(), 4);
}


/**
 * Get the direction of a runway.
 * @param t Tile to check.
 * @return Direction of the runway.
 * @pre IsRunwayExtreme
 */
static inline DiagDirection GetRunwayExtremeDirection(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	return (DiagDirection)GB(t.m8(), 12, 2);
}

/**
 * Get the two bits for a runway middle section.
 * @param t Tile to inspect.
 * @return the directions of the runway.
 * @pre IsPlainRunway
 */
static inline Direction GetPlainRunwayDirections(Tile t)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsPlainRunway(t));

	return (Direction)GB(t.m8(), 12, 3);
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
static inline void SetRunwayExtremeDirection(Tile t, DiagDirection dir)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsRunwayExtreme(t));

	SB(t.m8(), 12, 2, dir);
}

/**
 * Set the directions for a runway middle section.
 * @param t Tile to set.
 * @param dir the directions for the runway tile.
 * @pre IsPlainRunway
 */
static inline void AddPlainRunwayDirections(Tile t, DiagDirection dir, bool first)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsPlainRunway(t));

	if (first) {
		SB(t.m8(), 12, 3, DiagDirToDir(dir));
	} else {
		Direction pre_dir = GetPlainRunwayDirections(t);
		Direction add_dir = DiagDirToDir(dir);
		assert(IsDiagonalDirection(pre_dir));
		if (pre_dir < add_dir) Swap(add_dir, pre_dir);
		assert(((uint)DirToDiagDir(pre_dir) + (uint)DirToDiagDir(add_dir)) % 2 == 1);
		if (add_dir + 2 == pre_dir) {
			SB(t.m8(), 12, 3, add_dir + 1);
		} else if (pre_dir == DIR_NW && add_dir == DIR_NE) {
			SB(t.m8(), 12, 3, DIR_N);
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
static inline bool RemovePlainRunwayDirections(Tile t, DiagDirection dir)
{
	assert(IsValidTile(t));
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(IsPlainRunway(t));

	Direction cur_dir = GetPlainRunwayDirections(t);
	Direction remove_dir = DiagDirToDir(dir);

	if (remove_dir == cur_dir) {
		SB(t.m8(), 12, 4, 0);
		SetAirportTileType(t, ATT_SIMPLE_TRACK);
		return true;
	} else if ((cur_dir + 1) % DIR_END == remove_dir) {
		SB(t.m8(), 12, 3, (cur_dir - 1) & (DIR_END - 1));
		return false;
	} else if (cur_dir == (remove_dir + 1) % DIR_END) {
		SB(t.m8(), 12, 3, (cur_dir + 1) % DIR_END);
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
static inline void SetAirportTileTracks(Tile t, TrackBits tracks)
{
	assert(MayHaveAirTracks(t));

	SB(t.m8(), 0, 6, tracks);
}

/**
 * Set the hangar direction.
 * @param t Tile to modify.
 * @param dir Exit direction of the hangar.
 * @pre IsHangar
 */
static inline void SetHangarDirection(Tile t, DiagDirection dir)
{
	assert(IsHangar(t));
	SB(t.m8(), 14, 2, dir);
}

/**
 * Get the hangar direction.
 * @param t Tile to check.
 * @return the exit direction of the hangar.
 * @pre IsHangar
 */
static inline DiagDirection GetHangarDirection(Tile t)
{
	assert(IsHangar(t));
	return (DiagDirection)GB(t.m8(), 14, 2);

}

/**
 * Set whether the hangar is an extended one.
 * @param t Tile to modify.
 * @param is_extended Whether the hangar is an extended hangar.
 * @pre IsHangar
 */
static inline void SetExtendedHangar(Tile t, bool is_extended)
{
	assert(IsHangar(t));
	SB(t.m5(), 5, 1, is_extended);
}

/**
 * Check if it a tile is an extended hangar.
 * @param t Tile.
 * @return true if it is an extended hangar.
 * @pre IsHangar
 */
static inline bool IsExtendedHangar(Tile t)
{
	assert(IsHangar(t));
	return GB(t.m5(), 5, 1);
}

/**
 * Check if it a tile is a standard hangar.
 * @param t Tile.
 * @return true if extended hangar.
 * @pre IsHangar
 */
static inline bool IsStandardHangar(TileIndex t)
{
	assert(IsHangar(t));
	return !IsExtendedHangar(t);
}

/**
 * Return true if tile is an extended hangar.
 * @param t Tile.
 * @return true if extended hangar.
 * @pre IsHangar
 */
static inline bool IsExtendedHangarTile(TileIndex t)
{
	assert(IsAirport(t));
	return IsHangar(t) && IsExtendedHangar(t);
}

/**
 * Return true if tile is a standard hangar.
 * @param t Tile.
 * @return true if standard hangar.
 * @pre IsHangar
 */
static inline bool IsStandardHangarTile(TileIndex t)
{
	assert(IsAirport(t));
	return IsHangar(t) && !IsExtendedHangar(t);
}

/**
 * Get the tracks a given tile has.
 * @param t Tile to get the tracks of.
 * @pre MayHaveAirTracks
 */
static inline TrackBits GetAirportTileTracks(Tile t)
{
	assert(MayHaveAirTracks(t));

	return (TrackBits)GB(t.m8(), 0, 6);
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
static inline TrackBits GetReservedAirportTracks(Tile t)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	return (TrackBits)GB(t.m8(), 6, 6);
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
 * Check if an airport tile has any reserved track.
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
static inline bool SetAirportTracksReservation(Tile t, TrackBits tracks)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	TrackBits already_set = GetReservedAirportTracks(t);
	if ((tracks & ~already_set) == 0) return false;

	tracks |= already_set;

	SB(t.m8(), 6, 6, tracks);

	if (_show_airport_tracks) MarkTileDirtyByTile(t);
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
 * return whether the track has been removed
 * @pre MayHaveAirTracks
 */
static inline bool RemoveAirportTrackReservation(Tile t, Track track)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));
	assert(MayHaveAirTracks(t));

	TrackBits reserved = GetReservedAirportTracks(t);
	TrackBits tracks = TrackToTrackBits(track);
	if ((tracks & reserved) == TRACK_BIT_NONE) return false;
	reserved &= ~tracks;

	SB(t.m8(), 6, 6, reserved);
	if (_show_airport_tracks) MarkTileDirtyByTile(t);

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
 * Whether the gfx of the tile are controlled through its airtype.
 * @param t Tile.
 * @return whether it is an airtype controlled airtype.
 */
static inline bool HasAirtypeGfx(Tile t)
{
	assert(IsAirportTile(t));
	return (bool)GB(t.m6(), 7, 1);
}

/**
 * Set the gfx type of the tile.
 * @param t Tile to set the gfx type of.
 * @param airtype_controlled whether the gfx of the tile are handled via its airtype.
 */
static inline void SetAirGfxType(Tile t, bool airtype_controlled)
{
	assert(IsAirportTile(t));
	SB(t.m6(), 7, 1, airtype_controlled);
}

/**
 * Get the gfx_id a given tile has.
 * @param t Tile to get the gfx of.
 */
static inline AirportTiles GetTileAirportGfx(Tile t)
{
	assert(IsAirportTile(t));
	return (AirportTiles)GB(t.m4(), 0, 8);
}

/**
 * Set the gfx_id a given tile has.
 * @param t Tile to set the gfx of.
 */
static inline void SetTileAirportGfx(Tile t, AirportTiles at)
{
	assert(IsAirportTile(t));
	SB(t.m4(), 0, 8, at);
}

/**
 * Get the sprite for an airport tile.
 * @param t Tile to get the sprite of.
 * @return AirportTile ID.
 */
StationGfx GetAirportGfx(TileIndex t);

StationGfx GetTranslatedAirportTileID(StationGfx gfx);

/**
 * Set the gfx_id a given tile has for an airtype.
 * @param t Tile to set the gfx of.
 * @pre IsInfrastructure
 */
static inline void SetAirportGfxForAirtype(Tile t, AirportTiles at)
{
	assert(IsAirportTile(t));
	assert(IsInfrastructure(t));
	SB(t.m8(), 0, 8, at);
}

/**
 * Get the gfx_id a given tile has for an airtype.
 * @param t Tile to get the gfx of.
 */
static inline AirportTiles GetAirportGfxForAirtype(Tile t)
{
	assert(IsAirportTile(t));
	return (AirportTiles)GB(t.m8(), 0, 8);
}

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
