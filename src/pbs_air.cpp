/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_air.cpp Path based system routines for air vehicles. */

#include "stdafx.h"
#include "air_map.h"
#include "aircraft.h"
#include "pathfinder/follow_track.hpp"

#include "safeguards.h"

/**
 * When arriving at the end of a landing runway, choose an appropriate trackdir.
 * Or when an helicopter lands in an apron, choose an appropriate one.
 * @param tile
 * @param preferred_trackdir
 * @return a valid free trackdir (\a preferred_trackdir if possible).
 */
Trackdir GetFreeAirportTrackdir(TileIndex tile, Trackdir preferred_trackdir)
{
	if (tile == INVALID_TILE) return INVALID_TRACKDIR;
	assert(IsValidTrackdir(preferred_trackdir));
	assert(IsDiagonalTrackdir(preferred_trackdir));
	assert(IsAirportTile(tile));
	assert(MayHaveAirTracks(tile));
	assert(IsApron(tile) || (IsRunwayExtreme(tile) && IsRunwayEnd(tile)));
	if (HasAirportTrackReserved(tile)) return INVALID_TRACKDIR;

	TrackBits tracks = GetAirportTileTracks(tile) & TRACK_BIT_CROSS;
	if (tracks == TRACK_BIT_NONE) return INVALID_TRACKDIR;

	Track preferred_track = TrackdirToTrack(preferred_trackdir);
	if (HasTrack(tracks, preferred_track)) return preferred_trackdir;

	tracks &= ~TrackToTrackBits(preferred_track);
	if (tracks == TRACK_BIT_NONE) return INVALID_TRACKDIR;

	preferred_track = RemoveFirstTrack(&tracks);

	/* Get one trackdir of the two available trackdirs, better if the trackdir reaches tracks on next tile. */
	CFollowTrackAirport fs(INVALID_COMPANY);
	Trackdir trackdir = TrackToTrackdir(preferred_track);
	if (fs.Follow(tile, trackdir)) return trackdir;
	return ReverseTrackdir(trackdir);
}

/**
 * Remove reservation of given aircraft.
 * @param v vehicle that frees some reservations of tracks.
 * @param skip_first_track whether skip the first reserved vehicle track.
 */
void LiftAirportPathReservation(Aircraft *v, bool skip_first_track)
{
	if ((v->vehstatus & VS_HIDDEN) != 0) return;

	if (IsHeliportTile(v->tile)) {
		/* Special case for heliports. */
		assert(IsValidTrackdir(v->trackdir));
		assert(IsDiagonalTrackdir(v->trackdir));
		if (!skip_first_track) RemoveAirportTrackReservation(v->tile, TrackdirToTrack(v->trackdir));
		return;
	}

	/* If not rotating, v->trackdir is the first trackdir.
	 * If rotating, v->next_trackdir contains the first trackdir (once it has rotated). */
	Trackdir trackdir = v->next_trackdir == INVALID_TRACKDIR ? v->trackdir : v->next_trackdir;
	assert(IsValidTrackdir(trackdir));
	TileIndex tile = v->tile;
	assert(IsAirportTile(tile));
	assert(MayHaveAirTracks(tile));

	CFollowTrackAirport fs(INVALID_COMPANY);
	for (;;) {
		assert(IsAirportTile(tile));
		assert(MayHaveAirTracks(tile));
		Track track = TrackdirToTrack(trackdir);
		assert(HasAirportTrackReserved(tile, track));
		RemoveAirportTrackReservation(tile, track);
		TrackBits reserved = GetReservedAirportTracks(tile);

		/* Find next part of the path. */
		if ((reserved | TrackToTrackBits(track)) == TRACK_BIT_CROSS) {
			/* Path continues in the same tile (middle tile rotation). */
			assert(!v->path.empty());
			assert(v->path.tile.front() == tile);
			trackdir = v->path.td.front();
			v->path.pop_front();
			assert(IsValidTrackdir(trackdir));
			continue;
		}

		DiagDirection exit_dir = TrackdirToExitdir(trackdir);
		TrackdirBits edge_trackdirs = DiagdirReachesTrackdirs(ReverseDiagDir(exit_dir)) &
				TrackBitsToTrackdirBits(reserved);
		if (edge_trackdirs != TRACKDIR_BIT_NONE) {
			assert(CountBits(edge_trackdirs) == 1);
			trackdir = FindFirstTrackdir(edge_trackdirs);
			/* Path continues in the same tile (rotation at the edge of the tile). */
			continue;
		}

		if (!fs.Follow(tile, trackdir)) {
			/* Can't follow path. Path end. */
			assert(IsDiagonalTrackdir(trackdir));
			break;
		}

		/* Path may continue ahead. Get the corresponding tile and trackdir, if any. */
		fs.m_new_td_bits &= TrackBitsToTrackdirBits(GetReservedAirportTracks(fs.m_new_tile));
		assert(CountBits(fs.m_new_td_bits) < 2);
		if (fs.m_new_td_bits == TRACKDIR_BIT_NONE) {
			/* Path reservation ended. */
			assert(IsDiagonalTrackdir(trackdir));
			break;
		}

		tile = fs.m_new_tile;
		trackdir = FindFirstTrackdir(fs.m_new_td_bits);
	}

	if (skip_first_track) {
		/* Full path unreserved, but must keep the first reserved track.
		 * Reserve it again. */
		trackdir = v->next_trackdir == INVALID_TRACKDIR ? v->trackdir : v->next_trackdir;
		SetAirportTrackReservation(v->tile, TrackdirToTrack(trackdir));
	}

	assert(v->path.empty());
}
