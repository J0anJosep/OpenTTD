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
		{	0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
		},

		/* GUI sprites */
		{	SPR_AIRPORT_TYPE_GRAVEL,     // add tile to airport
			SPR_AIRPORT_TYPE_GRAVEL + 1, // build track tile
			SPR_AIRPORT_TYPE_GRAVEL + 2, // change air type
			SPR_AIRPORT_TYPE_GRAVEL + 3, //define runway
			SPR_AIRPORT_TYPE_GRAVEL + 4, //terminal
			SPR_AIRPORT_TYPE_GRAVEL + 5, //helipad
			SPR_AIRPORT_TYPE_GRAVEL + 6, //heliport
			SPR_AIRPORT_TYPE_GRAVEL + 7, //hangar
			SPR_AIRPORT_TYPE_GRAVEL + 8, //build predefined
			SPR_AIRPORT_TYPE_GRAVEL + 9, //copy
			SPR_AIRPORT_TYPE_GRAVEL + 10
		},

		/* cursor sprites */
		{	SPR_AIRPORT_TYPE_GRAVEL + 11,
			SPR_AIRPORT_TYPE_GRAVEL + 12,
			SPR_AIRPORT_TYPE_GRAVEL + 13,
			SPR_AIRPORT_TYPE_GRAVEL + 14,
			SPR_AIRPORT_TYPE_GRAVEL + 15,
			SPR_AIRPORT_TYPE_GRAVEL + 16,
			SPR_AIRPORT_TYPE_GRAVEL + 17,
			SPR_AIRPORT_TYPE_GRAVEL + 18,
			SPR_AIRPORT_TYPE_GRAVEL + 19,
			SPR_AIRPORT_TYPE_GRAVEL + 20,
			SPR_AIRPORT_TYPE_GRAVEL + 21
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

		/* Heliport availability. */
		false,
	},
	/** Asphalt */
	{ // Main Sprites
		{	0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
		},

		/* GUI sprites */
		{	SPR_AIRPORT_TYPE_ASPHALT,     // add tile to airport
			SPR_AIRPORT_TYPE_ASPHALT + 1, // build track tile
			SPR_AIRPORT_TYPE_ASPHALT + 2, // change air type
			SPR_AIRPORT_TYPE_ASPHALT + 3, //define runway
			SPR_AIRPORT_TYPE_ASPHALT + 4, //terminal
			SPR_AIRPORT_TYPE_ASPHALT + 5, //helipad
			SPR_AIRPORT_TYPE_ASPHALT + 6, //heliport
			SPR_AIRPORT_TYPE_ASPHALT + 7, //hangar
			SPR_AIRPORT_TYPE_ASPHALT + 8, //build predefined
			SPR_AIRPORT_TYPE_ASPHALT + 9, //copy
			SPR_AIRPORT_TYPE_ASPHALT + 10
		},

		/* cursor sprites */
		{	SPR_AIRPORT_TYPE_ASPHALT + 11,
			SPR_AIRPORT_TYPE_ASPHALT + 12,
			SPR_AIRPORT_TYPE_ASPHALT + 13,
			SPR_AIRPORT_TYPE_ASPHALT + 14,
			SPR_AIRPORT_TYPE_ASPHALT + 15,
			SPR_AIRPORT_TYPE_ASPHALT + 16,
			SPR_AIRPORT_TYPE_ASPHALT + 17,
			SPR_AIRPORT_TYPE_ASPHALT + 18,
			SPR_AIRPORT_TYPE_ASPHALT + 19,
			SPR_AIRPORT_TYPE_ASPHALT + 20,
			SPR_AIRPORT_TYPE_ASPHALT + 21
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

		/* Heliport availability. */
		true,
	},

	/** Water */
	{ // Main Sprites
		{	0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
		},

		/* GUI sprites */
		{	SPR_AIRPORT_TYPE_WATER,
			SPR_AIRPORT_TYPE_WATER + 1,
			SPR_AIRPORT_TYPE_WATER + 2,
			SPR_AIRPORT_TYPE_WATER + 3,
			SPR_AIRPORT_TYPE_WATER + 4,
			SPR_AIRPORT_TYPE_WATER + 5,
			SPR_AIRPORT_TYPE_WATER + 6,
			SPR_AIRPORT_TYPE_WATER + 7,
			SPR_AIRPORT_TYPE_WATER + 8,
			SPR_AIRPORT_TYPE_WATER + 9,
			SPR_AIRPORT_TYPE_WATER + 10
		},

		/* cursor sprites */
		{	SPR_AIRPORT_TYPE_WATER + 11,
			SPR_AIRPORT_TYPE_WATER + 12,
			SPR_AIRPORT_TYPE_WATER + 13,
			SPR_AIRPORT_TYPE_WATER + 14,
			SPR_AIRPORT_TYPE_WATER + 15,
			SPR_AIRPORT_TYPE_WATER + 16,
			SPR_AIRPORT_TYPE_WATER + 17,
			SPR_AIRPORT_TYPE_WATER + 18,
			SPR_AIRPORT_TYPE_WATER + 19,
			SPR_AIRPORT_TYPE_WATER + 20,
			SPR_AIRPORT_TYPE_WATER + 21
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

		/* Heliport availability. */
		false,
	}
};

#endif /* AIRTYPES_H */
