/* $Id: pbs_rail.h $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_rail.h PBS support routines for rail */

#ifndef PBS_RAIL_H
#define PBS_RAIL_H

#include "direction_type.h"
#include "vehicle_type.h"
#include "pbs.h"

TrackBits GetReservedTrackbits(TileIndex t);

bool TryReserveRailTrack(TileIndex tile, Track t, bool trigger_stations = true);
void UnreserveRailTrack(TileIndex tile, Track t);

PBSTileInfo FollowTrainReservation(const Train *v, Vehicle **train_on_res = NULL);
Train *GetTrainForReservation(TileIndex tile, Track track);

bool IsSafeWaitingPosition(const Train *v, TileIndex tile, Trackdir trackdir, bool include_line_end, bool forbid_90deg = false);
bool IsWaitingPositionFree(const Train *v, TileIndex tile, Trackdir trackdir, bool forbid_90deg = false);

/**
 * Check whether some of tracks is reserved on a tile.
 * @param tile The tile.
 * @param tracks The tracks to test.
 * @return true if at least on of the tracks \a tracks is reserved on tile \a tile.
 */
static inline bool HasReservedTracks(TileIndex tile, TrackBits tracks)
{
	return (GetReservedTrackbits(tile) & tracks) != TRACK_BIT_NONE;
}

#endif /* PBS_RAIL_H */
