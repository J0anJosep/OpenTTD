/* $Id: pbs_air.h $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_air.h PBS support routines for aircraft crossing tiles of an airport or just flying. */

#ifndef PBS_AIR_H
#define PBS_AIR_H

#include "air_type.h"
#include "direction_type.h"
#include "vehicle_type.h"
#include "pbs.h"

/* Station related functions. */
void AfterLoadSetAirportTileTypes();

/* Routines related to reservation of air tracks on a tile. */
TrackBits GetAllowedTracks(TileIndex tile);
bool CheckFreeAssociatedAirportTile(TileIndex tile, Track track);
bool TryAirportTrackReservation(TileIndex t, Track track);
bool RemoveAirportTrackReservation(TileIndex t, Track track);
bool HasAirportTrackReservation(TileIndex t);
bool HasAirportTracksReserved(TileIndex t, TrackBits tracks);
bool DoAirportPathReservation(TileIndex t, Trackdir trackdir);
bool IsAirportPositionFree(TileIndex tile, Trackdir trackdir);

PBSTileInfo FollowAirReservation(const Aircraft *v, Vehicle **aircraft_on_res = NULL);
Aircraft *GetAircraftForReservation(TileIndex tile, Track track);
bool IsSafeWaitingPosition(const Aircraft *v, TileIndex tile, Trackdir trackdir);

#endif /* PBS_AIR_H */
