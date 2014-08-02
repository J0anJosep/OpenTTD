/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file airtypes.h
 * All the airtype-specific information is stored here.
 */

#ifndef AIRTYPES_H
#define AIRTYPES_H

/**
 * Global AirType definition
 */
static const AirTypeInfo _original_airtypes[] = {
	/** Gravel */
	{ // Main Sprites
		{	SPR_RAIL_TRACK_Y,
			SPR_RAIL_TRACK_N_S,
			SPR_RAIL_TRACK_BASE,
			SPR_RAIL_SINGLE_X,
			SPR_RAIL_SINGLE_Y,
			SPR_RAIL_SINGLE_NORTH,
			SPR_RAIL_SINGLE_SOUTH,
			SPR_RAIL_SINGLE_EAST,
			SPR_RAIL_SINGLE_WEST,
		},

		/* GUI sprites */
		{	SPR_IMG_BUS_STATION, //build infrastructure
			SPR_IMG_DEPOT_RAIL, // build track tile
			SPR_IMG_DEPOT_RAIL,   //autoair
			SPR_IMG_DEPOT_RAIL,    //define runway
			SPR_IMG_DEPOT_RAIL,    //terminal
			SPR_IMG_DEPOT_RAIL,    //helipad
			SPR_IMG_DEPOT_RAIL,    //helipoort
			SPR_IMG_DEPOT_RAIL,    //hangar
			SPR_IMG_AIRPORT, //build predefined
			SPR_IMG_AIRPORT, //copy
			SPR_IMG_CONVERT_RAIL
		},

		/* cursor sprites */
		{	SPR_CURSOR_NS_ELRAIL, //build infrastructure
		        SPR_CURSOR_AUTOELRAIL,   //autoair
			SPR_IMG_DEPOT_RAIL,    //define runway
			SPR_IMG_DEPOT_RAIL,    //terminal
			SPR_IMG_DEPOT_RAIL,    //helipad
			SPR_IMG_DEPOT_RAIL,    //helipoort
			SPR_CURSOR_ELRAIL_DEPOT,    //hangar
			SPR_CURSOR_TUNNEL_RAIL, //copy
			SPR_CURSOR_CONVERT_RAIL
		},

		/* strings */
		{
			STR_AIRTYPE_NAME_GRAVEL,
			STR_TOOLBAR_AIRPORT_GRAVEL_CONSTRUCTION_CAPTION,
			STR_TOOLBAR_AIRPORT_GRAVEL_MENU_TEXT,
			STR_REPLACE_AIRCRAFT_GRAVEL_VEHICLES,
		},

		/* Offset of snow tiles */
		SPR_RAIL_SNOW_OFFSET,

		/* Compatible railtypes */
		AIRTYPES_GRAVEL,

		/* fallback_railtype */
		0,

		/* curve speed advantage (multiplier) */
		0,

		/* cost multiplier */
		8,

		/* maintenance cost multiplier */
		8,

		/* acceleration type */
		0,

		/* max speed */
		0,

		/* Gravel type label. */
		'GRVL',

		/* alternate labels */
		AirTypeLabelList(),

		/* map colour */
		0x0A,

		/* introduction date */
		INVALID_DATE,

		/* railtypes required for this to be introduced */
		AIRTYPES_NONE,

		/* introduction rail types */
		AIRTYPES_GRAVEL,

		/* sort order */
		0 << 4 | 7,

		{ NULL },
		{ NULL },

		/* Catchment. */
		3,

		/* Max number of runways. */
		2,

		/* Min runway length in tiles. */
		4,

		/* Base noise level. */
		0,

		/* Runway noise level. */
		1,
	},
	/** Asphalt */
	{ // Main Sprites
		{	SPR_RAIL_TRACK_Y,
			SPR_RAIL_TRACK_N_S,
			SPR_RAIL_TRACK_BASE,
			SPR_RAIL_SINGLE_X,
			SPR_RAIL_SINGLE_Y,
			SPR_RAIL_SINGLE_NORTH,
			SPR_RAIL_SINGLE_SOUTH,
			SPR_RAIL_SINGLE_EAST,
			SPR_RAIL_SINGLE_WEST,
		},

		/* GUI sprites */
		{	SPR_IMG_BUS_STATION, //build infrastructure
			SPR_IMG_DEPOT_MONO, // build track tile
			SPR_IMG_DEPOT_MONO,   //autoair
			SPR_IMG_DEPOT_MONO,    //define runway
			SPR_IMG_DEPOT_MONO,    //terminal
			SPR_IMG_DEPOT_MONO,    //helipad
			SPR_IMG_DEPOT_MONO,    //helipoort
			SPR_IMG_DEPOT_MONO,    //hangar
			SPR_IMG_AIRPORT, //build predefined
			SPR_IMG_AIRPORT, //copy
			SPR_IMG_CONVERT_MONO
		},

		/* cursor sprites */
		{	SPR_CURSOR_NS_ELRAIL, //build infrastructure
			SPR_CURSOR_AUTOELRAIL,   //autoair
			SPR_IMG_DEPOT_RAIL,    //define runway
			SPR_IMG_DEPOT_RAIL,    //terminal
			SPR_IMG_DEPOT_RAIL,    //helipad
			SPR_IMG_DEPOT_RAIL,    //helipoort
			SPR_CURSOR_ELRAIL_DEPOT,    //hangar
			SPR_CURSOR_TUNNEL_RAIL, //copy
			SPR_CURSOR_CONVERT_RAIL
		},

		/* strings */
		{
			STR_AIRTYPE_NAME_ASPHALT,
			STR_TOOLBAR_AIRPORT_ASPHALT_CONSTRUCTION_CAPTION,
			STR_TOOLBAR_AIRPORT_ASPHALT_CONSTRUCTION_CAPTION,
			STR_REPLACE_AIRCRAFT_ASPHALT_VEHICLES,
		},

		/* Offset of snow tiles */
		SPR_RAIL_SNOW_OFFSET,

		/* Compatible railtypes */
		AIRTYPES_ASPHALT,

		/* fallback_railtype */
		0,

		/* curve speed advantage (multiplier) */
		0,

		/* cost multiplier */
		8,

		/* maintenance cost multiplier */
		8,

		/* acceleration type */
		0,

		/* max speed */
		0,

		/* Gravel type label. */
		'ASPH',

		/* alternate labels */
		AirTypeLabelList(),

		/* map colour */
		0x0A,

		/* introduction date */
		INVALID_DATE,

		/* railtypes required for this to be introduced */
		AIRTYPES_NONE,

		/* introduction rail types */
		AIRTYPES_ASPHALT,

		/* sort order */
		1 << 4 | 7,

		{ NULL },
		{ NULL },

		/* Catchment. */
		8,

		/* Max number of runways. */
		4,

		/* Min runway length in tiles. */
		5,

		/* Base noise level. */
		1,

		/* Runway noise level. */
		3,
	},

	/** Water */
	{ // Main Sprites
		{	SPR_RAIL_TRACK_Y,
			SPR_RAIL_TRACK_N_S,
			SPR_RAIL_TRACK_BASE,
			SPR_RAIL_SINGLE_X,
			SPR_RAIL_SINGLE_Y,
			SPR_RAIL_SINGLE_NORTH,
			SPR_RAIL_SINGLE_SOUTH,
			SPR_RAIL_SINGLE_EAST,
			SPR_RAIL_SINGLE_WEST,
		},

		/* GUI sprites */
		{	SPR_IMG_BUS_STATION, //build infrastructure
			SPR_CURSOR_NS_TRACK, // build track tile
			SPR_CURSOR_AUTORAIL,   //autoair
			SPR_IMG_DEPOT_MAGLEV,    //define runway
			SPR_IMG_DEPOT_MAGLEV,    //terminal
			SPR_IMG_DEPOT_MAGLEV,    //helipad
			SPR_IMG_DEPOT_MAGLEV,    //helipoort
			SPR_IMG_DEPOT_MAGLEV,    //hangar
			SPR_IMG_AIRPORT, //build predefined
			SPR_IMG_AIRPORT, //copy
			SPR_IMG_CONVERT_MAGLEV
		},

		/* cursor sprites */
		{	SPR_CURSOR_NS_ELRAIL, //build infrastructure
			SPR_CURSOR_AUTOELRAIL,   //autoair
			SPR_IMG_DEPOT_RAIL,    //define runway
			SPR_IMG_DEPOT_RAIL,    //terminal
			SPR_IMG_DEPOT_RAIL,    //helipad
			SPR_IMG_DEPOT_RAIL,    //helipoort
			SPR_CURSOR_ELRAIL_DEPOT,    //hangar
			SPR_CURSOR_TUNNEL_RAIL, //copy
			SPR_CURSOR_CONVERT_RAIL
		},

		/* strings */
		{
			STR_AIRTYPE_NAME_WATER,
			STR_TOOLBAR_AIRPORT_WATER_CONSTRUCTION_CAPTION,
			STR_TOOLBAR_AIRPORT_WATER_CONSTRUCTION_CAPTION,
			STR_REPLACE_AIRCRAFT_WATER_VEHICLES,
		},

		/* Offset of snow tiles */
		SPR_RAIL_SNOW_OFFSET,

		/* Compatible railtypes */
		AIRTYPES_WATER,

		/* fallback_railtype */
		0,

		/* curve speed advantage (multiplier) */
		0,

		/* cost multiplier */
		8,

		/* maintenance cost multiplier */
		8,

		/* acceleration type */
		0,

		/* max speed */
		0,

		/* Gravel type label. */
		'WATR',

		/* alternate labels */
		AirTypeLabelList(),

		/* map colour */
		0x0A,

		/* introduction date */
		INVALID_DATE,

		/* railtypes required for this to be introduced */
		AIRTYPES_NONE,

		/* introduction rail types */
		AIRTYPES_WATER,

		/* sort order */
		2 << 4 | 7,

		{ NULL },
		{ NULL },

		/* Catchment. */
		4,

		/* Max number of runways. */
		2,

		/* Min runway length in tiles. */
		5,

		/* Base noise level. */
		1,

		/* Runway noise level. */
		0,
	}
};

#endif /* AIRTYPES_H */
