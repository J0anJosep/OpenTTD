/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport.h Various declarations for airports */

#ifndef AIRPORT_H
#define AIRPORT_H

#include "direction_type.h"
#include "track_type.h"
#include "tile_type.h"

/** Some airport-related constants */
static const uint MAX_TERMINALS =   8;                       ///< maximum number of terminals per airport
static const uint MAX_HELIPADS  =   3;                       ///< maximum number of helipads per airport
static const uint MAX_ELEMENTS  = 255;                       ///< maximum number of aircraft positions at airport

static const uint NUM_AIRPORTTILES_PER_GRF = 255;            ///< Number of airport tiles per NewGRF; limited to 255 to allow extending Action3 with an extended byte later on.

static const uint NUM_AIRPORTTILES       = 256;              ///< Total number of airport tiles.
static const uint NEW_AIRPORTTILE_OFFSET = 74;               ///< offset of first newgrf airport tilex
static const uint NUM_AIRTYPE_INFRATILES = 11;               ///< Total number of infrastructure tiles by airtype.

/** Airport types */
enum AirportTypes {
	AT_SMALL           =   0, ///< Small airport.
	AT_LARGE           =   1, ///< Large airport.
	AT_HELIPORT        =   2, ///< Heli port.
	AT_METROPOLITAN    =   3, ///< Metropolitan airport.
	AT_INTERNATIONAL   =   4, ///< International airport.
	AT_COMMUTER        =   5, ///< Commuter airport.
	AT_HELIDEPOT       =   6, ///< Heli depot.
	AT_INTERCON        =   7, ///< Intercontinental airport.
	AT_HELISTATION     =   8, ///< Heli station airport.
	AT_OILRIG          =   9, ///< Oilrig airport.
	NEW_AIRPORT_OFFSET =  10, ///< Number of the first newgrf airport.
	NUM_AIRPORTS_PER_GRF = 128, ///< Maximal number of airports per NewGRF.
	NUM_AIRPORTS       = 128, ///< Maximal number of airports in total.
	AT_CUSTOM          = 253, ///< Customized airport.
	AT_INVALID         = 254, ///< Invalid airport.
	AT_DUMMY           = 255, ///< Dummy airport.
};

uint8_t GetVehiclePosOnBuild(TileIndex hangar_tile);

TrackBits GetAllowedTracks(TileIndex tile);
void SetRunwayReservation(TileIndex tile, bool b);
TileIndex GetRunwayExtreme(TileIndex tile, DiagDirection dir);
uint GetRunwayLength(TileIndex tile);

enum AirportFlagBits : uint8_t {
	AFB_CLOSED_MANUAL         = 0,   ///< Airport closed: manually closed.
	AFB_HANGAR                = 1,   ///< Airport has at least one hangar tile.
	AFB_LANDING_RUNWAY        = 2,   ///< Airport has a landing runway.
};

enum AirportFlags : uint16_t {
	AF_NONE                =  0,     ///< No flag.
	AF_CLOSED_MANUAL       =  1 << AFB_CLOSED_MANUAL,
	AF_HANGAR              =  1 << AFB_HANGAR,
	AF_LANDING_RUNWAY      =  1 << AFB_LANDING_RUNWAY,
};
DECLARE_ENUM_AS_BIT_SET(AirportFlags)

#endif /* AIRPORT_H */
