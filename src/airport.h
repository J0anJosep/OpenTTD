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

/** Flags for airport movement data. */
enum AirportMovingDataFlags {
	AMED_NOSPDCLAMP = 1 << 0, ///< No speed restrictions.
	AMED_TAKEOFF    = 1 << 1, ///< Takeoff movement.
	AMED_SLOWTURN   = 1 << 2, ///< Turn slowly (mostly used in the air).
	AMED_LAND       = 1 << 3, ///< Landing onto landing strip.
	AMED_EXACTPOS   = 1 << 4, ///< Go exactly to the destination coordinates.
	AMED_BRAKE      = 1 << 5, ///< Taxiing at the airport.
	AMED_HELI_RAISE = 1 << 6, ///< Helicopter take-off.
	AMED_HELI_LOWER = 1 << 7, ///< Helicopter landing.
	AMED_HOLD       = 1 << 8, ///< Holding pattern movement (above the airport).
};

/** A single location on an airport where aircraft can move to. */
struct AirportMovingData {
	int16_t x;             ///< x-coordinate of the destination.
	int16_t y;             ///< y-coordinate of the destination.
	uint16_t flag;         ///< special flags when moving towards the destination.
	Direction direction; ///< Direction to turn the aircraft after reaching the destination.
};

AirportMovingData RotateAirportMovingData(const AirportMovingData *orig, Direction rotation, uint num_tiles_x, uint num_tiles_y);

struct AirportFTAbuildup;

/** Finite sTate mAchine (FTA) of an airport. */
struct AirportFTAClass {
public:
	/** Bitmask of airport flags. */
	enum Flags {
		AIRPLANES   = 0x1,                     ///< Can planes land on this airport type?
		HELICOPTERS = 0x2,                     ///< Can helicopters land on this airport type?
		ALL         = AIRPLANES | HELICOPTERS, ///< Mask to check for both planes and helicopters.
		SHORT_STRIP = 0x4,                     ///< This airport has a short landing strip, dangerous for fast aircraft.
	};

	AirportFTAClass(
		const AirportMovingData *moving_data,
		const uint8_t *terminals,
		const uint8_t num_helipads,
		const uint8_t *entry_points,
		Flags flags,
		const AirportFTAbuildup *apFA,
		uint8_t delta_z
	);

	~AirportFTAClass();

	/**
	 * Get movement data at a position.
	 * @param position Element number to get movement data about.
	 * @return Pointer to the movement data.
	 */
	const AirportMovingData *MovingData(uint8_t position) const
	{
		assert(position < nofelements);
		return &moving_data[position];
	}

	const AirportMovingData *moving_data; ///< Movement data.
	struct AirportFTA *layout;            ///< state machine for airport
	const uint8_t *terminals;                ///< %Array with the number of terminal groups, followed by the number of terminals in each group.
	const uint8_t num_helipads;              ///< Number of helipads on this airport. When 0 helicopters will go to normal terminals.
	Flags flags;                          ///< Flags for this airport type.
	uint8_t nofelements;                     ///< number of positions the airport consists of
	const uint8_t *entry_points;             ///< when an airplane arrives at this airport, enter it at position entry_point, index depends on direction
	uint8_t delta_z;                         ///< Z adjustment for helicopter pads
};

DECLARE_ENUM_AS_BIT_SET(AirportFTAClass::Flags)


/** Internal structure used in openttd - Finite sTate mAchine --> FTA */
struct AirportFTA {
	AirportFTA *next;        ///< possible extra movement choices from this position
	uint64_t block;            ///< 64 bit blocks (st->airport.flags), should be enough for the most complex airports
	uint8_t position;           ///< the position that an airplane is at
	uint8_t next_position;      ///< next position from this position
	uint8_t heading;            ///< heading (current orders), guiding an airplane to its target on an airport
};

const AirportFTAClass *GetAirport(const uint8_t airport_type);
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
