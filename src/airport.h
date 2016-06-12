/* $Id$ */

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
#include "tile_type.h"


static const uint NUM_AIRPORTTILES_PER_GRF = 255;            ///< Number of airport tiles per NewGRF; limited to 255 to allow extending Action3 with an extended byte later on.

static const uint NUM_AIRPORTTILES       = 256;              ///< Total number of airport tiles.
static const uint NEW_AIRPORTTILE_OFFSET = 200;              ///< offset of first newgrf airport tile
static const uint INVALID_AIRPORTTILE    = NUM_AIRPORTTILES; ///< id for an invalid airport tile

static const uint LONG_RUNWAY_LENGTH     = 6;                ///< Runways with 5 or less tiles are considered short.

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
	AT_INVALID         = 253, ///< Invalid airport.
	AT_CUSTOM          = 254, ///< Customized airport.
	AT_DUMMY           = 255, ///< Dummy airport.
};

enum AirportFlags {
	AF_NONE                =  0,       ///< No flag.
	AF_CLOSED_MANUAL       =  1 <<  0, ///< The airport is closed manually.
	AF_CLOSED_ZEP_CRASH    =  1 <<  1, ///< The airport is closed due to a zeppelin crash.
	AF_CLOSED_REG_CRASH    =  1 <<  2, ///< The airport is closed due to a regular aircraft crash.
	AF_CLOSED_DESIGN       =  1 <<  3, ///< The airport is closed due to a design modification.
	AF_CLOSED              =  AF_CLOSED_MANUAL | AF_CLOSED_ZEP_CRASH | AF_CLOSED_REG_CRASH | AF_CLOSED_DESIGN,

	AF_HELIPADS            =  1 <<  8, ///< Airport has helipads.
	AF_HELIPORTS           =  1 <<  9, ///< Airport has heliports.
	AF_HELICOPTERS         =  AF_HELIPADS | AF_HELIPORTS,

	/* Short: 5 or less tiles. */
	AF_SHORT_TAKEOFF       =  1 << 10, ///< Airport has a short takeoff runway.
	AF_LONG_TAKEOFF        =  1 << 11, ///< Airport has a long takeoff runway.
	AF_TAKEOFF             =  AF_SHORT_TAKEOFF | AF_LONG_TAKEOFF,

	AF_SHORT_LANDING       =  1 << 12, ///< Airport has a short landing runway.
	AF_LONG_LANDING        =  1 << 13, ///< Airport has a long landing runway.
	AF_LANDING             =  AF_SHORT_LANDING | AF_LONG_LANDING,

	AF_STRUCTURE_MASK      =  AF_HELICOPTERS | AF_TAKEOFF | AF_LANDING,
};

#endif /* AIRPORT_H */
