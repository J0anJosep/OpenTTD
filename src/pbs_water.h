/* $Id: pbs_water.h $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_water.h PBS water support routines. */

#ifndef PBS_WATER_H
#define PBS_WATER_H

#include "direction_type.h"
#include "vehicle_type.h"
#include "pbs.h"

/* Routines related to reservation of water tracks on a tile. */
TrackBits GetReservedWaterTracks(TileIndex t);
TrackBits GetFreeWaterTrackReservation(TileIndex t, TrackdirBits trackdirs);
void SetWaterTrackReservation(TileIndex t, TrackBits tracks);

bool WaterTrackMayExist(TileIndex t);
bool SetWaterTrackReservation(TileIndex t, Track track, bool value);
bool HasWaterTrackReservation(TileIndex t);
bool HasWaterTracksReserved(TileIndex t, TrackBits tracks);
bool IsWaterPositionFree(TileIndex tile, Trackdir trackdir);

/* Routines related to reservation of water paths. */
void LiftPathReservation(TileIndex tile, Trackdir trackdir);
void LiftReservations(TileIndex tile, TrackdirBits trackdirs);
bool LiftReservations(TileIndex tile);

#endif /* PBS_WATER_H */
