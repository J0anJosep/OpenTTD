/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filter_window_gui.h Functions related to the filter windows. */

#ifndef FILTER_WINDOW_GUI_H
#define FILTER_WINDOW_GUI_H

#include "../window_type.h"
#include "../station_type.h"
#include "../company_type.h"
#include "../town_type.h"
#include "filter_active.h"

void ShowFilterWindow(Window *parent_window, WindowNumber new_window_number);

const FilterActive *FindFilterData(WindowClass window_class, WindowNumber window_number);

void StationUpdateFilters(const StationID station);
void CompanyUpdateFilters(const CompanyID company);
void TownUpdateFilters(const TownID town);

#endif /* FILTER_WINDOW_GUI_H */
