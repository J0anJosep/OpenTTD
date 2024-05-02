/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_gui.h Various declarations for airports */

#ifndef AIRPORT_GUI_H
#define AIRPORT_GUI_H

#include "air_type.h"
#include "dropdown_type.h"

struct Window *ShowBuildAirToolbar(AirType airtype);
DropDownList GetAirTypeDropDownList(bool for_replacement = false, bool all_option = false);

void InitializeAirportGui();

#endif /* AIRPORT_GUI_H */
