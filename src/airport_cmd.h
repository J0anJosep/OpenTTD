/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_cmd.h Command definitions related to airports. */

#ifndef AIRPORT_CMD_H
#define AIRPORT_CMD_H

#include "command_type.h"
#include "station_type.h"

CommandCost CmdBuildAirport(DoCommandFlag flags, TileIndex tile, uint8_t airport_type, uint8_t layout, StationID station_to_join, bool allow_adjacent);
CommandCost CmdOpenCloseAirport(DoCommandFlag flags, StationID station_id);

DEF_CMD_TRAIT(CMD_BUILD_AIRPORT,            CmdBuildAirport,          CMD_AUTO | CMD_NO_WATER, CMDT_LANDSCAPE_CONSTRUCTION)
DEF_CMD_TRAIT(CMD_OPEN_CLOSE_AIRPORT,       CmdOpenCloseAirport,      0,                       CMDT_ROUTE_MANAGEMENT)

CommandCallback CcBuildAirport;

#endif /* AIRPORT_CMD_H */
