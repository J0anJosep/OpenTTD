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

/** Tiles for Country Airfield (small) */
static const std::initializer_list<AirportTileTable> _description_country = {
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_3,            DIAGDIR_NW,                APT_SMALL_BUILDING_1),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_2,            DIAGDIR_NW,                APT_SMALL_BUILDING_2),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_1,            DIAGDIR_NW,                APT_SMALL_BUILDING_3),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_SMALL_DEPOT_SE),

	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_FLAG,                    DIAGDIR_NE,                APT_GRASS_FENCE_NE_FLAG),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_GRASS_1),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_GRASS_2),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_RIGHT,                                APT_GRASS_FENCE_SW),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_X,                           DIAGDIR_NE,                APT_RUNWAY_SMALL_FAR_END),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_CROSS | TRACK_BIT_LEFT,      DIR_NE,                    APT_RUNWAY_SMALL_MIDDLE),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE,    DIR_NE,                    APT_RUNWAY_SMALL_MIDDLE),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_CROSS | TRACK_BIT_UPPER,     DIAGDIR_NE,                APT_RUNWAY_SMALL_NEAR_END),
};

/** Tiles for Commuter Airfield (small) */
static const std::initializer_list<AirportTileTable> _description_commuter = {
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TOWER,                   DIAGDIR_NE,                APT_TOWER),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_3,            DIAGDIR_SE,                APT_BUILDING_3),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_Y,                           APRON_HELIPAD,             APT_HELIPAD_2_FENCE_NW),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_Y,                           APRON_HELIPAD,             APT_HELIPAD_2_FENCE_NW),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_LOWER,                                                  APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X | TRACK_BIT_LOWER,                                    APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_UPPER,                                 APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_LEFT,                                  APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_RIGHT,                                APT_APRON_FENCE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y | TRACK_BIT_LOWER,                                    APT_APRON_FENCE_NE),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW,                               APT_APRON_FENCE_SW),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_Y,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_Y,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
};

/** Tiles for City Airport (large) */
static const std::initializer_list<AirportTileTable> _description_city = {
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_1,            DIAGDIR_NE,                APT_BUILDING_1),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_EMPTY,                   DIAGDIR_NE,                APT_APRON_FENCE_NW),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND_1),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON_FENCE_NW),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_RIGHT,                                                  APT_APRON_FENCE_NW),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),

	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_2,            DIAGDIR_NE,                APT_BUILDING_2),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_PIER,                    DIAGDIR_NE,                APT_PIER),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_TERMINAL,     DIAGDIR_NE,                APT_ROUND_TERMINAL),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND_PIER_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER,                                APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_SW),

	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_3,            DIAGDIR_NE,                APT_BUILDING_3),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_Y,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_PIER_NW_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER,                                APT_APRON_S),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_UPPER,                                                  APT_APRON_HOR),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_APRON_N_FENCE_SW),

	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TRANSMITTER,             DIAGDIR_NE,                APT_RADIO_TOWER_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LEFT,                                 APT_APRON_W),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_RIGHT | TRACK_BIT_LOWER | TRACK_BIT_X,                  APT_APRON_VER_CROSSING_S),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_UPPER,                                APT_APRON_HOR_CROSSING_E),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_ARPON_N),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TOWER,                   DIAGDIR_NE,                APT_TOWER_FENCE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_EMPTY_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_LOWER | TRACK_BIT_Y,                                    APT_APRON_S),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_UPPER | TRACK_BIT_LEFT,                                 APT_APRON_HOR_CROSSING_W),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_RIGHT | TRACK_BIT_Y,                                    APT_APRON_VER_CROSSING_N),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_APRON_E),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_RADAR,                   DIAGDIR_NE,                APT_RADAR_GRASS_FENCE_SW),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_X,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_UPPER,                       DIR_NE,                    APT_RUNWAY_1),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_LEFT,                        DIR_NE,                    APT_RUNWAY_3),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_X,                           DIR_NE,                    APT_RUNWAY_4),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_X,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
};

/** Tiles for Metropolitan Airport (large) - 2 runways */
static const std::initializer_list<AirportTileTable> _description_metropolitan = {
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_1,            DIAGDIR_NE,                APT_BUILDING_1),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_EMPTY,                   DIAGDIR_NE,                APT_APRON_FENCE_NW),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND_1),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON_FENCE_NW),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_RIGHT,                                                  APT_APRON_FENCE_NW),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),

	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_2,            DIAGDIR_NE,                APT_BUILDING_2),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_PIER,                    DIAGDIR_NE,                APT_PIER),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_TERMINAL,     DIAGDIR_NE,                APT_ROUND_TERMINAL),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND_PIER_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER,                                APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_SW),

	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_3,            DIAGDIR_NE,                APT_BUILDING_3),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_Y,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_PIER_NW_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y | TRACK_BIT_LOWER,                                    APT_APRON_S),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y | TRACK_BIT_UPPER,                                    APT_APRON_HOR),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_APRON_N_FENCE_SW),

	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_RADAR,                   DIAGDIR_NE,                APT_RADAR_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LEFT,                                 APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW,                               APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_LOWER,                                 APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW,                               APT_APRON),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TOWER,                   DIAGDIR_NE,                APT_TOWER_FENCE_SW),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_NONE,                        DIAGDIR_NE,                APT_RUNWAY_END),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_LOWER | TRACK_BIT_Y,         DIR_NE,                    APT_RUNWAY_5),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_UPPER,                       DIR_NE,                    APT_RUNWAY_5),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_LEFT,                        DIR_NE,                    APT_RUNWAY_5),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_CROSS,                       DIR_NE,                    APT_RUNWAY_5),
	AirportTileTable(ATT_RUNWAY_START_NO_LANDING,      TRACK_BIT_X,                           DIAGDIR_NE,                APT_RUNWAY_END),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_X,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_UPPER,                       DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_NONE,                        DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
};

/** Tiles for International Airport (large) - 2 runways */
static const std::initializer_list<AirportTileTable> _description_international = {
	AirportTileTable(ATT_RUNWAY_START_NO_LANDING,      TRACK_BIT_X,                           DIAGDIR_SW,                APT_RUNWAY_END_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_CROSS,                       DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_RIGHT,                       DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_NONE,                        DIAGDIR_SW,                APT_RUNWAY_END_FENCE_NW),

	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TRANSMITTER,             DIAGDIR_NE,                APT_RADIO_TOWER_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE,                               APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_RIGHT,                                APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER,                                APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_RIGHT | TRACK_BIT_CROSS,                                APT_APRON),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),

	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_3,            DIAGDIR_NE,                APT_BUILDING_3),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE,                               APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_2,            DIAGDIR_NE,                APT_BUILDING_2),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_UPPER | TRACK_BIT_RIGHT,              APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_SW),

	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE,                               APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_2,            DIAGDIR_NE,                APT_BUILDING_2),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL,                                                    APT_APRON),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_CROSS,                       APRON_HELIPAD,             APT_HELIPAD_1),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER | TRACK_BIT_LEFT,               APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TOWER,                   DIAGDIR_NE,                APT_TOWER),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_LOWER,                                 APT_APRON),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_CROSS,                       APRON_HELIPAD,             APT_HELIPAD_1),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y | TRACK_BIT_LOWER | TRACK_BIT_LEFT,                   APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SE,                               APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_UPPER,                                APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LEFT,                                 APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_UPPER | TRACK_BIT_CROSS,                                APT_APRON),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_RADAR,                   DIAGDIR_NE,                APT_RADAR_FENCE_SW),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_Y,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_NONE,                        DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE),
};

/** Tiles for Intercontinental Airport (large) - 4 runways */
static const std::initializer_list<AirportTileTable> _description_intercontinental = {
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_RADAR,                   DIAGDIR_NE,                APT_RADAR_FENCE_NE),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_NONE,                        DIAGDIR_SW,                APT_RUNWAY_END_FENCE_NE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_Y,                           DIAGDIR_SW,                APT_RUNWAY_END_FENCE_NW_SW),

	AirportTileTable(ATT_RUNWAY_START_NO_LANDING,      TRACK_BIT_Y,                           DIAGDIR_SW,                APT_RUNWAY_END_FENCE_NE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_SW,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_NONE,                        DIAGDIR_SW,                APT_RUNWAY_END_FENCE_SE_SW),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE_SW),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_EMPTY,                   DIAGDIR_NE,                APT_EMPTY),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER,                                APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NW,                               APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TRANSMITTER,             DIAGDIR_NE,                APT_RADIO_TOWER_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_APRON_HALF_EAST),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_TOWER,                   DIAGDIR_NE,                APT_TOWER),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_Y,                           APRON_HELIPAD,             APT_HELIPAD_2),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_CROSS,                       APRON_HELIPAD,             APT_HELIPAD_2),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_UPPER | TRACK_BIT_RIGHT,              APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON_FENCE_NW),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_LEFT,                                                   APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_1,            DIAGDIR_NE,                APT_BUILDING_1),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW,                               APT_APRON),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_FLAT,         DIAGDIR_NE,                APT_LOW_BUILDING),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),

	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_FLAT,         DIAGDIR_NE,                APT_LOW_BUILDING),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_NE,                               APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_2,            DIAGDIR_NE,                APT_BUILDING_2),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_UPPER | TRACK_BIT_RIGHT,              APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_RIGHT,                                APT_APRON_FENCE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LOWER,                                APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_UPPER,                                 APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_3,            DIAGDIR_NE,                APT_BUILDING_3),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_X,                           APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_LOWER,                                 APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_3WAY_SW,                               APT_APRON_FENCE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LEFT,                                 APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON_FENCE_SE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_ALL & ~TRACK_BIT_RIGHT & ~TRACK_BIT_LOWER,              APT_APRON),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_TERMINAL,     DIAGDIR_NE,                APT_ROUND_TERMINAL),
	AirportTileTable(ATT_APRON_NORMAL,                 TRACK_BIT_CROSS,                       APRON_APRON,               APT_STAND),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y | TRACK_BIT_UPPER,                                    APT_APRON_FENCE_SW),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_NONE,                                                   APT_APRON_HALF_WEST),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_FLAG,                    DIAGDIR_NE,                APT_GRASS_FENCE_NE_FLAG_2),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_LEFT | TRACK_BIT_CROSS,                                 APT_APRON_FENCE_NE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_UPPER | TRACK_BIT_CROSS,                                APT_APRON_FENCE_SW),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_EMPTY,                   DIAGDIR_NE,                APT_EMPTY),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_Y,                                                      APT_APRON_FENCE_NE),
	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_NONE,                        DIAGDIR_NE,                APT_RUNWAY_END_FENCE_NE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_FENCE_NW),
	AirportTileTable(ATT_RUNWAY_START_NO_LANDING,      TRACK_BIT_Y,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE_SW),

	AirportTileTable(ATT_RUNWAY_END,                   TRACK_BIT_Y,                           DIAGDIR_NE,                APT_RUNWAY_END_FENCE_NE_SE),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_MIDDLE,                TRACK_BIT_NONE,                        DIR_NE,                    APT_RUNWAY_2),
	AirportTileTable(ATT_RUNWAY_START_ALLOW_LANDING,   TRACK_BIT_NONE,                        DIAGDIR_NE,                APT_RUNWAY_END_FENCE_SE_SW),
	AirportTileTable(ATT_INFRASTRUCTURE_NO_CATCH,      ATTG_NO_CATCH_EMPTY,                   DIAGDIR_NE,                APT_EMPTY),
};

/** Tiles for Heliport */
static const std::initializer_list<AirportTileTable> _description_heliport = {
	AirportTileTable(ATT_APRON_HELIPORT,               TRACK_BIT_CROSS,                       APRON_HELIPORT,            APT_HELIPORT),
};

/** Tiles for Helidepot */
static const std::initializer_list<AirportTileTable> _description_helidepot = {
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_FLAT,         DIAGDIR_NE,                APT_LOW_BUILDING_FENCE_N),
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),

	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_X,                           APRON_HELIPAD,             APT_HELIPAD_2_FENCE_NE_SE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_SE_SW),
};

/** Tiles for Helistation */
static const std::initializer_list<AirportTileTable> _description_helistation = {
	AirportTileTable(ATT_HANGAR_STANDARD,              TRACK_BIT_Y,                           DIAGDIR_SE,                APT_DEPOT_SE),
	AirportTileTable(ATT_INFRASTRUCTURE_WITH_CATCH,    ATTG_WITH_CATCH_BUILDING_FLAT,         DIAGDIR_NE,                APT_LOW_BUILDING_FENCE_NW),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_Y | TRACK_BIT_LOWER,         APRON_HELIPAD,             APT_HELIPAD_3_FENCE_NW),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_CROSS,                       APRON_HELIPAD,             APT_HELIPAD_3_FENCE_NW_SW),

	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS,                                                  APT_APRON_FENCE_NE_SE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_X,                                                      APT_APRON_FENCE_SE),
	AirportTileTable(ATT_SIMPLE_TRACK,                 TRACK_BIT_CROSS | TRACK_BIT_LEFT,                                 APT_APRON_FENCE_SE),
	AirportTileTable(ATT_APRON_HELIPAD,                TRACK_BIT_X | TRACK_BIT_UPPER,         APRON_HELIPAD,             APT_HELIPAD_3_FENCE_SE_SW),
};

/** Tiles for Oil rig */
static const std::initializer_list<AirportTileTable> _description_oilrig = {
	AirportTileTable(ATT_APRON_BUILTIN_HELIPORT,       TRACK_BIT_CROSS,                       APRON_BUILTIN_HELIPORT,    APT_DEPOT_SE /* TODO */),
};

static const std::initializer_list<AirportTileLayout> _layouts_country = {
	{ _description_country, 4, 3 },
};

static const std::initializer_list<AirportTileLayout> _layouts_city = {
	{ _description_city, 6, 6 },
};

static const std::initializer_list<AirportTileLayout> _layouts_heliport = {
	{ _description_heliport, 1, 1 },
};

static const std::initializer_list<AirportTileLayout> _layouts_metropolitan = {
	{ _description_metropolitan, 6, 6 },
};

static const std::initializer_list<AirportTileLayout> _layouts_international = {
	{ _description_international, 7, 7 },
};

static const std::initializer_list<AirportTileLayout> _layouts_commuter = {
	{ _description_commuter, 5, 4 },
};

static const std::initializer_list<AirportTileLayout> _layouts_helidepot = {
	{ _description_helidepot, 2, 2 },
};

static const std::initializer_list<AirportTileLayout> _layouts_intercontinental = {
	{ _description_intercontinental, 9, 11 },
};

static const std::initializer_list<AirportTileLayout> _layouts_helistation = {
	{ _description_helistation, 4, 2 },
};

static const std::initializer_list<AirportTileLayout> _layouts_oilrig = {
	{ _description_oilrig, 1, 1 },
};

/** General AirportSpec definition. */
#define AS(dname, airtype, min_year, max_year, num_runways, num_aprons, num_helipads, num_heliports, min_runway_length, ttdpatch_type, class_id, name, preview, enabled, has_hangar, has_heliport) \
{{class_id, 0}, _layouts_##dname, airtype, num_runways, num_aprons, num_helipads, num_heliports, min_runway_length, min_year, max_year, name, ttdpatch_type, preview, enabled, has_hangar, has_heliport, GRFFileProps(AT_INVALID) }

/* The helidepot and helistation have ATP_TTDP_SMALL because they are at ground level */
const AirportSpec _origin_airport_specs[] = {
	AS(country,          AIRTYPE_GRAVEL,      0,                   1959, 1, 2, 0, 0, 4, ATP_TTDP_SMALL,    APC_SMALL,    STR_AIRPORT_SMALL,            SPR_AIRTYPE_PREVIEW_SMALL,             true,   true,   false),
	AS(city,             AIRTYPE_ASPHALT,  1955, CalendarTime::MAX_YEAR, 1, 3, 0, 0, 6, ATP_TTDP_LARGE,    APC_LARGE,    STR_AIRPORT_CITY,             SPR_AIRTYPE_PREVIEW_LARGE,             true,   true,   false),
	AS(heliport,         AIRTYPE_ASPHALT,  1963, CalendarTime::MAX_YEAR, 0, 0, 0, 1, 0, ATP_TTDP_HELIPORT, APC_HELIPORT, STR_AIRPORT_HELIPORT,         SPR_AIRTYPE_PREVIEW_HELIPORT,          true,   false,  true),
	AS(metropolitan,     AIRTYPE_ASPHALT,  1980, CalendarTime::MAX_YEAR, 2, 3, 0, 0, 6, ATP_TTDP_LARGE,    APC_LARGE,    STR_AIRPORT_METRO,            SPR_AIRTYPE_PREVIEW_METROPOLITAN,      true,   true,   false),
	AS(international,    AIRTYPE_ASPHALT,  1990, CalendarTime::MAX_YEAR, 2, 6, 2, 0, 7, ATP_TTDP_LARGE,    APC_HUB,      STR_AIRPORT_INTERNATIONAL,    SPR_AIRTYPE_PREVIEW_INTERNATIONAL,     true,   true,   false),
	AS(commuter,         AIRTYPE_ASPHALT,  1983, CalendarTime::MAX_YEAR, 1, 3, 2, 0, 5, ATP_TTDP_SMALL,    APC_SMALL,    STR_AIRPORT_COMMUTER,         SPR_AIRTYPE_PREVIEW_COMMUTER,          true,   true,   false),
	AS(helidepot,        AIRTYPE_ASPHALT,  1976, CalendarTime::MAX_YEAR, 0, 0, 1, 0, 0, ATP_TTDP_SMALL,    APC_HELIPORT, STR_AIRPORT_HELIDEPOT,        SPR_AIRTYPE_PREVIEW_HELIDEPOT,         true,   true,   true),
	AS(intercontinental, AIRTYPE_ASPHALT,  2002, CalendarTime::MAX_YEAR, 4, 8, 2, 0, 8, ATP_TTDP_LARGE,    APC_HUB,      STR_AIRPORT_INTERCONTINENTAL, SPR_AIRTYPE_PREVIEW_INTERCONTINENTAL,  true,   true,   false),
	AS(helistation,      AIRTYPE_ASPHALT,  1980, CalendarTime::MAX_YEAR, 0, 0, 3, 0, 0, ATP_TTDP_SMALL,    APC_HELIPORT, STR_AIRPORT_HELISTATION,      SPR_AIRTYPE_PREVIEW_HELISTATION,       true,   true,   true),
	AS(oilrig,           AIRTYPE_WATER,       0, CalendarTime::MAX_YEAR, 0, 0, 0, 1, 0, ATP_TTDP_OILRIG,   APC_HELIPORT, STR_NULL,                     0,                                     false,  false,  false),
};

static_assert(NEW_AIRPORT_OFFSET == lengthof(_origin_airport_specs));

#undef AS

#endif /* AIRPORT_DEFAULTS_H */
