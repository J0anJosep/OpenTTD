/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_defaults.h Tables with default values for airports and airport tiles. */

#ifndef AIRPORT_DEFAULTS_H
#define AIRPORT_DEFAULTS_H

#include "../track_type.h"
#include "../air_type.h"
#include "airporttile_ids.h"
#include "airport_movement.h"

/**
 * Define a generic airport.
 * @param name Suffix of the names of the airport data.
 * @param terminals The terminals.
 * @param num_helipads Number of heli pads.
 * @param flags Information about the class of FTA.
 * @param delta_z Height of the airport above the land.
 */
#define AIRPORT_GENERIC(name, terminals, num_helipads, flags, delta_z) \
static const AirportFTAClass _airportfta_ ## name(_airport_moving_data_ ## name, terminals, \
num_helipads, _airport_entries_ ## name, flags, _airport_fta_ ## name, delta_z);

/**
 * Define an airport.
 * @param name Suffix of the names of the airport data.
 * @param num_helipads Number of heli pads.
 * @param short_strip Airport has a short land/take-off strip.
 */
#define AIRPORT(name, num_helipads, short_strip) \
AIRPORT_GENERIC(name, _airport_terminal_ ## name, num_helipads, AirportFTAClass::ALL | (short_strip ? AirportFTAClass::SHORT_STRIP : (AirportFTAClass::Flags)0), 0)

/**
 * Define a heliport.
 * @param name Suffix of the names of the helipad data.
 * @param num_helipads Number of heli pads.
 * @param delta_z Height of the airport above the land.
 */
#define HELIPORT(name, num_helipads, delta_z) \
AIRPORT_GENERIC(name, nullptr, num_helipads, AirportFTAClass::HELICOPTERS, delta_z)

AIRPORT(country, 0, true)
AIRPORT(city, 0, false)
HELIPORT(heliport, 1, 60)
AIRPORT(metropolitan, 0, false)
AIRPORT(international, 2, false)
AIRPORT(commuter, 2, true)
HELIPORT(helidepot, 1, 0)
AIRPORT(intercontinental, 2, false)
HELIPORT(helistation, 3, 0)
HELIPORT(oilrig, 1, 54)
AIRPORT_GENERIC(dummy, nullptr, 0, AirportFTAClass::ALL, 0)

#undef HELIPORT
#undef AIRPORT
#undef AIRPORT_GENERIC

/**
 * Definition of an airport tiles layout.
 * @param x offset x of this tile
 * @param y offset y of this tile
 * @param m StationGfx of the tile
 * @see _airport_specs
 * @see AirportTileTable
 */
#define MK(x, y, m) {{x, y}, m}

/**
 * Terminator of airport tiles layout definition
 */
#define MKEND {{-0x80, 0}, 0}

/** Tiles for Country Airfield (small) */
static const std::initializer_list<AirportTileTable> _tile_table_country_0 = {
	MK(0, 0, APT_SMALL_BUILDING_1),
	MK(1, 0, APT_SMALL_BUILDING_2),
	MK(2, 0, APT_SMALL_BUILDING_3),
	MK(3, 0, APT_SMALL_DEPOT_SE),
	MK(0, 1, APT_GRASS_FENCE_NE_FLAG),
	MK(1, 1, APT_GRASS_1),
	MK(2, 1, APT_GRASS_2),
	MK(3, 1, APT_GRASS_FENCE_SW),
	MK(0, 2, APT_RUNWAY_SMALL_FAR_END),
	MK(1, 2, APT_RUNWAY_SMALL_MIDDLE),
	MK(2, 2, APT_RUNWAY_SMALL_MIDDLE),
	MK(3, 2, APT_RUNWAY_SMALL_NEAR_END),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_country = {
	{ _tile_table_country_0, DIR_N },
};

/** Tiles for Commuter Airfield (small) */
static const std::initializer_list<AirportTileTable> _tile_table_commuter_0 = {
	MK(0, 0, APT_TOWER),
	MK(1, 0, APT_BUILDING_3),
	MK(2, 0, APT_HELIPAD_2_FENCE_NW),
	MK(3, 0, APT_HELIPAD_2_FENCE_NW),
	MK(4, 0, APT_DEPOT_SE),
	MK(0, 1, APT_APRON_FENCE_NE),
	MK(1, 1, APT_APRON),
	MK(2, 1, APT_APRON),
	MK(3, 1, APT_APRON),
	MK(4, 1, APT_APRON_FENCE_SW),
	MK(0, 2, APT_APRON_FENCE_NE),
	MK(1, 2, APT_STAND),
	MK(2, 2, APT_STAND),
	MK(3, 2, APT_STAND),
	MK(4, 2, APT_APRON_FENCE_SW),
	MK(0, 3, APT_RUNWAY_END_FENCE_SE),
	MK(1, 3, APT_RUNWAY_2),
	MK(2, 3, APT_RUNWAY_2),
	MK(3, 3, APT_RUNWAY_2),
	MK(4, 3, APT_RUNWAY_END_FENCE_SE),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_commuter = {
	{ _tile_table_commuter_0, DIR_N },
};

/** Tiles for City Airport (large) */
static const std::initializer_list<AirportTileTable> _tile_table_city_0 = {
	MK(0, 0, APT_BUILDING_1),
	MK(1, 0, APT_APRON_FENCE_NW),
	MK(2, 0, APT_STAND_1),
	MK(3, 0, APT_APRON_FENCE_NW),
	MK(4, 0, APT_APRON_FENCE_NW),
	MK(5, 0, APT_DEPOT_SE),
	MK(0, 1, APT_BUILDING_2),
	MK(1, 1, APT_PIER),
	MK(2, 1, APT_ROUND_TERMINAL),
	MK(3, 1, APT_STAND_PIER_NE),
	MK(4, 1, APT_APRON),
	MK(5, 1, APT_APRON_FENCE_SW),
	MK(0, 2, APT_BUILDING_3),
	MK(1, 2, APT_STAND),
	MK(2, 2, APT_PIER_NW_NE),
	MK(3, 2, APT_APRON_S),
	MK(4, 2, APT_APRON_HOR),
	MK(5, 2, APT_APRON_N_FENCE_SW),
	MK(0, 3, APT_RADIO_TOWER_FENCE_NE),
	MK(1, 3, APT_APRON_W),
	MK(2, 3, APT_APRON_VER_CROSSING_S),
	MK(3, 3, APT_APRON_HOR_CROSSING_E),
	MK(4, 3, APT_ARPON_N),
	MK(5, 3, APT_TOWER_FENCE_SW),
	MK(0, 4, APT_EMPTY_FENCE_NE),
	MK(1, 4, APT_APRON_S),
	MK(2, 4, APT_APRON_HOR_CROSSING_W),
	MK(3, 4, APT_APRON_VER_CROSSING_N),
	MK(4, 4, APT_APRON_E),
	MK(5, 4, APT_RADAR_GRASS_FENCE_SW),
	MK(0, 5, APT_RUNWAY_END_FENCE_SE),
	MK(1, 5, APT_RUNWAY_1),
	MK(2, 5, APT_RUNWAY_2),
	MK(3, 5, APT_RUNWAY_3),
	MK(4, 5, APT_RUNWAY_4),
	MK(5, 5, APT_RUNWAY_END_FENCE_SE),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_city = {
	{ _tile_table_city_0, DIR_N },
};

/** Tiles for Metropolitain Airport (large) - 2 runways */
static const std::initializer_list<AirportTileTable> _tile_table_metropolitan_0 = {
	MK(0, 0, APT_BUILDING_1),
	MK(1, 0, APT_APRON_FENCE_NW),
	MK(2, 0, APT_STAND_1),
	MK(3, 0, APT_APRON_FENCE_NW),
	MK(4, 0, APT_APRON_FENCE_NW),
	MK(5, 0, APT_DEPOT_SE),
	MK(0, 1, APT_BUILDING_2),
	MK(1, 1, APT_PIER),
	MK(2, 1, APT_ROUND_TERMINAL),
	MK(3, 1, APT_STAND_PIER_NE),
	MK(4, 1, APT_APRON),
	MK(5, 1, APT_APRON_FENCE_SW),
	MK(0, 2, APT_BUILDING_3),
	MK(1, 2, APT_STAND),
	MK(2, 2, APT_PIER_NW_NE),
	MK(3, 2, APT_APRON_S),
	MK(4, 2, APT_APRON_HOR),
	MK(5, 2, APT_APRON_N_FENCE_SW),
	MK(0, 3, APT_RADAR_FENCE_NE),
	MK(1, 3, APT_APRON),
	MK(2, 3, APT_APRON),
	MK(3, 3, APT_APRON),
	MK(4, 3, APT_APRON),
	MK(5, 3, APT_TOWER_FENCE_SW),
	MK(0, 4, APT_RUNWAY_END),
	MK(1, 4, APT_RUNWAY_5),
	MK(2, 4, APT_RUNWAY_5),
	MK(3, 4, APT_RUNWAY_5),
	MK(4, 4, APT_RUNWAY_5),
	MK(5, 4, APT_RUNWAY_END),
	MK(0, 5, APT_RUNWAY_END_FENCE_SE),
	MK(1, 5, APT_RUNWAY_2),
	MK(2, 5, APT_RUNWAY_2),
	MK(3, 5, APT_RUNWAY_2),
	MK(4, 5, APT_RUNWAY_2),
	MK(5, 5, APT_RUNWAY_END_FENCE_SE),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_metropolitan = {
	{ _tile_table_metropolitan_0, DIR_N },
};

/** Tiles for International Airport (large) - 2 runways */
static const std::initializer_list<AirportTileTable> _tile_table_international_0 = {
	MK(0, 0, APT_RUNWAY_END_FENCE_NW),
	MK(1, 0, APT_RUNWAY_FENCE_NW),
	MK(2, 0, APT_RUNWAY_FENCE_NW),
	MK(3, 0, APT_RUNWAY_FENCE_NW),
	MK(4, 0, APT_RUNWAY_FENCE_NW),
	MK(5, 0, APT_RUNWAY_FENCE_NW),
	MK(6, 0, APT_RUNWAY_END_FENCE_NW),
	MK(0, 1, APT_RADIO_TOWER_FENCE_NE),
	MK(1, 1, APT_APRON),
	MK(2, 1, APT_APRON),
	MK(3, 1, APT_APRON),
	MK(4, 1, APT_APRON),
	MK(5, 1, APT_APRON),
	MK(6, 1, APT_DEPOT_SE),
	MK(0, 2, APT_BUILDING_3),
	MK(1, 2, APT_APRON),
	MK(2, 2, APT_STAND),
	MK(3, 2, APT_BUILDING_2),
	MK(4, 2, APT_STAND),
	MK(5, 2, APT_APRON),
	MK(6, 2, APT_APRON_FENCE_SW),
	MK(0, 3, APT_DEPOT_SE),
	MK(1, 3, APT_APRON),
	MK(2, 3, APT_STAND),
	MK(3, 3, APT_BUILDING_2),
	MK(4, 3, APT_STAND),
	MK(5, 3, APT_APRON),
	MK(6, 3, APT_HELIPAD_1),
	MK(0, 4, APT_APRON_FENCE_NE),
	MK(1, 4, APT_APRON),
	MK(2, 4, APT_STAND),
	MK(3, 4, APT_TOWER),
	MK(4, 4, APT_STAND),
	MK(5, 4, APT_APRON),
	MK(6, 4, APT_HELIPAD_1),
	MK(0, 5, APT_APRON_FENCE_NE),
	MK(1, 5, APT_APRON),
	MK(2, 5, APT_APRON),
	MK(3, 5, APT_APRON),
	MK(4, 5, APT_APRON),
	MK(5, 5, APT_APRON),
	MK(6, 5, APT_RADAR_FENCE_SW),
	MK(0, 6, APT_RUNWAY_END_FENCE_SE),
	MK(1, 6, APT_RUNWAY_2),
	MK(2, 6, APT_RUNWAY_2),
	MK(3, 6, APT_RUNWAY_2),
	MK(4, 6, APT_RUNWAY_2),
	MK(5, 6, APT_RUNWAY_2),
	MK(6, 6, APT_RUNWAY_END_FENCE_SE),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_international = {
	{ _tile_table_international_0, DIR_N },
};

/** Tiles for International Airport (large) - 2 runways */
static const std::initializer_list<AirportTileTable> _tile_table_intercontinental_0 = {
	MK(0, 0, APT_RADAR_FENCE_NE),
	MK(1, 0, APT_RUNWAY_END_FENCE_NE_NW),
	MK(2, 0, APT_RUNWAY_FENCE_NW),
	MK(3, 0, APT_RUNWAY_FENCE_NW),
	MK(4, 0, APT_RUNWAY_FENCE_NW),
	MK(5, 0, APT_RUNWAY_FENCE_NW),
	MK(6, 0, APT_RUNWAY_FENCE_NW),
	MK(7, 0, APT_RUNWAY_FENCE_NW),
	MK(8, 0, APT_RUNWAY_END_FENCE_NW_SW),
	MK(0, 1, APT_RUNWAY_END_FENCE_NE_NW),
	MK(1, 1, APT_RUNWAY_2),
	MK(2, 1, APT_RUNWAY_2),
	MK(3, 1, APT_RUNWAY_2),
	MK(4, 1, APT_RUNWAY_2),
	MK(5, 1, APT_RUNWAY_2),
	MK(6, 1, APT_RUNWAY_2),
	MK(7, 1, APT_RUNWAY_END_FENCE_SE_SW),
	MK(8, 1, APT_APRON_FENCE_NE_SW),
	MK(0, 2, APT_APRON_FENCE_NE_SW),
	MK(1, 2, APT_EMPTY),
	MK(2, 2, APT_APRON_FENCE_NE),
	MK(3, 2, APT_APRON),
	MK(4, 2, APT_APRON),
	MK(5, 2, APT_APRON),
	MK(6, 2, APT_APRON),
	MK(7, 2, APT_RADIO_TOWER_FENCE_NE),
	MK(8, 2, APT_APRON_FENCE_NE_SW),
	MK(0, 3, APT_APRON_FENCE_NE),
	MK(1, 3, APT_APRON_HALF_EAST),
	MK(2, 3, APT_APRON_FENCE_NE),
	MK(3, 3, APT_TOWER),
	MK(4, 3, APT_HELIPAD_2),
	MK(5, 3, APT_HELIPAD_2),
	MK(6, 3, APT_APRON),
	MK(7, 3, APT_APRON_FENCE_NW),
	MK(8, 3, APT_APRON_FENCE_SW),
	MK(0, 4, APT_APRON_FENCE_NE),
	MK(1, 4, APT_APRON),
	MK(2, 4, APT_APRON),
	MK(3, 4, APT_STAND),
	MK(4, 4, APT_BUILDING_1),
	MK(5, 4, APT_STAND),
	MK(6, 4, APT_APRON),
	MK(7, 4, APT_LOW_BUILDING),
	MK(8, 4, APT_DEPOT_SE),
	MK(0, 5, APT_DEPOT_SE),
	MK(1, 5, APT_LOW_BUILDING),
	MK(2, 5, APT_APRON),
	MK(3, 5, APT_STAND),
	MK(4, 5, APT_BUILDING_2),
	MK(5, 5, APT_STAND),
	MK(6, 5, APT_APRON),
	MK(7, 5, APT_APRON),
	MK(8, 5, APT_APRON_FENCE_SW),
	MK(0, 6, APT_APRON_FENCE_NE),
	MK(1, 6, APT_APRON),
	MK(2, 6, APT_APRON),
	MK(3, 6, APT_STAND),
	MK(4, 6, APT_BUILDING_3),
	MK(5, 6, APT_STAND),
	MK(6, 6, APT_APRON),
	MK(7, 6, APT_APRON),
	MK(8, 6, APT_APRON_FENCE_SW),
	MK(0, 7, APT_APRON_FENCE_NE),
	MK(1, 7, APT_APRON_FENCE_SE),
	MK(2, 7, APT_APRON),
	MK(3, 7, APT_STAND),
	MK(4, 7, APT_ROUND_TERMINAL),
	MK(5, 7, APT_STAND),
	MK(6, 7, APT_APRON_FENCE_SW),
	MK(7, 7, APT_APRON_HALF_WEST),
	MK(8, 7, APT_APRON_FENCE_SW),
	MK(0, 8, APT_APRON_FENCE_NE),
	MK(1, 8, APT_GRASS_FENCE_NE_FLAG_2),
	MK(2, 8, APT_APRON_FENCE_NE),
	MK(3, 8, APT_APRON),
	MK(4, 8, APT_APRON),
	MK(5, 8, APT_APRON),
	MK(6, 8, APT_APRON_FENCE_SW),
	MK(7, 8, APT_EMPTY),
	MK(8, 8, APT_APRON_FENCE_NE_SW),
	MK(0, 9, APT_APRON_FENCE_NE),
	MK(1, 9, APT_RUNWAY_END_FENCE_NE_NW),
	MK(2, 9, APT_RUNWAY_FENCE_NW),
	MK(3, 9, APT_RUNWAY_FENCE_NW),
	MK(4, 9, APT_RUNWAY_FENCE_NW),
	MK(5, 9, APT_RUNWAY_FENCE_NW),
	MK(6, 9, APT_RUNWAY_FENCE_NW),
	MK(7, 9, APT_RUNWAY_FENCE_NW),
	MK(8, 9, APT_RUNWAY_END_FENCE_SE_SW),
	MK(0, 10, APT_RUNWAY_END_FENCE_NE_SE),
	MK(1, 10, APT_RUNWAY_2),
	MK(2, 10, APT_RUNWAY_2),
	MK(3, 10, APT_RUNWAY_2),
	MK(4, 10, APT_RUNWAY_2),
	MK(5, 10, APT_RUNWAY_2),
	MK(6, 10, APT_RUNWAY_2),
	MK(7, 10, APT_RUNWAY_END_FENCE_SE_SW),
	MK(8, 10, APT_EMPTY),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_intercontinental = {
	{ _tile_table_intercontinental_0, DIR_N },
};

/** Tiles for Heliport */
static const std::initializer_list<AirportTileTable> _tile_table_heliport_0 = {
	MK(0, 0, APT_HELIPORT),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_heliport = {
	{ _tile_table_heliport_0, DIR_N },
};

/** Tiles for Helidepot */
static const std::initializer_list<AirportTileTable> _tile_table_helidepot_0 = {
	MK(0, 0, APT_LOW_BUILDING_FENCE_N),
	MK(1, 0, APT_DEPOT_SE),
	MK(0, 1, APT_HELIPAD_2_FENCE_NE_SE),
	MK(1, 1, APT_APRON_FENCE_SE_SW),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_helidepot = {
	{ _tile_table_helidepot_0, DIR_N },
};

/** Tiles for Helistation */
static const std::initializer_list<AirportTileTable> _tile_table_helistation_0 = {
	MK(0, 0, APT_DEPOT_SE),
	MK(1, 0, APT_LOW_BUILDING_FENCE_NW),
	MK(2, 0, APT_HELIPAD_3_FENCE_NW),
	MK(3, 0, APT_HELIPAD_3_FENCE_NW_SW),
	MK(0, 1, APT_APRON_FENCE_NE_SE),
	MK(1, 1, APT_APRON_FENCE_SE),
	MK(2, 1, APT_APRON_FENCE_SE),
	MK(3, 1, APT_HELIPAD_3_FENCE_SE_SW),
	MKEND
};

static const std::initializer_list<AirportTileLayout> _tile_table_helistation = {
	{ _tile_table_helistation_0, DIR_N },
};

#undef MK
#undef MKEND

/** General AirportSpec definition. */
#define AS_GENERIC(fsm, layouts, depots, size_x, size_y, noise, catchment, min_year, max_year, maint_cost, ttdpatch_type, class_id, name, preview, enabled) \
	{{class_id, 0}, fsm, layouts, depots, size_x, size_y, noise, catchment, min_year, max_year, name, ttdpatch_type, preview, maint_cost, enabled, GRFFileProps(AT_INVALID)}

/** AirportSpec definition for airports without any depot. */
#define AS_ND(ap_name, size_x, size_y, min_year, max_year, catchment, noise, maint_cost, ttdpatch_type, class_id, name, preview) \
	AS_GENERIC(&_airportfta_##ap_name, _tile_table_##ap_name, {}, \
		size_x, size_y, noise, catchment, min_year, max_year, maint_cost, ttdpatch_type, class_id, name, preview, true)

/** AirportSpec definition for airports with at least one depot. */
#define AS(ap_name, size_x, size_y, min_year, max_year, catchment, noise, maint_cost, ttdpatch_type, class_id, name, preview) \
	AS_GENERIC(&_airportfta_##ap_name, _tile_table_##ap_name, _airport_depots_##ap_name, \
		size_x, size_y, noise, catchment, min_year, max_year, maint_cost, ttdpatch_type, class_id, name, preview, true)

/* The helidepot and helistation have ATP_TTDP_SMALL because they are at ground level */
const AirportSpec _origin_airport_specs[] = {
	AS(country,          4, 3,     0,     1959,  4,  3,  7, ATP_TTDP_SMALL,    APC_SMALL,    STR_AIRPORT_SMALL,            SPR_AIRPORT_PREVIEW_SMALL),
	AS(city,             6, 6,  1955, CalendarTime::MAX_YEAR,  5,  7, 24, ATP_TTDP_LARGE,    APC_LARGE,    STR_AIRPORT_CITY,             SPR_AIRPORT_PREVIEW_LARGE),
	AS_ND(heliport,      1, 1,  1963, CalendarTime::MAX_YEAR,  4,  2,  4, ATP_TTDP_HELIPORT, APC_HELIPORT, STR_AIRPORT_HELIPORT,         SPR_AIRPORT_PREVIEW_HELIPORT),
	AS(metropolitan,     6, 6,  1980, CalendarTime::MAX_YEAR,  6, 10, 28, ATP_TTDP_LARGE,    APC_LARGE,    STR_AIRPORT_METRO,            SPR_AIRPORT_PREVIEW_METROPOLITAN),
	AS(international,    7, 7,  1990, CalendarTime::MAX_YEAR,  8, 15, 42, ATP_TTDP_LARGE,    APC_HUB,      STR_AIRPORT_INTERNATIONAL,    SPR_AIRPORT_PREVIEW_INTERNATIONAL),
	AS(commuter,         5, 4,  1983, CalendarTime::MAX_YEAR,  4,  7, 20, ATP_TTDP_SMALL,    APC_SMALL,    STR_AIRPORT_COMMUTER,         SPR_AIRPORT_PREVIEW_COMMUTER),
	AS(helidepot,        2, 2,  1976, CalendarTime::MAX_YEAR,  4,  2,  7, ATP_TTDP_SMALL,    APC_HELIPORT, STR_AIRPORT_HELIDEPOT,        SPR_AIRPORT_PREVIEW_HELIDEPOT),
	AS(intercontinental, 9, 11, 2002, CalendarTime::MAX_YEAR, 10, 23, 72, ATP_TTDP_LARGE,    APC_HUB,      STR_AIRPORT_INTERCONTINENTAL, SPR_AIRPORT_PREVIEW_INTERCONTINENTAL),
	AS(helistation,      4, 2,  1980, CalendarTime::MAX_YEAR,  4,  4, 14, ATP_TTDP_SMALL,    APC_HELIPORT, STR_AIRPORT_HELISTATION,      SPR_AIRPORT_PREVIEW_HELISTATION),
	AS_GENERIC(&_airportfta_oilrig, {}, _default_airports_rotation, 0, nullptr, 0, 1, 1, 0, 4, 0, 0, 0, ATP_TTDP_OILRIG, APC_HELIPORT, STR_NULL, 0, false),
};

static_assert(NEW_AIRPORT_OFFSET == lengthof(_origin_airport_specs));

#define AS_GENERIC(fsm, att, rot, att_len, depot_tbl, num_depots, size_x, size_y, noise, catchment, min_year, max_year, maint_cost, ttdpatch_type, class_id, name, preview, enabled) \
{fsm, att, rot, att_len, depot_tbl, num_depots, size_x, size_y, noise, catchment, min_year, max_year, name, ttdpatch_type, class_id, preview, maint_cost, enabled, GRFFileProps(AT_INVALID)}

#undef AS
#undef AS_ND
#undef AS_GENERIC

struct TileDescription {
	const AirType ground;        // The ground type for this tile.
	const bool tracks;           // The tile can contain tracks for aircraft.
	const AirportTileType type;  // Use this tile will have (terminal, tracks,...).
	const TerminalType terminal_type; // Subtype of terminal.
	const DiagDirection dir;     // Direction of runway or exit direction of hangars.
	const TrackBits trackbits;   // Tracks for this tile.
	const Direction runway_directions; // Directions of the runways present on this tile.
                                 // Maps a direction into the diagonal directions of the runways.
	const AirportTiles gfx_id;   // Sprite for this tile.

	/* Description for simple track tiles. */
	TileDescription(AirType at, AirportTileType att, TrackBits trackbits) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(trackbits),
			runway_directions(INVALID_DIR),
			gfx_id((AirportTiles)0)
	{
		assert(tracks);
		assert(att == ATT_SIMPLE_TRACK);
	}

	/* Description for terminals. */
	TileDescription(AirType at, AirportTileType att, TrackBits trackbits, TerminalType type) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(type),
			dir(INVALID_DIAGDIR),
			trackbits(trackbits),
			runway_directions(INVALID_DIR),
			gfx_id((AirportTiles)0)
	{
		assert(tracks);
	}

	/* Description for hangars and runway end and start. */
	TileDescription(AirType at, AirportTileType att, TrackBits trackbits, DiagDirection dir) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(dir),
			trackbits(trackbits),
			runway_directions(INVALID_DIR),
			gfx_id((AirportTiles)0)
	{
		assert(att == ATT_HANGAR_STANDARD ||
				att == ATT_HANGAR_EXTENDED ||
				att == ATT_RUNWAY_END ||
				att == ATT_RUNWAY_START_ALLOW_LANDING ||
				att == ATT_RUNWAY_START_NO_LANDING);
	}

	/* Description for runway (not end, not start). */
	TileDescription(AirType at, AirportTileType att, TrackBits trackbits, Direction runway_directions) :
			ground(at),
			tracks(true),
			type(att),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(trackbits),
			runway_directions(runway_directions),
			gfx_id((AirportTiles)0)
	{
		assert(tracks);
		assert(att == ATT_RUNWAY_MIDDLE);
		assert(IsValidDirection(runway_directions));
	}

	/* Description for infrastructure. */
	TileDescription(AirType at, AirportTileType att, AirportTiles gfx_id) :
			ground(at),
			tracks(false),
			type(att),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(TRACK_BIT_NONE),
			runway_directions(INVALID_DIR),
			gfx_id(gfx_id)
	{
		assert(att == ATT_INFRASTRUCTURE_WITH_CATCH || att == ATT_INFRASTRUCTURE_NO_CATCH);
	}

	/* Description for a non-airport tile. */
	TileDescription(AirType at) :
			ground(at),
			tracks(false),
			type(ATT_INVALID),
			terminal_type(HTT_INVALID),
			dir(INVALID_DIAGDIR),
			trackbits(TRACK_BIT_NONE),
			runway_directions(INVALID_DIR),
			gfx_id((AirportTiles)0) {}
};

/** Tiles for Country Airfield (small) */
static const TileDescription _description_country_0[] = {
	TileDescription(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE_WITH_CATCH,	APT_SMALL_BUILDING_1									),
	TileDescription(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE_WITH_CATCH,	APT_SMALL_BUILDING_2									),
	TileDescription(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE_WITH_CATCH,	APT_SMALL_BUILDING_3									),
	TileDescription(AIRTYPE_GRAVEL,		ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_GRAVEL,		ATT_INFRASTRUCTURE_NO_CATCH,	APT_GRASS_FENCE_NE_FLAG									),
	TileDescription(AIRTYPE_GRAVEL,		ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_GRAVEL,		ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_GRAVEL,		ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_RIGHT						),

	TileDescription(AIRTYPE_GRAVEL,		ATT_RUNWAY_END, 				TRACK_BIT_X,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_GRAVEL,		ATT_RUNWAY_MIDDLE,				TRACK_BIT_CROSS | TRACK_BIT_LEFT,		DIR_NE			),
	TileDescription(AIRTYPE_GRAVEL,		ATT_RUNWAY_MIDDLE,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE,		DIR_NE			),
	TileDescription(AIRTYPE_GRAVEL,		ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_CROSS | TRACK_BIT_UPPER,		DIAGDIR_NE		),
};

/** Tiles for Commuter Airfield (small) */
static const TileDescription _description_commuter_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_TOWER												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_3											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_Y,							HTT_HELIPAD		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_Y,							HTT_HELIPAD		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_LOWER											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_UPPER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_LEFT							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_RIGHT						),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y | TRACK_BIT_LOWER							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW						),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_Y,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_Y,							DIAGDIR_NE		),
};

/** Tiles for City Airport (large) */
static const TileDescription _description_city_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_1											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_APRON_FENCE_NW										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_X,							HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_2											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_PIER												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_ROUND_TERMINAL										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_3											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_Y,							HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_PIER_NW_NE											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_UPPER											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_APRON_N_FENCE_SW									),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADIO_TOWER_FENCE_NE								),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT | TRACK_BIT_LOWER | TRACK_BIT_X			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_ARPON_N												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_TOWER_FENCE_SW										),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_EMPTY_FENCE_NE										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_LOWER | TRACK_BIT_Y							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_UPPER | TRACK_BIT_LEFT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT | TRACK_BIT_Y							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_APRON_E												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADAR_GRASS_FENCE_SW								),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_X,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_CROSS,						DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_CROSS,						DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_X,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_X,							DIAGDIR_NE		),
};

/** Tiles for Metropolitan Airport (large) - 2 runways */
static const TileDescription _description_metropolitan_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_1											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_APRON_FENCE_NW										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_X,							HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_2											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_PIER												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_ROUND_TERMINAL										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_3											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_Y,							HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_PIER_NW_NE											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y | TRACK_BIT_LOWER							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y | TRACK_BIT_UPPER							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_APRON_N_FENCE_SW									),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADAR_FENCE_NE										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_HORZ							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_TOWER_FENCE_SW										),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_NONE,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_LOWER | TRACK_BIT_Y,			DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_UPPER,						DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_LEFT,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_CROSS,						DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_NO_LANDING,	TRACK_BIT_X,							DIAGDIR_NE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_X,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_CROSS,						DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_NONE,							DIAGDIR_NE		),
};

/** Tiles for International Airport (large) - 2 runways */
static const TileDescription _description_international_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_NO_LANDING,	TRACK_BIT_X,							DIAGDIR_SW		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_CROSS,						DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_RIGHT,						DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_NONE,							DIAGDIR_SW		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADIO_TOWER_FENCE_NE								),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_RIGHT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_X												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LOWER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT | TRACK_BIT_CROSS						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_3											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_2											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_LEFT							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_RIGHT						),

	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_2											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_CROSS,						HTT_HELIPAD		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LOWER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_UPPER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_TOWER												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_LOWER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_CROSS,						HTT_HELIPAD		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y | TRACK_BIT_LOWER | TRACK_BIT_LEFT			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_UPPER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_X												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LEFT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_UPPER | TRACK_BIT_CROSS						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADAR_FENCE_SW										),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_Y,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_NONE,							DIAGDIR_NE		),
};

/** Tiles for Intercontinental Airport (large) - 4 runways */
static const TileDescription _description_intercontinental_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADAR_FENCE_NE										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_NONE,							DIAGDIR_SW		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_Y,							DIAGDIR_SW		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_NO_LANDING,	TRACK_BIT_Y,							DIAGDIR_SW		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_SW			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_NONE,							DIAGDIR_SW		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y												),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_EMPTY												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_X												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LOWER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT | TRACK_BIT_CROSS						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_RADIO_TOWER_FENCE_NE								),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y												),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_RIGHT | TRACK_BIT_X							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_TOWER												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_Y,							HTT_HELIPAD		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_CROSS,						HTT_HELIPAD		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_UPPER | TRACK_BIT_RIGHT		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_X												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_LEFT											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_LEFT | TRACK_BIT_X							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_HORZ						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_1											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_LOW_BUILDING										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_LOW_BUILDING										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_2											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_LEFT							),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_RIGHT						),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LOWER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_UPPER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_BUILDING_3											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW						),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LEFT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_ALL & ~TRACK_BIT_RIGHT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_ROUND_TERMINAL										),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_NORMAL,			TRACK_BIT_CROSS,						HTT_TERMINAL	),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_VERT | TRACK_BIT_UPPER		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_LEFT | TRACK_BIT_X | TRACK_BIT_UPPER			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_GRASS_FENCE_NE_FLAG_2								),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_LEFT | TRACK_BIT_CROSS						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_UPPER						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_X												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LEFT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_UPPER | TRACK_BIT_CROSS						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_EMPTY												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y												),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_Y												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_NONE,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_NO_LANDING, 	TRACK_BIT_Y,							DIAGDIR_NE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_END,					TRACK_BIT_Y,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_MIDDLE,				TRACK_BIT_NONE,							DIR_NE			),
	TileDescription(AIRTYPE_ASPHALT,	ATT_RUNWAY_START_ALLOW_LANDING,	TRACK_BIT_NONE,							DIAGDIR_NE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_EMPTY												),
};

/** Tiles for Heliport */
static const TileDescription _description_heliport_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPORT,			TRACK_BIT_NONE,							HTT_HELIPORT	),
};

/** Tiles for Helidepot */
static const TileDescription _description_helidepot_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_NO_CATCH,	APT_LOW_BUILDING_FENCE_N								),
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_X,							HTT_HELIPAD		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
};

/** Tiles for Helistation */
static const TileDescription _description_helistation_0[] = {
	TileDescription(AIRTYPE_ASPHALT,	ATT_HANGAR_STANDARD,			TRACK_BIT_Y,							DIAGDIR_SE		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_INFRASTRUCTURE_WITH_CATCH,	APT_LOW_BUILDING_FENCE_NW								),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_Y | TRACK_BIT_LOWER,			HTT_HELIPAD		),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_CROSS,						HTT_HELIPAD		),

	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS											),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_X												),
	TileDescription(AIRTYPE_ASPHALT,	ATT_SIMPLE_TRACK,				TRACK_BIT_CROSS | TRACK_BIT_LEFT						),
	TileDescription(AIRTYPE_ASPHALT,	ATT_TERMINAL_HELIPAD,			TRACK_BIT_X | TRACK_BIT_UPPER,			HTT_HELIPAD		),
};

/** Tiles for Oil rig */
static const TileDescription _description_oilrig_0[] = {
	TileDescription(AIRTYPE_WATER,		ATT_TERMINAL_BUILTIN_HELIPORT,	TRACK_BIT_NONE,							HTT_BUILTIN_HELIPORT),
};

static const TileDescription *_description_airport_specs[] = {
	_description_country_0,
	_description_city_0,
	_description_heliport_0,
	_description_metropolitan_0,
	_description_international_0,
	_description_commuter_0,
	_description_helidepot_0,
	_description_intercontinental_0,
	_description_helistation_0,
	_description_oilrig_0,
};

static const bool _description_airport_hangars[] = {
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

static const bool _description_airport_heliport[] = {
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

static_assert(NEW_AIRPORT_OFFSET == lengthof(_description_airport_hangars));
static_assert(NEW_AIRPORT_OFFSET == lengthof(_description_airport_specs));

#endif /* AIRPORT_DEFAULTS_H */
