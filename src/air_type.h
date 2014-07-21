/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file air_type.h The different types of air tracks. */

#ifndef AIR_TYPE_H
#define AIR_TYPE_H

#include "core/enum_type.hpp"

typedef uint32 AirTypeLabel;

static const AirTypeLabel AIRTYPE_GRAVEL_LABEL     = 'GRVL';
static const AirTypeLabel AIRTYPE_ASPHALT_LABEL    = 'ASPH';
static const AirTypeLabel AIRTYPE_WATER_LABEL      = 'WATR';

/** Enumeration for all possible airtypes. */
enum AirType {
	AIRTYPE_BEGIN    = 0,    ///< Used for iterations
	AIRTYPE_GRAVEL   = 0,    ///< Gravel surface
	AIRTYPE_ASPHALT,         ///< Asphalt surface
	AIRTYPE_WATER,           ///< Water surface
	AIRTYPE_END,             ///< Used for iterations
	INVALID_AIRTYPE  = 0xFF, ///< Flag for invalid airtype

	DEF_AIRTYPE_FIRST = AIRTYPE_END, ///< Default airtype: first available
	DEF_AIRTYPE_LAST,                ///< Default airtype: last available
	DEF_AIRTYPE_MOST_USED,           ///< Default airtype: most used
};

/** Allow incrementing of airtype variables */
DECLARE_POSTFIX_INCREMENT(AirType)

/** Define basic enum properties */
template <> struct EnumPropsT<AirType> : MakeEnumPropsT<AirType, byte, AIRTYPE_BEGIN, AIRTYPE_END, INVALID_AIRTYPE, 4> {};
typedef TinyEnumT<AirType> AirTypeByte;

/** The different airtypes we support, but then a bitmask of them.  */
enum AirTypes {
	AIRTYPES_NONE     = 0,                      ///< No rail types
	AIRTYPES_GRAVEL   = 1 << AIRTYPE_GRAVEL,    ///< Gravel surface
	AIRTYPES_ASPHALT  = 1 << AIRTYPE_ASPHALT,   ///< Asphalt surface
	AIRTYPES_WATER    = 1 << AIRTYPE_WATER,     ///< Water surface
	AIRTYPES_ALL      = 1 << AIRTYPE_GRAVEL |
				1 << AIRTYPE_ASPHALT |
				1 << AIRTYPE_WATER,
	INVALID_AIRTYPES  = UINT_MAX,               ///< Invalid airtypes
};
DECLARE_ENUM_AS_BIT_SET(AirTypes)

/** Types of tiles an airport can have. */
enum AirportTileType {
	ATT_BEGIN = 0,
	ATT_INFRASTRUCTURE = ATT_BEGIN, // 000
	ATT_SIMPLE_TRACK   = 1,         // 001
	ATT_TERMINAL       = 2,         // 010
	ATT_HANGAR         = 3,         // 011
	ATT_RUNWAY         = 4,         // 100
	ATT_RUNWAY_END     = 5,         // 101
	ATT_RUNWAY_START   = 6,         // 110
	ATT_WAITING_POINT  = 7,         // 111 (unused)
	ATT_END,

	ATT_INVALID,

	/* Mask for tiles that can be start or end of a ground path. */
	ATT_SAFEPOINT_MASK = 2,         // 010
};

enum TerminalType {
	HTT_BEGIN = 0,
	HTT_TERMINAL = HTT_BEGIN,
	HTT_HELIPAD,
	HTT_HELIPORT,
	HTT_BUILTIN_HELIPORT,
	HTT_TERMINAL_END,
	HTT_INVALID,
};

#endif /* AIR_TYPE_H */
