/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_widget.h Types related to the airport widgets. */

#ifndef WIDGETS_AIRPORT_WIDGET_H
#define WIDGETS_AIRPORT_WIDGET_H

/** Widgets of the #BuildAirToolbarWindow class. */
enum AirportToolbarWidgets : WidgetID {
	WID_AT_CAPTION,                   ///< Caption of the window.
	WID_AT_BUILD_TILE,                ///< Add tiles to an airport.
	WID_AT_INFRASTRUCTURE_CATCH,      ///< Build a piece of airport infrastructure with catchment.
	WID_AT_INFRASTRUCTURE_NO_CATCH,   ///< Build a piece of airport infrastructure without catchment.
	WID_AT_TRACKS,                    ///< Add new tracks to a tracktile.
	WID_AT_RUNWAY_LANDING,            ///< Define a new runway for an airport allowing landing.
	WID_AT_RUNWAY_NO_LANDING,         ///< Define a new runway for an airport not allowing landing.
	WID_AT_APRON,                     ///< Build a new apron.
	WID_AT_HELIPAD,                   ///< Build a new helipad.
	WID_AT_HELIPORT,                  ///< Build a new heliport (same as helipad but taller).
	WID_AT_HANGAR_STANDARD,           ///< Build a new standard hangar.
	WID_AT_HANGAR_EXTENDED,           ///< Build a new extended hangar.
	WID_AT_CONVERT,                   ///< Change the airtype of this airport.
	WID_AT_REMOVE,                    ///< Remove widget.
	WID_AT_AIRPORT,                   ///< Build a predefined airport.
	WID_AT_DEMOLISH,                  ///< Demolish button.

	WID_AT_REMOVE_FIRST = WID_AT_BUILD_TILE,   ///< First an last widgets that work combined with remove widget.
	WID_AT_REMOVE_LAST = WID_AT_HANGAR_EXTENDED,

	INVALID_WID_AT = -1,
};

/** Widgets of the #BuildAirportWindow class. */
enum AirportPickerWidgets : WidgetID {
	WID_AP_CLASS_DROPDOWN,    ///< Dropdown of airport classes.
	WID_AP_AIRPORT_LIST,      ///< List of airports.
	WID_AP_SCROLLBAR,         ///< Scrollbar of the list.
	WID_AP_LAYOUT_NUM,        ///< Current number of the layout.
	WID_AP_LAYOUT_DECREASE,   ///< Decrease the layout number.
	WID_AP_LAYOUT_INCREASE,   ///< Increase the layout number.
	WID_AP_ROTATION,          ///< Selected angle of rotation of the layout.
	WID_AP_ROTATION_DECREASE, ///< Decrease the angle of rotation.
	WID_AP_ROTATION_INCREASE, ///< Increase the angle of rotation.
	WID_AP_AIRPORT_SPRITE,    ///< A visual display of the airport currently selected.
	WID_AP_EXTRA_TEXT,        ///< Additional text about the airport.
	WID_AP_COVERAGE_LABEL,    ///< Label if you want to see the coverage.
	WID_AP_BTN_DONTHILIGHT,   ///< Don't show the coverage button.
	WID_AP_BTN_DOHILIGHT,     ///< Show the coverage button.
	WID_AP_ACCEPTANCE,        ///< Acceptance info.
};

/** Widgets of the #BuildHangarWindow class. */
enum BuildHangarHeliportWidgets : WidgetID {
	WID_BHW_CAPTION,   ///< Caption of the window.
	WID_BHW_NE,        ///< Hangar/heliport with NE entry.
	WID_BHW_SE,        ///< Hangar/heliport with SE entry.
	WID_BHW_SW,        ///< Hangar/heliport with SW entry.
	WID_BHW_NW,        ///< Hangar/heliport with NW entry.
};

/** Widgets of the #BuildAirportInfraNoCatchWindow class. */
enum BuildAirportInfrastructureNoCatchmentWidgets : WidgetID {
	WID_BAINC_CAPTION,   ///< Caption of the window.
	WID_BAINC_FLAG,
	WID_BAINC_TRANSMITTER,
	WID_BAINC_TOWER,
	WID_BAINC_RADAR,
	WID_BAINC_PIER,
	WID_BAINC_EMPTY,
	WID_BAINC_ROTATION,          ///< Selected angle of rotation of the infrastructure tile.
	WID_BAINC_ROTATION_DECREASE, ///< Decrease the angle of rotation.
	WID_BAINC_ROTATION_INCREASE, ///< Increase the angle of rotation.
};
DECLARE_ENUM_AS_ADDABLE(BuildAirportInfrastructureNoCatchmentWidgets)

/** Widgets of the #BuildAirportInfraWithCatchWindow class. */
enum BuildAirportInfrastructureWithCatchmentWidgets : WidgetID {
	WID_BAIWC_CAPTION,   ///< Caption of the window.
	WID_BAIWC_BUILDING_1,
	WID_BAIWC_BUILDING_2,
	WID_BAIWC_BUILDING_3,
	WID_BAIWC_BUILDING_FLAT,
	WID_BAIWC_BUILDING_TERMINAL,
	WID_BAIWC_ROTATION,          ///< Selected angle of rotation of the infrastructure tile.
	WID_BAIWC_ROTATION_DECREASE, ///< Decrease the angle of rotation.
	WID_BAIWC_ROTATION_INCREASE, ///< Increase the angle of rotation.
};

#endif /* WIDGETS_AIRPORT_WIDGET_H */
