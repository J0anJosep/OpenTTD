/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file air.h Air specific functions. */

#ifndef AIR_H
#define AIR_H

#include "track_type.h"
#include "gfx_type.h"
#include "core/bitmath_func.hpp"
#include "economy_func.h"
#include "slope_type.h"
#include "strings_type.h"
#include "date_type.h"
#include "air_type.h"

struct SpriteGroup;

/** Sprite groups for a airtype. */
enum AirTypeSpriteGroup {
	ATSG_CURSORS,     ///< Cursor and toolbar icon images
	ATSG_OVERLAY,     ///< Images for overlaying track
	ATSG_GROUND,      ///< Main group of ground images
	ATSG_TUNNEL,      ///< Main group of ground images for snow or desert
	ATSG_DEPOT,       ///< Depot images
	ATSG_FENCES,      ///< Fence images
	ATSG_END,
};

/**
 * Offsets for sprites within an overlay/underlay set.
 * These are the same for overlay and underlay sprites.
 */
enum AirTrackOffset {
	ATO_X,            ///< Piece of air track in X direction
	ATO_Y,            ///< Piece of air track in Y direction
	ATO_N,            ///< Piece of air track in northern corner
	ATO_S,            ///< Piece of air track in southern corner
	ATO_E,            ///< Piece of air track in eastern corner
	ATO_W,            ///< Piece of air track in western corner
};

/**
 * Offsets from base sprite for fence sprites. These are in the order of
 *  the sprites in the original data files.
 */
enum AirFenceOffset {
	AFO_FLAT_X,
	AFO_FLAT_Y,
	AFO_FLAT_VERT,
	AFO_FLAT_HORZ,
	AFO_SLOPE_SW,
	AFO_SLOPE_SE,
	AFO_SLOPE_NE,
	AFO_SLOPE_NW,
};

/** List of airport type labels. */
typedef SmallVector<AirTypeLabel, 4> AirTypeLabelList;

/**
 * This struct contains all the info that is needed to draw and construct tracks.
 */
class AirTypeInfo {
public:
	/**
	 * Struct containing the main sprites. @note not all sprites are listed, but only
	 *  the ones used directly in the code
	 */
	struct {
		SpriteID track_y;      ///< single piece of track in Y direction, with ground
		SpriteID track_ns;     ///< two pieces of track in North and South corner (East-West direction)
		SpriteID ground;       ///< ground sprite for a 3-way switch
		SpriteID single_x;     ///< single piece of track in X direction, without ground
		SpriteID single_y;     ///< single piece of track in Y direction, without ground
		SpriteID single_n;     ///< single piece of track in the northern corner
		SpriteID single_s;     ///< single piece of track in the southern corner
		SpriteID single_e;     ///< single piece of track in the eastern corner
		SpriteID single_w;     ///< single piece of track in the western corner
	} base_sprites;

	/**
	 * struct containing the sprites for the airport GUI. @note only sprites referred to
	 * directly in the code are listed
	 */
	struct {
		SpriteID build_infrastructure;
		SpriteID build_track_tile;
		SpriteID auto_air;                    ///< button for the autotrack construction
		SpriteID define_runway;               ///< button for defining runways
		SpriteID build_terminal;
		SpriteID build_helipad;
		SpriteID build_heliport;
		SpriteID build_hangar;
		SpriteID build_predefined_airport;
		SpriteID copy_existing_airport;
		SpriteID convert_air;                 ///< button for converting airport
	} gui_sprites;

	/**
	 * struct containing the sprites for the airport GUI. @note only sprites referred to
	 * directly in the code are listed
	 */
	struct {
		CursorID build_infrastructure;
		CursorID auto_air;                    ///< button for the autotrack construction
		CursorID define_runway;               ///< button for defining runways
		CursorID build_terminal;
		CursorID build_helipad;
		CursorID build_heliport;
		CursorID build_hangar;
		CursorID copy_airport;
		CursorID convert_air;                 ///< button for converting airport
	} cursor;

	struct {
		StringID name;
		StringID toolbar_caption;
		StringID menu_text;
		StringID replace_text;
	} strings;

	/** sprite number difference between a piece of track on a snowy ground and the corresponding one on normal ground */
	SpriteID snow_offset;

	/** bitmask to the OTHER airtypes on which an engine of THIS airtype can physically travel */
	AirTypes compatible_airtypes;

	/**
	 * Original airtype number to use when drawing non-newgrf airtypes, or when drawing stations.
	 */
	byte fallback_airtype;

	/**
	 * Multiplier for curve maximum speed advantage
	 */
	byte curve_speed;

	/**
	 * Cost multiplier for building this air type
	 */
	uint16 cost_multiplier;

	/**
	 * Cost multiplier for maintenance of this air type
	 */
	uint16 maintenance_multiplier;

	/**
	 * Acceleration type of this air type
	 */
	uint8 acceleration_type;

	/**
	 * Maximum speed for vehicles travelling on this air type
	 */
	uint16 max_speed;

	/**
	 * Unique 32 bit air type identifier
	 */
	AirTypeLabel label;

	/**
	 * Air type labels this type provides in addition to the main label.
	 */
	AirTypeLabelList alternate_labels;

	/**
	 * Colour on mini-map
	 */
	byte map_colour;

	/**
	 * Introduction date.
	 * When #INVALID_DATE or a vehicle using this airtype gets introduced earlier,
	 * the vehicle's introduction date will be used instead for this airtype.
	 * The introduction at this date is furthermore limited by the
	 * #introduction_required_types.
	 */
	Date introduction_date;

	/**
	 * Bitmask of airtypes that are required for this airtype to be introduced
	 * at a given #introduction_date.
	 */
	AirTypes introduction_required_airtypes;

	/**
	 * Bitmask of which other airtypes are introduced when this airtype is introduced.
	 */
	AirTypes introduces_airtypes;

	/**
	 * The sorting order of this airtype for the toolbar dropdown.
	 */
	byte sorting_order;

	/**
	 * NewGRF providing the Action3 for the airtype. NULL if not available.
	 */
	const GRFFile *grffile[ATSG_END];

	/**
	 * Sprite groups for resolving sprites
	 */
	const SpriteGroup *group[ATSG_END];

	/**
	 * Catchment area radius.
	 */
	byte catchment_radius;

	/**
	 * Max number of runways.
	 */
	byte max_num_runways;

	/**
	 * Minimum runway length in tiles.
	 */
	byte min_runway_length;

	/**
	 * Base noise level. Each station has this noise level plus the noise created by each runway.
	 * Example: if base noise is 5 and there are 4 runways and runway level is 6,
	 *                  total noise level of the airport is 5 + 4 * 6 = 29
	 */
	byte base_noise_level;

	/**
	 * Runway noise level.
	 */
	byte runway_noise_level;


	inline bool UsesOverlay() const
	{
		return this->group[ATSG_GROUND] != NULL;
	}

	/**
	 * Offset between the current airtype and normal air. This means that:<p>
	 * 1) All the sprites in an airset MUST be in the same order. This order
	 *    is determined by normal air. Check sprites 1005 and following for this order<p>
	 * 2) The position where the airtype is loaded must always be the same, otherwise
	 *    the offset will fail.
	 */
	inline uint GetAirTypeSpriteOffset() const
	{
		return 82 * this->fallback_airtype;
	}
};


/**
 * Returns a pointer to the Airtype information for a given airtype
 * @param airtype the air type which the information is requested for
 * @return The pointer to the AirTypeInfo
 */
static inline const AirTypeInfo *GetAirTypeInfo(const AirType airtype)
{
	extern AirTypeInfo _airtypes[AIRTYPE_END];
	assert(airtype < AIRTYPE_END);
	return &_airtypes[airtype];
}

/**
 * Checks if an engine of the given AirType can drive
 * on a tile with a given AirType.
 * @return Whether the engine can drive on this tile.
 * @param  enginetype The AirType of the engine we are considering.
 * @param  tiletype   The AirType of the tile we are considering.
 */
static inline bool IsCompatibleAirType(const AirType enginetype, const AirType tiletype)
{
	return HasBit(GetAirTypeInfo(enginetype)->compatible_airtypes, tiletype);
}

/**
 * Returns the cost of building the specified airtype.
 * @param airtype The airtype being built.
 * @return The cost multiplier.
 */
static inline Money AirBuildCost(AirType airtype)
{
	assert(airtype < AIRTYPE_END);
	return (_price[PR_BUILD_STATION_AIRPORT] * GetAirTypeInfo(airtype)->cost_multiplier) >> 3;
}

/**
 * Returns the 'cost' of clearing the specified airtype.
 * @param airtype The airtype being removed.
 * @return The cost.
 */
static inline Money AirClearCost(AirType airtype)
{
	/* Clearing airport tiles in fact earns money, but if the build cost is set
	 * very low then a loophole exists where money can be made.
	 * In this case we limit the removal earnings to 3/4s of the build
	 * cost.
	 */
	assert(airtype < AIRTYPE_END);
	return max(_price[PR_CLEAR_STATION_AIRPORT], -AirBuildCost(airtype) * 3 / 4);
}

/**
 * Calculates the cost of air conversion
 * @param from The airtype we are converting from
 * @param to   The airtype we are converting to
 * @return Cost per TrackBit
 */
static inline Money AirConvertCost(AirType from, AirType to)
{
	return AirBuildCost(to) + AirClearCost(from);
}

/**
 * Calculates the maintenance cost of a number of track bits.
 * @param airtype The airtype to get the cost of.
 * @param num Number of track bits of this airtype.
 * @param total_num Total number of track bits of all airtypes.
 * @return Total cost.
 */
static inline Money AirMaintenanceCost(AirType airtype, uint32 num, uint32 total_num)
{
	assert(airtype < AIRTYPE_END);
	return (_price[PR_INFRASTRUCTURE_AIRPORT] * GetAirTypeInfo(airtype)->maintenance_multiplier * num * (1 + IntSqrt(total_num))) >> 11; // 4 bits fraction for the multiplier and 7 bits scaling.
}

Foundation GetAirFoundation(Slope tileh, TrackBits bits);


bool HasAirTypeAvail(const CompanyID company, const AirType airtype);
bool ValParamAirType(const AirType airtype);

AirTypes AddDateIntroducedAirTypes(AirTypes current, Date date);

AirType GetBestAirType(const CompanyID company);
AirTypes GetCompanyAirTypes(const CompanyID c);

AirType GetAirTypeByLabel(AirTypeLabel label, bool allow_alternate_labels = true);

void ResetAirTypes();
void InitAirTypes();
AirType AllocateAirType(AirTypeLabel label);

#endif /* AIR_H */
