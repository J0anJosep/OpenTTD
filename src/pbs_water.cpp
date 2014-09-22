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

/**
 * Follow a reservation starting from a specific tile to its end.
 * @param o Owner (unused: as ships can cross tiles of other owners).
 * @param rts Former railtypes (unused: can be converted to water_types? sea, river, canal).
 * @param tile Start tile.
 * @param trackdir Track direction to look for.
 * @param ignore_oneway Unused: former ignore one-way signals for trains.
 * @return Last tile and info of the reserved path.
 */
static PBSTileInfo FollowShipReservation(Owner o, RailTypes rts, TileIndex tile, Trackdir trackdir, bool ignore_oneway = false)
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

	/** Init the best ship to NULL always! */
	FindShipOnTrackInfo() : best(NULL) {}
};

/**
 * Callback for Has/FindVehicleOnPos to find a train on a specific track.
 * @param v Vehicle to test.
 * @param data FindshipOnTrackInfo.
 * @return The vehicle on track or NULL if none found.
 */
static Vehicle *FindShipOnTrackEnum(Vehicle *v, void *data)
{
	FindShipOnTrackInfo *info = (FindShipOnTrackInfo *)data;

	if (v->type != VEH_SHIP || v->IsInDepot()) return NULL;

	Ship *s = Ship::From(v);
	TrackBits tracks = s->state;
	if (tracks == TRACK_BIT_WORMHOLE || HasBit(tracks, TrackdirToTrack(info->res.trackdir))) {
		/* ALWAYS return the lowest ID (anti-desync!) */
		if (info->best == NULL || s->index < info->best->index) info->best = s;
		return s;
	}

	return NULL;
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
 * @return The vehicle holding the reservation or NULL if the path is stray.
 */
Ship *GetShipForReservation(TileIndex tile, Track track)
{
	assert(HasWaterTracksReserved(tile, TrackToTrackBits(track)));
	Trackdir trackdir = TrackToTrackdir(track);

	/* Follow the path from tile to both ends.
	 * One of the end tiles should have a ship on it. */
	for (int i = 0; i < 2; ++i, trackdir = ReverseTrackdir(trackdir)) {
		FindShipOnTrackInfo fsoti;
		fsoti.res = FollowShipReservation(GetTileOwner(tile), (RailTypes)0, tile, trackdir, true);

		FindVehicleOnPos(fsoti.res.tile, &fsoti, FindShipOnTrackEnum);
		if (fsoti.best != NULL) return fsoti.best;

		/* Special case for bridges/tunnels: check the other end as well. */
		if (IsTileType(fsoti.res.tile, MP_TUNNELBRIDGE)) {
			FindVehicleOnPos(GetOtherTunnelBridgeEnd(fsoti.res.tile), &fsoti, FindShipOnTrackEnum);
			if (fsoti.best != NULL) return fsoti.best;
		}
	}

	return NULL;
}

/**
 * Remove a reservation starting on given tile with a given trackdir.
 * @param tile Starting tile.
 * @param trackdir Starting trackdir.
 */
void LiftPathReservation(TileIndex tile, Trackdir trackdir)
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
 * @param tile The tile that asks the path to be freed.
 * @param track The track that asks to be freed.
 * @return True if at least a track is freed.
 * @note The path will not be freed if the ship tile and track is
 *       the same as the tile and track that ask to free the path.
 * @note Immediately after game conversion, the game will create and lift some lost reservations.
 */
bool TryLiftShipReservation(const Ship *v, TileIndex tile, Track track)
{
	if (v == NULL) DEBUG(misc, 0, "Lifting a lost reservation.");

	bool reservation_removed = false;

	if (v != NULL && v->tile == tile && TrackdirToTrack(v->GetVehicleTrackdir()) == track) return false;

	/* Do not disallow 90 deg turns as the setting might have changed between reserving and now. */
	CFollowTrackWater fs(v == NULL ? INVALID_COMPANY : v->owner);

	Trackdir trackdir;
	if (v != NULL) {
		tile = v->tile;
		trackdir = v->GetVehicleTrackdir();
		/* Skip first tile of a tunnel */
		if (IsTileType(tile, MP_TUNNELBRIDGE) && GetTunnelBridgeDirection(tile) == TrackdirToExitdir(trackdir)) {
			fs.Follow(tile, trackdir);
			tile = fs.m_new_tile;
			trackdir = FindFirstTrackdir(fs.m_new_td_bits & TrackBitsToTrackdirBits(GetReservedWaterTracks(tile)));
		}
	} else {
		trackdir = TrackToTrackdir(track);
		if (!SetWaterTrackReservation(tile, track, false)) NOT_REACHED();
		reservation_removed = true;
	}

	bool check_first = (v != NULL);

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
			if (tile == v->dest_tile) {
				return false;
			}
			check_first = false;
		}

		if (!SetWaterTrackReservation(tile, TrackdirToTrack(trackdir), false)) NOT_REACHED();
	}

	return reservation_removed || v->tile != tile;
}

/**
 * Before extending a reservation, remove colliding paths when necessary.
 * @param tile First tile we want to reserve.
 * @param trackdirs Track directions we want to check for reservations.
 *                  (trackdirs we reach when entering \a tile)
 */
void LiftReservations(TileIndex tile, TrackdirBits trackdirs)
{
	/* First, check only on tile */
	if (HasWaterTrackReservation(tile)) {
		/* Only one track can be reserved as we are currently reaching the tile */
		// revise
		TrackBits tracks = GetReservedWaterTracks(tile);
		Track track = RemoveFirstTrack(&tracks);
		assert(tracks == TRACK_BIT_NONE);
		if (track != INVALID_TRACK) TryLiftShipReservation(GetShipForReservation(tile, track), tile, track);
	}

	CFollowTrackWater ft(INVALID_COMPANY);

	for (Trackdir trackdir = RemoveFirstTrackdir(&trackdirs); trackdir != INVALID_TRACKDIR; trackdir = RemoveFirstTrackdir(&trackdirs)) {
		/* Check the next tile, if a path collides, try unreserve it. */
		if (!ft.Follow(tile, trackdir)) continue;

		/* Check for reachable tracks.
		 * Don't discard 90deg turns as we don't want two paths to collide
		 * even if they cannot really collide because of a 90deg turn */
		ft.m_new_td_bits = DiagdirReachesTrackdirs(ft.m_exitdir) &
				TrackBitsToTrackdirBits(GetReservedWaterTracks(ft.m_new_tile));

		if (ft.m_new_td_bits != TRACKDIR_BIT_NONE) {
			Track track = TrackBitsToTrack(TrackdirBitsToTrackBits(ft.m_new_td_bits));
			TryLiftShipReservation(GetShipForReservation(ft.m_new_tile, track), ft.m_new_tile, track);
		}
	}
}

/**
 * Free ship paths on a tile.
 * @param tile Tile we want to free.
 * @return True if tile has no reservation after the paths have been freed.
 */
bool LiftReservations(TileIndex tile)
{
	if (!HasWaterTrackReservation(tile)) return true;

	Track track;
	FOR_EACH_SET_TRACK(track, GetReservedWaterTracks(tile)) {
		Ship *s = GetShipForReservation(tile, track);
		TryLiftShipReservation(s, tile, track);
	}

	return !HasWaterTrackReservation(tile);
}
