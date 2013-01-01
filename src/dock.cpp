/* $Id: dock.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dock.cpp Implementation of the dock base class. */

#include "stdafx.h"
#include "core/pool_func.hpp"
#include "dock_base.h"
#include "station_base.h"

/** The pool of docks. */
DockPool _dock_pool("Dock");
INSTANTIATE_POOL_METHODS(Dock)

/**
 * Find a dock at a given tile.
 * @param tile Tile with a dock.
 * @return The dock in the given tile.
 * @pre IsDockTile()
 */
/* static */ Dock *Dock::GetByTile(TileIndex tile)
{
	const Station *st = Station::GetByTile(tile);

	for (Dock *d = st->GetPrimaryDock();; d = d->next) {
		if (d->sloped == tile || d->flat == tile) return d;
		assert(d->next != NULL);
	}
	NOT_REACHED();
}

/**
 * Set the tracks of a dock that a ship can cross.
 * @param
 * @param tracks TrackBits that can be crossed.
 * @pre tracks == TRACK_BIT_X, TRACK_BIT_Y or TRACK_BIT_CROSS
 */
void SetDockTracks(TileIndex t, TrackBits tracks)
{
	assert(tracks == TRACK_BIT_X || tracks == TRACK_BIT_Y || tracks == TRACK_BIT_CROSS);
	assert(IsValidTile(t) && GetTileSlope(t) == SLOPE_FLAT && IsDockTile(t));

	SB(_me[t].m7, 4, 2, tracks);
}

/**
 * Rotate the passable tracks of a dock tile.
 */
void RotateDockTracks(TileIndex t)
{
	TrackBits tracks = GetDockTracks(t);
	switch (tracks) {
		case TRACK_BIT_X:
			tracks = TRACK_BIT_Y;
			break;
		case TRACK_BIT_Y:
			tracks = TRACK_BIT_CROSS;
			break;
		case TRACK_BIT_CROSS:
			tracks = TRACK_BIT_X;
			break;
		default: NOT_REACHED();
	}

	SetDockTracks(t, tracks);
}

/**
 * Get the tracks of a dock that can be used by water vehicles
 */
TrackBits GetDockTracks(TileIndex t)
 {
	assert(IsValidTile(t) && GetTileSlope(t) == SLOPE_FLAT && IsDockTile(t));

	return (TrackBits)GB(_me[t].m7, 4, 2);
 }