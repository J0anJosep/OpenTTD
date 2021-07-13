/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airporttile_ids.h Enum of the default airport tiles. */

#ifndef AIRPORTTILE_IDS_H
#define AIRPORTTILE_IDS_H

enum AirportTiles : byte {
	APT_WITH_CATCH_BUILDING_1 = 0,
	APT_WITH_CATCH_BUILDING_2,
	APT_WITH_CATCH_BUILDING_3,
	APT_WITH_CATCH_BUILDING_FLAT,
	APT_WITH_CATCH_BUILDING_TERMINAL,
	APT_NO_CATCH_FLAG,
	APT_NO_CATCH_TRANSMITTER,
	APT_NO_CATCH_TOWER,
	APT_NO_CATCH_RADAR,
	APT_NO_CATCH_PIER,
	APT_NO_CATCH_EMPTY,
	APT_NEWGRF,
};

#endif /* AIRPORTTILE_IDS_H */
