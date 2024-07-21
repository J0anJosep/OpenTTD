/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_air.h PBS support routines for aircraft crossing tiles of an airport or just flying. */

#ifndef PBS_AIR_H
#define PBS_AIR_H

#include "track_type.h"

struct Aircraft;

Trackdir GetFreeAirportTrackdir(TileIndex tile, Trackdir preferred_trackdir);
void LiftAirportPathReservation(Aircraft *v, bool skip_first_track);

#endif /* PBS_AIR_H */
