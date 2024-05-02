/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_water.cpp Path based system routines for water. */

#include "stdafx.h"
#include "viewport_func.h"
#include "ship.h"
#include "vehicle_func.h"
#include "pathfinder/follow_track.hpp"
#include "pbs_water.h"

/**
 * Return the reserved water track bits of the tile.
 * @param t Tile to query.
 * @return Reserved trackbits.
 * @pre WaterTrackMayExist
 */
TrackBits GetReservedWaterTracks(TileIndex t)
{
	assert(WaterTrackMayExist(t));
	if (IsTileType(t, MP_TUNNELBRIDGE)) {
		if (HasTunnelBridgeReservation(t)) {
			return (GetTunnelBridgeDirection(t) % 2) == 0 ? TRACK_BIT_X : TRACK_BIT_Y;
		} else {
			return TRACK_BIT_NONE;
		}
	}

	byte track_b = GB(t.m8(), 12, 3);
	if (track_b == 0) return TRACK_BIT_NONE;
	Track track = (Track)(track_b - 1);    // map array saves Track+1
	return TrackToTrackBits(track) | (HasBit(t.m8(), 15) ? TrackToTrackBits(TrackToOppositeTrack(track)) : TRACK_BIT_NONE);
}

/**
 * When trying to enter a tile with possible trackdirs,
 * return only the tracks that don't collide with another path.
 * @param t Tile
 * @param trackdirs Track directions that are free.
 * @return Track directions that, if taken, don't collide with other paths.
 */
TrackBits GetFreeWaterTrackReservation(TileIndex t, TrackdirBits trackdirs)
{
	TrackBits trackbits = TRACK_BIT_NONE;
	for (Trackdir trackdir = RemoveFirstTrackdir(&trackdirs); trackdir != INVALID_TRACKDIR; trackdir = RemoveFirstTrackdir(&trackdirs)) {
		if (IsWaterPositionFree(t, trackdir)) trackbits |= TrackToTrackBits(TrackdirToTrack(trackdir));
	}
	return trackbits;
}

/**
 * Set the reserved tracks of a tile.
 * @param t Tile to set.
 * @param tracks Tracks to reserve on the tile \a t.
 * @pre WaterTrackMayExist
 */
void SetWaterTrackReservation(TileIndex t, TrackBits tracks)
{
	if (!_settings_game.pf.ship_path_reservation) return;

	Track track = RemoveFirstTrack(&tracks);
	SB(t.m8(), 12, 3, track == INVALID_TRACK ? 0 : track + 1);
	SB(t.m8(), 15, 1, tracks != TRACK_BIT_NONE);
}

/**
 * Return true if a ship can try to cross the tile.
 * @tile Tile to check.
 * @return True if the tile may contain water tracks, false otherwise.
 */
bool WaterTrackMayExist(TileIndex t)
{
	return IsValidTile(t) && (
			IsTileType(t, MP_WATER) ||
			IsBuoyTile(t) || IsDockTile(t) ||
			(IsTileType(t, MP_RAILWAY) && GetRailGroundType(t) == RAIL_GROUND_WATER) ||
			(IsTileType(t, MP_TUNNELBRIDGE) && GetTunnelBridgeTransportType(t) == TRANSPORT_WATER)
		);
}

/**
 * Reserve a track of a tile.
 * @param t Tile where to reserve the (water) track.
 * @param b Track to reserve.
 * @param value true if adding a reservation, false if removing it.
 * @return True if the track has been reserved.
 */
bool SetWaterTrackReservation(TileIndex t, Track track, bool value)
{
	assert(WaterTrackMayExist(t));
	assert(track != INVALID_TRACK);

	if (!_settings_game.pf.ship_path_reservation && value) return true;

	if (IsTileType(t, MP_TUNNELBRIDGE)) {
		assert(IsDiagonalTrack(track));
		if (value == HasTunnelBridgeReservation(t)) return false;

		SetTunnelBridgeReservation(t, value);
		TileArea ta(t);
		t = GetOtherTunnelBridgeEnd(t);
		SetTunnelBridgeReservation(t, value);
		ta.Add(t);

		for (TileIndex t : ta) MarkTileDirtyByTile(t);

		return true;
	}

	TrackBits trackbits = GetReservedWaterTracks(t);

	if (value) {
		if (TrackOverlapsTracks(trackbits, track)) return false;
		trackbits |= TrackToTrackBits(track);

		SB(t.m8(), 12, 3, RemoveFirstTrack(&trackbits) + 1);
		SB(t.m8(), 15, 1, trackbits != TRACK_BIT_NONE);
	} else {
		TrackBits removing_track = TrackToTrackBits(track);
		if (!(trackbits & removing_track)) return false;
		trackbits &= ~removing_track;
		track = RemoveFirstTrack(&trackbits);
		assert(trackbits == TRACK_BIT_NONE);
		SB(t.m8(), 12, 3, track == INVALID_TRACK ? 0 : track + 1);
		SB(t.m8(), 15, 1, 0);
	}

	MarkTileDirtyByTile(t);
	return true;
}

/**
 * There is a track already reserved?
 * @param tile Tile to check.
 * @return True if there is a track reserved on \a tile, false if none is reserved.
 * @pre WaterTrackMayExist
 */
bool HasWaterTrackReservation(TileIndex t)
{
	assert(WaterTrackMayExist(t));
	return GetReservedWaterTracks(t) != TRACK_BIT_NONE;
}

/**
 * Are some of this tracks reserved?
 * @param t Tile to check.
 * @param tracks Tracks to check.
 * @return True if any of the given tracks \a tracks is reserved on tile \a t.
 */
bool HasWaterTracksReserved(TileIndex t, TrackBits tracks)
{
	assert(WaterTrackMayExist(t));
	return (GetReservedWaterTracks(t) & tracks) != TRACK_BIT_NONE;
}

/**
 * Check if a track collides with the water tracks reserved on a tile.
 * @param t Tile to check.
 * @param track Track to check.
 * @return True if the track can be reserved on tile \a t without colliding other reserved paths.
 * @note It doesn't check next tile.
 */
bool TrackCollidesTrackReservation(TileIndex t, Track track)
{
	assert(WaterTrackMayExist(t));
	return TrackOverlapsTracks(GetReservedWaterTracks(t), track);
}

/**
 * Check if a tile can be reserved and does not collide with another path on next tile.
 * @param tile The tile.
 * @param trackdir The trackdir to check.
 * @return True if reserving track direction \a trackdir on tile \a tile
 *         doesn't collide with other paths.
 */
bool IsWaterPositionFree(TileIndex tile, Trackdir trackdir)
{
	if (!_settings_game.pf.ship_path_reservation) return true;

	/* Check the next tile, if a path collides, then it isn't a waiting position at all. */
	CFollowTrackWater ft(INVALID_COMPANY);

	/* Skip tiles of a lock. */
	if (IsLockTile(tile)) {
		while (ft.Follow(tile, trackdir) && CheckSameLock(tile, ft.m_new_tile)) {
			tile = ft.m_new_tile;
		}
	}

	Track track = TrackdirToTrack(trackdir);

	/* Tile reserved? Can never be a free waiting position. */
	if (TrackCollidesTrackReservation(tile, track)) return false;

	if (!ft.Follow(tile, trackdir)) {
		if (IsTileType(ft.m_new_tile, MP_INDUSTRY)) {
			/* Let ships approach oil rigs. */
			return true;
		} else if (!(IsDockTile(tile) && IsDockTile(ft.m_new_tile))) {
			/* Let ships cross docks when next tile is a dock. */
			/* We can reverse on docks if needed, but only when next tile is a dock as well. */
			return false;
		}
	}

	/* On tunnels and bridges we must check the other bridge end. */
	if (IsTileType(tile, MP_TUNNELBRIDGE) && IsTileType(ft.m_new_tile, MP_TUNNELBRIDGE) &&
		GetOtherTunnelBridgeEnd(tile) == ft.m_new_tile) {
		tile = ft.m_new_tile;
		if (!ft.Follow(ft.m_new_tile, trackdir)) return false;
	}

	/* Check for reachable tracks.
	 * Don't discard 90deg turns as we don't want two paths to collide
	 * even if they cannot really collide because of a 90deg turn */
	ft.m_new_td_bits &= DiagdirReachesTrackdirs(ft.m_exitdir);

	return !HasWaterTracksReserved(ft.m_new_tile, TrackdirBitsToTrackBits(ft.m_new_td_bits));
}

/**
 * Follow a reservation starting from a specific tile to its end.
 * @param o Owner (unused: as ships can cross tiles of other owners).
 * @param rts Former railtypes (unused: can be converted to water_types? sea, river, canal).
 * @param tile Start tile.
 * @param trackdir Track direction to look for.
 * @return Last tile and info of the reserved path.
 */
static PBSTileInfo FollowShipReservation(Owner o, RailTypes rts, TileIndex tile, Trackdir trackdir)
{
	/* Start track not reserved? */
	assert(HasWaterTracksReserved(tile, TrackToTrackBits(TrackdirToTrack(trackdir))));

	/* Do not disallow 90 deg turns as the setting might have changed between reserving and now. */
	CFollowTrackWater fs(o, rts);
	while (fs.Follow(tile, trackdir)) {
		TrackdirBits reserved = fs.m_new_td_bits & TrackBitsToTrackdirBits(GetReservedWaterTracks(fs.m_new_tile));
		if (reserved == TRACKDIR_BIT_NONE) break;

		/* Can't have more than one reserved trackdir */
		trackdir = FindFirstTrackdir(reserved);
		tile = fs.m_new_tile;
	}

	return PBSTileInfo(tile, trackdir, false);
}

/**
 * Helper struct for finding the best matching vehicle on a specific track.
 */
struct FindShipOnTrackInfo {
	PBSTileInfo res; ///< Information about the track.
	Ship *best;      ///< The currently "best" vehicle we have found.

	/** Init the best ship to nullptr always! */
	FindShipOnTrackInfo() : best(nullptr) {}
};

/**
 * Callback for Has/FindVehicleOnPos to find a train on a specific track.
 * @param v Vehicle to test.
 * @param data FindshipOnTrackInfo.
 * @return The vehicle on track or nullptr if none found.
 */
static Vehicle *FindShipOnTrackEnum(Vehicle *v, void *data)
{
	FindShipOnTrackInfo *info = (FindShipOnTrackInfo *)data;

	if (v->type != VEH_SHIP) return nullptr;

	Ship *s = Ship::From(v);
	TrackBits tracks = s->state;

	if (tracks == TRACK_BIT_DEPOT) return nullptr;

	if (tracks == TRACK_BIT_WORMHOLE || HasBit(tracks, TrackdirToTrack(info->res.trackdir))) {
		/* ALWAYS return the lowest ID (anti-desync!) */
		if (info->best == nullptr || s->index < info->best->index) info->best = s;
		return s;
	}

	return nullptr;
}

/**
 * Follow the reserved path of a ship to its end.
 * @param v The vehicle.
 * @return The last tile of the reservation or the current ship tile if no reservation is present.
 */
PBSTileInfo FollowShipReservation(const Ship *v)
{
	assert(v->type == VEH_SHIP);

	return FollowShipReservation(v->owner, (RailTypes)0, v->tile,  v->GetVehicleTrackdir());
}

/**
 * Find the ship which has reserved a specific path.
 * @param tile A tile on the path.
 * @param track A reserved track on the tile.
 * @return The vehicle holding the reservation or nullptr if the path is stray.
 */
Ship *GetShipForReservation(TileIndex tile, Track track)
{
	assert(HasWaterTracksReserved(tile, TrackToTrackBits(track)));
	Trackdir trackdir = TrackToTrackdir(track);

	/* Follow the path from tile to both ends.
	 * One of the end tiles should have a ship on it. */
	for (int i = 0; i < 2; ++i, trackdir = ReverseTrackdir(trackdir)) {
		FindShipOnTrackInfo fsoti;
		fsoti.res = FollowShipReservation(GetTileOwner(tile), (RailTypes)0, tile, trackdir);

		FindVehicleOnPos(fsoti.res.tile, &fsoti, FindShipOnTrackEnum);
		if (fsoti.best != nullptr) return fsoti.best;

		/* Special case for bridges/tunnels: check the other end as well. */
		if (IsTileType(fsoti.res.tile, MP_TUNNELBRIDGE)) {
			FindVehicleOnPos(GetOtherTunnelBridgeEnd(fsoti.res.tile), &fsoti, FindShipOnTrackEnum);
			if (fsoti.best != nullptr) return fsoti.best;
		}

		/* Special case for locks: check all lock tiles. */
		if (IsLockTile(fsoti.res.tile)) {
			TileIndex t = GetLockMiddleTile(fsoti.res.tile);
			TileIndexDiff tilediff_to_lower = GetLockTileIndexDiffToLastLowerTile(t);
			TileArea ta(t + tilediff_to_lower, t - tilediff_to_lower);

			for (TileIndex t : ta) {
				FindVehicleOnPos(t, &fsoti, FindShipOnTrackEnum);
				if (fsoti.best != nullptr) return fsoti.best;
			}
		}
	}

	/* Ship that reserved a given path not found. */
	return nullptr;
}

/**
 * Remove a reservation starting on given tile with a given trackdir.
 * @param tile Starting tile.
 * @param trackdir Starting trackdir.
 */
void LiftShipPathReservation(TileIndex tile, Trackdir trackdir)
{
	if (!SetWaterTrackReservation(tile, TrackdirToTrack(trackdir), false)) NOT_REACHED();

	CFollowTrackWater fs(INVALID_COMPANY);

	while (fs.Follow(tile, trackdir)) {
		/* Skip 2nd tile of an aqueduct. */
		if (IsBridgeTile(tile) && IsBridgeTile(fs.m_new_tile) &&
				GetOtherTunnelBridgeEnd(fs.m_new_tile) == tile) {
			tile = fs.m_new_tile;
			continue;
		}

		fs.m_new_td_bits &= TrackBitsToTrackdirBits(GetReservedWaterTracks(fs.m_new_tile));

		/* Can't have more than one reserved trackdir */
		trackdir = FindFirstTrackdir(fs.m_new_td_bits);
		if (trackdir == INVALID_TRACKDIR) break;
		tile = fs.m_new_tile;

		if (!SetWaterTrackReservation(tile, TrackdirToTrack(trackdir), false)) NOT_REACHED();
	}
}

/**
 * Unreserve the path of a ship, keeping of course the current tile and track reserved.
 * @param ship The ship we want to free the path of.
 * @param tile The tile that asks the path to be freed (see note 2).
 * @param track The track that asks to be freed (see note 2).
 * @param keep_pref_water_trackdirs Whether to keep initial reservation for consistence
 *                                  with preferred water trackdirs.
 * @note 1.- The path will not be freed if the ship tile and track is
 *       the same as the tile and track that ask to free the path.
 * @note 2.- If @param tile is INVALID_TILE, then the algorithm removes the full path
 *       the ship, keeping a consistent path with preferred trackdirs
 *       if @param keep_pref_water_trackdirs is true.
 */
void LiftShipPathReservation(Ship *v, TileIndex tile, Track track, bool keep_pref_water_trackdirs)
{
	assert(v != nullptr);
	if (v->tile == tile && TrackdirToTrack(v->GetVehicleTrackdir()) == track) return;

	/* Do not disallow 90 deg turns as the setting might have changed between reserving and now. */
	CFollowTrackWater fs(v->owner);

	tile = v->tile;
	Trackdir trackdir = v->GetVehicleTrackdir();

	/* Skip first tile of a tunnel. */
	if (IsTileType(tile, MP_TUNNELBRIDGE) && GetTunnelBridgeDirection(tile) == TrackdirToExitdir(trackdir)) {
		fs.Follow(tile, trackdir);
		tile = fs.m_new_tile;
		trackdir = FindFirstTrackdir(fs.m_new_td_bits & TrackBitsToTrackdirBits(GetReservedWaterTracks(tile)));
	}

	bool check_first = true;
	keep_pref_water_trackdirs &= HasPreferredWaterTrackdirs(v->tile) &&
			!HasTrackdir(GetPreferredWaterTrackdirs(v->tile), trackdir);
	TileIndex keep_lock_reserved = v->tile;

	while (fs.Follow(tile, trackdir)) {
		/* Skip 2nd tile of an aqueduct. */
		if (IsBridgeTile(tile) && IsBridgeTile(fs.m_new_tile) &&
				GetOtherTunnelBridgeEnd(fs.m_new_tile) == tile) {
			tile = fs.m_new_tile;
			continue;
		}

		fs.m_new_td_bits &= TrackBitsToTrackdirBits(GetReservedWaterTracks(fs.m_new_tile));

		/* Can't have more than one reserved trackdir */
		trackdir = FindFirstTrackdir(fs.m_new_td_bits);
		if (trackdir == INVALID_TRACKDIR) break;

		tile = fs.m_new_tile;

		if (check_first) {
			if (tile == v->dest_tile) return;
			check_first = false;
		}

		/* Skip tiles of the same lock. */
		if (CheckSameLock(keep_lock_reserved, tile)) continue;

		if (keep_pref_water_trackdirs) {
			if (HasPreferredWaterTrackdirs(tile) &&
					!HasTrackdir(GetPreferredWaterTrackdirs(tile), trackdir)) continue;

			/* A path must keep all lock tiles reserved. */
			if (IsLockTile(tile)) keep_lock_reserved = tile;

			keep_pref_water_trackdirs = false;
			continue;
		}

		if (!SetWaterTrackReservation(tile, TrackdirToTrack(trackdir), false)) NOT_REACHED();
	}
}

/**
 * Free ship paths on a tile.
 * @param tile Tile we want to free.
 * @param keep_pref_water_trackdirs Whether keep preferred water trackdir paths if possible.
 * @return True if tile has no reservation after the paths have been freed.
 */
bool LiftShipPathsReservations(TileIndex tile, bool keep_pref_water_trackdirs)
{
	if (!HasWaterTrackReservation(tile)) return true;

	Track track;
	for (Track track : SetTrackBitIterator(GetReservedWaterTracks(tile))) {
		Ship *s = GetShipForReservation(tile, track);
		LiftShipPathReservation(s, tile, track, keep_pref_water_trackdirs);
	}

	return !HasWaterTrackReservation(tile);
}

/**
 * Check whether a tile has some preference for water trackdirs.
 * @param tile Tile to check.
 * @return true if there is some preferred trackdir.
 */
bool HasPreferredWaterTrackdirs(TileIndex tile)
{
	assert(WaterTrackMayExist(tile));

	switch (GetTileType(tile)) {
		case MP_STATION: return GB(t.m7(), 7, 1);
		case MP_TUNNELBRIDGE: return GB(_m[tile].m3, 7, 1);
		default: return GB(_m[tile].m2, 15, 1);
	}
}

/**
 * Get the preferred trackdirs for a water tile, if any is set.
 * @param tile Tile to check.
 * @return trackdir bits preferred on tile.
 */
TrackdirBits GetPreferredWaterTrackdirs(TileIndex tile)
{
	assert(WaterTrackMayExist(tile));

	switch (GetTileType(tile)) {
		case MP_RAILWAY:
			return (HasBit(_m[tile].m2, 12) ? TRACKDIR_BIT_MASK_ES : TRACKDIR_BIT_NONE) |
					(HasBit(_m[tile].m2, 13) ? TRACKDIR_BIT_MASK_WN : TRACKDIR_BIT_NONE);
		case MP_WATER:
			return (TrackdirBits)((GB(t.m6(), 2, 6) << 8) |
					(GB(t.m6(), 0, 2) << 4) | GB(t.m7(), 4, 4));
		case MP_STATION:
			if (IsBuoy(tile)) {
				return (TrackdirBits)(GB(tile.m5(), 2, 6) << 8 | GB(tile.m5(), 0, 2) << 4 |
						GB(t.m6(), 0, 3) << 1 | GB(t.m7(), 6, 1));
			} else {
				assert(IsDock(tile));
				return (TrackdirBits)(GB(tile.m4, 2, 2) << 8 | GB(tile.m4, 0, 2));
			}
		case MP_TUNNELBRIDGE: {
			TrackdirBits trackdirs = TrackBitsToTrackdirBits(DiagDirToDiagTrackBits(GetTunnelBridgeDirection(tile)));
			if (GB(_m[tile].m3, 5, 1) == 0) trackdirs &= TRACKDIR_BIT_MASK_WN;
			if (GB(_m[tile].m3, 6, 1) == 0) trackdirs &= TRACKDIR_BIT_MASK_ES;
			return trackdirs;
		}
		default:
			NOT_REACHED();
	}
}

/**
 * Set some trackdir bits to a given value (1 preferred, 0 not preferred).
 * @param tile Tile to modify.
 * @param change_trackdirs TrackdirBits to modify.
 * @param preference Value to set (1 preferred, 0 not preferred).
 */
void SetPreferredWaterTrackdirs(TileIndex tile, TrackdirBits change_trackdirs, bool preference)
{
	assert(WaterTrackMayExist(tile));
	TrackdirBits present_trackdirs = GetPreferredWaterTrackdirs(tile);

	if (preference == true) {
		present_trackdirs |= change_trackdirs;
	} else {
		present_trackdirs &= ~change_trackdirs;
	}

	// Save updated trackdirs.
	switch (GetTileType(tile)) {
		case MP_RAILWAY:
			SB(_m[tile].m2, 12, 1, ((present_trackdirs & TRACKDIR_BIT_MASK_ES) != TRACKDIR_BIT_NONE));
			SB(_m[tile].m2, 13, 1, ((present_trackdirs & TRACKDIR_BIT_MASK_WN) != TRACKDIR_BIT_NONE));
			break;
		case MP_WATER:
			SB(t.m6(), 0, 2, present_trackdirs >> 4);
			SB(t.m6(), 2, 6, present_trackdirs >> 8);
			SB(t.m7(), 4, 4, present_trackdirs);
			break;
		case MP_STATION:
			if (IsBuoy(tile)) {
				SB(tile.m5(),  0, 2, present_trackdirs >> 4);
				SB(tile.m5(),  2, 6, present_trackdirs >> 8);
				SB(t.m6(), 0, 3, present_trackdirs >> 1);
				SB(t.m7(), 6, 1, present_trackdirs);
			} else {
				assert(IsDock(tile));
				SB(tile.m4,  0, 2, present_trackdirs);
				SB(tile.m4,  2, 2, present_trackdirs >> 8);
			}
			break;
		case MP_TUNNELBRIDGE:
			SB(_m[tile].m3,  5, 1, (present_trackdirs & TRACKDIR_BIT_MASK_ES) != TRACKDIR_BIT_NONE);
			SB(_m[tile].m3,  6, 1, (present_trackdirs & TRACKDIR_BIT_MASK_WN) != TRACKDIR_BIT_NONE);
			break;
		default:
			NOT_REACHED();
	}

	// Update whether tile has preferred water trackdirs.
	switch (GetTileType(tile)) {
		case MP_STATION:
			SB(t.m7(), 7, 1, present_trackdirs != TRACKDIR_BIT_NONE);
			return;
		case MP_TUNNELBRIDGE:
			SB(_m[tile].m3,  7, 1, present_trackdirs != TRACKDIR_BIT_NONE);
			return;
		default:
			SB(_m[tile].m2, 15, 1, present_trackdirs != TRACKDIR_BIT_NONE);
			return;
	}
}

/**
 * It clears all preferences on water trackdirs and then establishes the ones
 * specified in trackdirs. */
void ClearAndSetPreferredWaterTrackdirs(TileIndex tile, TrackdirBits trackdirs)
{
	SetPreferredWaterTrackdirs(tile, TRACKDIR_BIT_MASK, false);
	if (trackdirs != TRACKDIR_BIT_NONE) SetPreferredWaterTrackdirs(tile, trackdirs, true);
}
