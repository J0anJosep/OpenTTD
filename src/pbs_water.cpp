/* $Id: pbs_water.cpp $ */

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

	byte track_b = GB(_me[t].m7, 0, 3);
	if (track_b == 0) return TRACK_BIT_NONE;
	Track track = (Track)(track_b - 1);    // map array saves Track+1
	return TrackToTrackBits(track) | (HasBit(_me[t].m7, 3) ? TrackToTrackBits(TrackToOppositeTrack(track)) : TRACK_BIT_NONE);
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
	Track track = RemoveFirstTrack(&tracks);
	SB(_me[t].m7, 0, 3, track == INVALID_TRACK ? 0 : track + 1);
	SB(_me[t].m7, 3, 1, tracks != TRACK_BIT_NONE);
}

/**
 * Return true if a ship can try to cross the tile.
 * @tile Tile to check.
 * @return True if the tile may contain water tracks, false otherwise.
 */
bool WaterTrackMayExist(TileIndex t)
{
	return t != INVALID_TILE && (
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

	if (IsTileType(t, MP_TUNNELBRIDGE)) {
		assert(IsDiagonalTrack(track));
		if (value == HasTunnelBridgeReservation(t)) return false;
		SetTunnelBridgeReservation(t, value);
		MarkTileDirtyByTile(t);
		t = GetOtherTunnelBridgeEnd(t);
		SetTunnelBridgeReservation(t, value);
		MarkTileDirtyByTile(t);
		return true;
	}

	TrackBits trackbits = GetReservedWaterTracks(t);

	if (value) {
		if (TrackOverlapsTracks(trackbits, track)) return false;
		trackbits |= TrackToTrackBits(track);

		SB(_me[t].m7, 0, 3, RemoveFirstTrack(&trackbits) + 1);
		SB(_me[t].m7, 3, 1, trackbits != TRACK_BIT_NONE);
	} else {
		TrackBits removing_track = TrackToTrackBits(track);
		if (!(trackbits & removing_track)) return false;
		trackbits &= ~removing_track;
		track = RemoveFirstTrack(&trackbits);
		assert(trackbits == TRACK_BIT_NONE);
		SB(_me[t].m7, 0, 3, track == INVALID_TRACK ? 0 : track + 1);
		SB(_me[t].m7, 3, 1, 0);
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
 * Reserve a path starting at a tile with a trackdir.
 * @param tile Starting tile.
 * @param trackdir Trackdir to take.
 * @return True if at least the first tile has been reserved.
 */
bool DoWaterPathReservation(TileIndex t, Trackdir trackdir)
{
	if (trackdir == INVALID_TRACKDIR) return false;
	assert(WaterTrackMayExist(t));
	Track track = TrackdirToTrack(trackdir);

	/* If track is reserved, water path is already done */
	if (HasWaterTracksReserved(t, TrackToTrackBits(track))) return true;

	if (!IsWaterPositionFree(t, trackdir)) return false;
	if (!SetWaterTrackReservation(t, track, true)) NOT_REACHED();

	return true;
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
	Track track = TrackdirToTrack(trackdir);

	/* Tile reserved? Can never be a free waiting position. */
	if (TrackCollidesTrackReservation(tile, track)) return false;

	/* Check the next tile, if a path collides, then it isn't a waiting position at all. */
	CFollowTrackWater ft(INVALID_COMPANY);

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

