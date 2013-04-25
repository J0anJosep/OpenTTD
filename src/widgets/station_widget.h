/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file station_widget.h Types related to the station widgets. */

#ifndef WIDGETS_STATION_WIDGET_H
#define WIDGETS_STATION_WIDGET_H

/** Widgets of the #StationViewWindow class. */
enum StationViewWidgets {
	WID_SV_CAPTION,            ///< Caption of the window.
	WID_SV_SORT_ORDER,         ///< 'Sort order' button
	WID_SV_SORT_BY,            ///< 'Sort by' button
	WID_SV_GROUP,              ///< label for "group by"
	WID_SV_GROUP_BY,           ///< 'Group by' button
	WID_SV_WAITING,            ///< List of waiting cargo.
	WID_SV_SCROLLBAR,          ///< Scrollbar.
	WID_SV_ACCEPT_RATING_LIST, ///< List of accepted cargoes / rating of cargoes.
	WID_SV_LOCATION,           ///< 'Location' button.
	WID_SV_ACCEPTS_RATINGS,    ///< 'Accepts' / 'Ratings' button.
	WID_SV_RENAME,             ///< 'Rename' button.
	WID_SV_CLOSE_AIRPORT,      ///< 'Close airport' button.
	WID_SV_TRAINS,             ///< List of scheduled trains button.
	WID_SV_ROADVEHS,           ///< List of scheduled road vehs button.
	WID_SV_SHIPS,              ///< List of scheduled ships button.
	WID_SV_PLANES,             ///< List of scheduled planes button.
};

/** Widgets of the #CompanyStationsWindow class. */
enum StationListWidgets {
	/* Name starts with ST instead of S, because of collision with SaveLoadWidgets */
	WID_STL_CAPTION,        ///< Caption of the window.
	WID_STL_FILTER,         ///< Filter icon button.
	WID_STL_LIST,           ///< The main panel, list of stations.
	WID_STL_SCROLLBAR,      ///< Scrollbar next to the main panel.

	WID_STL_SORTBY,         ///< 'Sort by' button - reverse sort direction.
	WID_STL_SORTDROPBTN,    ///< Dropdown button.
};

/** Widgets of the #SelectStationWindow class. */
enum JoinStationWidgets {
	WID_JS_CAPTION,   // Caption of the window.
	WID_JS_PANEL,     // Main panel.
	WID_JS_SCROLLBAR, // Scrollbar of the panel.
};

#endif /* WIDGETS_STATION_WIDGET_H */
