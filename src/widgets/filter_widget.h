/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filter_widget.h Types related to the filter widgets. */

#ifndef WIDGETS_FILTER_WIDGET_H
#define WIDGETS_FILTER_WIDGET_H

/** Widgets for the #VehicleGroupFilterWindow,
 * 			#CatchmentAreaWindow,
 * 			#IndustryFilterWindow
 * 			#TownWindowFilter,
 * 			#StationFilterWindow classes. */
enum FilterWindowWidgets {
	FW_WIDGET_CAPTION,

	FW_WIDGET_TOWNS,                     ///< Town tab
	FW_WIDGET_COMPANIES,                 ///< Owners tab
	FW_WIDGET_CARGO,                     ///< Cargotypes tab (for catchment area window, production)
	FW_WIDGET_CARGO_ACCEPTANCE,          ///< Cargotypes acceptance tab
	FW_WIDGET_STATION_FACILITIES,        ///< Airport, dock, bus station, truck station or rail station
	FW_WIDGET_STATIONS,                  ///< Stations tab
	FW_WIDGET_ORDERS,                    ///< Orders/timetable tab
	FW_WIDGET_VEHICLE_GROUP_PROPERTIES,  ///< Vehicle properties
	FW_WIDGET_CATCHMENT_AREA_PROPERTIES, ///< Catchment area properties
	FW_WIDGET_TOWN_PROPERTIES,           ///< Town properties
	FW_WIDGET_STATE,                     ///< Selected items for filtering tab
	FW_WIDGET_RESET,                     ///< Reset tab

	FW_LIST,                             ///< The list of elements
	FW_SCROLLBAR,                        ///< Scrollbar for the list

};

#endif /* WIDGETS_FILTER_WIDGET_H */
