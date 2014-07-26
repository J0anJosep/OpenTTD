/* $Id: airport_translation.h $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_translation.h Tables with translation values for airports. */

#ifndef AIRPORT_TRANSLATION_H
#define AIRPORT_TRANSLATION_H

#include "../track_type.h"
#include "../air_type.h"
#include "airporttile_ids.h"

struct TileTranslation {
	const AirType ground;        // The ground type for this tile.
	const bool tracks;           // The tile can contain tracks for aircraft.
	const AirportTileType type;  // Use this tile will have (terminal, tracks,...).
	const TerminalType terminal_type; // Subtype of terminal.
	const DiagDirection dir;     // Direction of runway or exit direction of hangars.
	const TrackBits trackbits;   // Tracks for this tile.
	const TrackBits runway_tracks; // Tracks that belong to a runway.
	const AirportTiles gfx_id;   // Sprite for this tile.
	const bool catchment;        // True if infrastructure does have catchment.
	const bool landing;          // True for a starting runway where planes can land.

	/* Translation for simple track tiles. */
	TileTranslation(AirType at, AirportTileType att, TrackBits trackbits) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(trackbits),
			runway_tracks(TRACK_BIT_NONE),
			gfx_id((AirportTiles)0),
			catchment(false),
			landing(false)
	{
		assert(tracks);
		assert(att == ATT_SIMPLE_TRACK);
	}

	/* Translation for terminals. */
	TileTranslation(AirType at, AirportTileType att, TerminalType type, TrackBits trackbits) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(type),
			dir(INVALID_DIAGDIR),
			trackbits(trackbits),
			runway_tracks(TRACK_BIT_NONE),
			gfx_id((AirportTiles)0),
			catchment(false),
			landing(false)
	{
		assert(att == ATT_TERMINAL);
		assert(tracks);
	}

	/* Translation for hangars. */
	TileTranslation(AirType at, AirportTileType att, DiagDirection diag_dir, TrackBits trackbits) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(diag_dir),
			trackbits(trackbits),
			runway_tracks(TRACK_BIT_NONE),
			gfx_id((AirportTiles)0),
			catchment(false),
			landing(false)
	{
		assert(att == ATT_HANGAR);
		assert(tracks);
	}

	/* Translation for runway (not end, not start). */
	TileTranslation(AirType at, AirportTileType att, TrackBits trackbits, TrackBits runway_tracks) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(trackbits),
			runway_tracks(runway_tracks),
			gfx_id((AirportTiles)0),
			catchment(false),
			landing(false)
	{
		assert(tracks);
		assert(att == ATT_RUNWAY);
		assert((~TRACK_BIT_CROSS & runway_tracks) == 0 && (runway_tracks != TRACK_BIT_NONE));
	}

	/* Translation for runway end/start). */
	TileTranslation(AirType at, AirportTileType att, TrackBits trackbits, DiagDirection dir, bool landing) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(dir),
			trackbits(trackbits),
			runway_tracks(TRACK_BIT_NONE),
			gfx_id((AirportTiles)0),
			catchment(false),
			landing(landing)
	{
		assert(att == ATT_RUNWAY_END || att == ATT_RUNWAY_START);
		assert(IsValidDiagDirection(dir));
	}

	/* Translation for infrastructure. */
	TileTranslation(AirType at, AirportTileType att, AirportTiles gfx_id, bool catchment) :
			ground(at),
			tracks(false),
			type(att),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(TRACK_BIT_NONE),
			runway_tracks(TRACK_BIT_NONE),
			gfx_id(gfx_id),
			catchment(catchment),
			landing(false)
	{
		assert(att == ATT_INFRASTRUCTURE);
	}
};

/** Tiles for Country Airfield (small) */
static const TileTranslation _translation_country_0[] = {
	TileTranslation(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE,	APT_SMALL_BUILDING_1,			true				),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE,	APT_SMALL_BUILDING_2,			true				),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE,	APT_SMALL_BUILDING_3,			true				),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE,	APT_GRASS_FENCE_NE_FLAG,		false				),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_RIGHT					),

	TileTranslation(AIRTYPE_GRAVEL,		ATT_RUNWAY_END, 	TRACK_BIT_X,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_RUNWAY,		TRACK_BIT_CROSS | TRACK_BIT_LEFT,	TRACK_BIT_X			),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_RUNWAY,		TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE,	TRACK_BIT_X			),
	TileTranslation(AIRTYPE_GRAVEL,		ATT_RUNWAY_START,	TRACK_BIT_CROSS | TRACK_BIT_UPPER,	DIAGDIR_NE,		true	),
};

/** Tiles for Commuter Airfield (small) */
static const TileTranslation _translation_commuter_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_TOWER,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_3,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_LOWER								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_UPPER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_LEFT						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_RIGHT					),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y | TRACK_BIT_LOWER						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW					),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_Y,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_Y,				DIAGDIR_NE,		true	),
};

/** Tiles for City Airport (large) */
static const TileTranslation _translation_city_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_1,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_APRON_FENCE_NW,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_2,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_PIER,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_ROUND_TERMINAL,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_3,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_PIER_NW_NE,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_UPPER								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_APRON_N_FENCE_SW,			false				),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADIO_TOWER_FENCE_NE,		false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT | TRACK_BIT_LOWER | TRACK_BIT_X				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_ARPON_N,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_TOWER_FENCE_SW,			false				),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_EMPTY_FENCE_NE,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_LOWER | TRACK_BIT_Y						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_UPPER | TRACK_BIT_LEFT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT | TRACK_BIT_Y						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_APRON_E,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADAR_GRASS_FENCE_SW,		false				),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_X,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_CROSS,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_CROSS,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_X,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_X,				DIAGDIR_NE,		true	),
};

/** Tiles for Metropolitain Airport (large) - 2 runways */
static const TileTranslation _translation_metropolitan_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_1,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_APRON_FENCE_NW,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_2,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_PIER,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_ROUND_TERMINAL,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_3,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_PIER_NW_NE,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y | TRACK_BIT_LOWER						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y | TRACK_BIT_UPPER						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_APRON_N_FENCE_SW,			false				),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADAR_FENCE_NE,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_HORZ						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_TOWER_FENCE_SW,	false						),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_NONE,				DIAGDIR_NE,		false	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_LOWER | TRACK_BIT_Y,		TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_UPPER,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_LEFT,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_CROSS,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_X,				DIAGDIR_NE,		false	),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_X,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_CROSS,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_NONE,				DIAGDIR_NE,		true	),
};

/** Tiles for International Airport (large) - 2 runways */
static const TileTranslation _translation_international_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_X,				DIAGDIR_SW,		false	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_CROSS,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_RIGHT,			TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_NONE,				DIAGDIR_SW,		false	),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADIO_TOWER_FENCE_NE,		false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_RIGHT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_X								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LOWER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT | TRACK_BIT_CROSS					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_3,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_2,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_LEFT						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_RIGHT					),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_2,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_CROSS			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LOWER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_UPPER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_TOWER,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_LOWER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_CROSS			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y | TRACK_BIT_LOWER | TRACK_BIT_LEFT				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_UPPER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_X								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LEFT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_UPPER | TRACK_BIT_CROSS					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADAR_FENCE_SW,			false				),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_Y,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_NONE,				DIAGDIR_NE,		true	),
};

/** Tiles for International Airport (large) - 2 runways */
static const TileTranslation _translation_intercontinental_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADAR_FENCE_NE, 			true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_NONE,				DIAGDIR_SW,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_Y,				DIAGDIR_SW,		true	),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_Y,				DIAGDIR_SW,		false	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_NONE,				DIAGDIR_SW,		false	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_EMPTY,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_X								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LOWER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT | TRACK_BIT_CROSS					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_RADIO_TOWER_FENCE_NE,		false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_RIGHT | TRACK_BIT_X						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_TOWER,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_UPPER | TRACK_BIT_RIGHT			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_X								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_LEFT								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_LEFT | TRACK_BIT_X						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_HORZ					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_1,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_LOW_BUILDING,			true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_LOW_BUILDING,			true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_2,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_LEFT						),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_RIGHT					),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LOWER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_UPPER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_BUILDING_3,				true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW					),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LEFT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_ALL & ~TRACK_BIT_RIGHT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_ROUND_TERMINAL,			false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_TERMINAL,				TRACK_BIT_CROSS			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_VERT | TRACK_BIT_UPPER			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_LEFT | TRACK_BIT_X | TRACK_BIT_UPPER				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_GRASS_FENCE_NE_FLAG_2,		false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_LEFT | TRACK_BIT_CROSS					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_UPPER					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_X								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LEFT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_UPPER | TRACK_BIT_CROSS					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_EMPTY,				false				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y								),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_Y								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_NONE,				DIAGDIR_NE,		false	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START, 	TRACK_BIT_Y,				DIAGDIR_NE,		false	),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,		TRACK_BIT_Y,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY,		TRACK_BIT_NONE,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_RUNWAY_START,	TRACK_BIT_NONE,				DIAGDIR_NE,		true	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_EMPTY,				true				),
};

/** Tiles for Heliport */
static const TileTranslation _translation_heliport_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPORT,				TRACK_BIT_NONE			),
};

/** Tiles for Helidepot */
static const TileTranslation _translation_helidepot_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_LOW_BUILDING_FENCE_N,		true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_X			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
};

/** Tiles for Helistation */
static const TileTranslation _translation_helistation_0[] = {
	TileTranslation(AIRTYPE_ASPHALT,	ATT_HANGAR,		DIAGDIR_SE,				TRACK_BIT_Y			),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE,	APT_LOW_BUILDING_FENCE_NW,		true				),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_Y | TRACK_BIT_LOWER	),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_CROSS	),

	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_X								),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,	TRACK_BIT_CROSS | TRACK_BIT_LEFT					),
	TileTranslation(AIRTYPE_ASPHALT,	ATT_TERMINAL,		HTT_HELIPAD,				TRACK_BIT_X | TRACK_BIT_UPPER	),
};

/** Tiles for Oil rig */
static const TileTranslation _translation_oilrig_0[] = {
	TileTranslation(AIRTYPE_WATER,		ATT_TERMINAL,		HTT_BUILTIN_HELIPORT,			TRACK_BIT_NONE			),
};

static const TileTranslation *_translation_airport_specs[] = {
	_translation_country_0,
	_translation_city_0,
	_translation_heliport_0,
	_translation_metropolitan_0,
	_translation_international_0,
	_translation_commuter_0,
	_translation_helidepot_0,
	_translation_intercontinental_0,
	_translation_helistation_0,
	_translation_oilrig_0,
};

static const bool _translation_airport_hangars[] = {
	1,
	1,
	0,
	1,
	1,
	1,
	1,
	1,
	1,
	0,
};

static const bool _translation_airport_heliport[] = {
	0,
	0,
	1,
	0,
	0,
	0,
	1,
	0,
	1,
	0,
};

assert_compile(NEW_AIRPORT_OFFSET == lengthof(_translation_airport_hangars));
assert_compile(NEW_AIRPORT_OFFSET == lengthof(_translation_airport_specs));

#endif /* AIRPORT_TRANSLATION_H */
