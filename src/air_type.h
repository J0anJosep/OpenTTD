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
enum AirType : byte {
	AIRTYPE_BEGIN    = 0,    ///< Used for iterations
	AIRTYPE_GRAVEL   = 0,    ///< Gravel surface
	AIRTYPE_ASPHALT  = 1,    ///< Asphalt surface
	AIRTYPE_WATER    = 2,    ///< Water surface
	AIRTYPE_END      = 16,   ///< Used for iterations
	INVALID_AIRTYPE  = 0xFF, ///< Flag for invalid airtype

	DEF_AIRTYPE_FIRST = AIRTYPE_END, ///< Default airtype: first available
	DEF_AIRTYPE_LAST,                ///< Default airtype: last available
	DEF_AIRTYPE_MOST_USED,           ///< Default airtype: most used
};

/** Allow incrementing of airtype variables */
DECLARE_POSTFIX_INCREMENT(AirType)

/** Define basic enum properties */
template <> struct EnumPropsT<AirType> : MakeEnumPropsT<AirType, byte, AIRTYPE_BEGIN, AIRTYPE_END, INVALID_AIRTYPE, 4> {};

/** The different airtypes we support, but then a bitmask of them.  */
enum AirTypes : uint16 {
	AIRTYPES_NONE     = 0,                      ///< No rail types
	AIRTYPES_GRAVEL   = 1 << AIRTYPE_GRAVEL,    ///< Gravel surface
	AIRTYPES_ASPHALT  = 1 << AIRTYPE_ASPHALT,   ///< Asphalt surface
	AIRTYPES_WATER    = 1 << AIRTYPE_WATER,     ///< Water surface
	AIRTYPES_ALL      = 1 << AIRTYPE_GRAVEL |
				1 << AIRTYPE_ASPHALT |
				1 << AIRTYPE_WATER,
	INVALID_AIRTYPES  = UINT16_MAX,             ///< Invalid airtypes
};
DECLARE_ENUM_AS_BIT_SET(AirTypes)

/** Types of tiles an airport can have. */
enum AirportTileType : byte {
	ATT_BEGIN = 0,
	ATT_INFRASTRUCTURE_NO_CATCH    = ATT_BEGIN, // 0000
	ATT_INFRASTRUCTURE_WITH_CATCH  =  1,        // 0001
	ATT_SIMPLE_TRACK               =  2,        // 0010
	ATT_WAITING_POINT              =  3,        // 0011
	ATT_APRON_NORMAL               =  4,        // 0100
	ATT_APRON_HELIPAD              =  5,        // 0101
	ATT_APRON_HELIPORT             =  6,        // 0110
	ATT_APRON_BUILTIN_HELIPORT     =  7,        // 0111
	ATT_HANGAR_STANDARD            =  8,        // 1000
	ATT_HANGAR_EXTENDED            = 10,        // 1010
	ATT_RUNWAY_MIDDLE              = 12,        // 1100
	ATT_RUNWAY_END                 = 13,        // 1101
	ATT_RUNWAY_START_NO_LANDING    = 14,        // 1110
	ATT_RUNWAY_START_ALLOW_LANDING = 15,        // 1111
	ATT_END,

	ATT_INVALID,

	ATT_NUM_BITS                     = 4,
	ATT_INFRA_LAYOUT_NUM_BITS        = 3,
	ATT_INFRA_LAYOUT_BITS            = 0,
	ATT_APRON_LAYOUT_NUM_BITS        = 2,
	ATT_APRON_LAYOUT_BITS            = 1,
	ATT_HANGAR_LAYOUT_NUM_BITS       = 2,
	ATT_HANGAR_LAYOUT_BITS           = 2,
	ATT_RUNWAY_LAYOUT_NUM_BITS       = 2,
	ATT_RUNWAY_LAYOUT_BITS           = 3,
	ATT_RUNWAY_START_LAYOUT_NUM_BITS = 3,
	ATT_RUNWAY_START_LAYOUT_BITS     = 7,
};

enum ApronType : byte {
	APRON_BEGIN = 0,
	APRON_APRON = APRON_BEGIN,
	APRON_HELIPAD,
	APRON_HELIPORT,
	APRON_BUILTIN_HELIPORT,
	APRON_END,
	APRON_INVALID,
};

#endif /* AIR_TYPE_H */
