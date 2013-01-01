/* $Id: zoning.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file zoning.cpp Functions related with the catchment area layer. */

#include "stdafx.h"
#include "viewport_func.h"
#include "window_func.h"
#include "filters/filter_active.h"
#include "station_base.h"

FilterActive _ca_controller;
bool *_ca_layer = NULL;
bool _stations_modified;

/**
 * Reset the _ca_controller
 */
void ResetCatchmentAreaLayer()
{
	free(_ca_layer);
	extern uint _map_size;
	_ca_layer = CallocT<bool>(_map_size);
	_ca_controller.Reset();
	_stations_modified = false;
}

/**
 * Set tiles of \a check_area to a desired state \a setting_tiles_to if the tile is caught by station \a st
 * @param st station to be considered
 * @param check_area the area we want to update (only tiles in this area will be changed)
 * @param setting_tiles_to the value we want the layer to be updated to
 */
void SetZoning(const Station *st, const TileArea check_area, bool setting_tiles_to)
{
	const bool *mask = st->GetStationCatchmentFootprint(check_area);
	MASKED_TILE_AREA_LOOP(tile, check_area, mask) {
		_ca_layer[tile] = setting_tiles_to;
	}
	delete [] mask;
}

void UpdateCatchmentAreaLayer()
{
	extern uint _map_size;
	MemSetT<bool>(_ca_layer, 0, _map_size);

	Station *st;
	for (bool setting_tiles_to = true;; setting_tiles_to = false) {
		//for each station, update
		for (uint i = _ca_controller.lists[LT_STATIONS].Length(); i--;) {
			if (_ca_controller.lists[LT_STATIONS][i].IsActive() == setting_tiles_to) {
				st = Station::Get(_ca_controller.lists[LT_STATIONS][i].GetElement());
				SetZoning(st, st->GetStationCatchmentArea(), setting_tiles_to);
			}
		}
		if (!setting_tiles_to) break;
	}

	MarkWholeScreenDirty();
}

/**
 * Update catchment area layer for given area
 * @param check_area tile area to update
 */
void UpdateCALayer(const TileArea check_area)
{
	//get the list of stations involved with the given area
	Station *st;
	SmallVector<Station*, 32> involved_stations_state[2]; // [1]-> positive, [0]->negative
	for (uint i = _ca_controller.lists[LT_STATIONS].Length(); i--;) {
		if (!_ca_controller.lists[LT_STATIONS][i].IsActive()) continue;
		st = Station::Get(_ca_controller.lists[LT_STATIONS][i].GetElement());
		if (st->GetStationCatchmentArea().Intersects(check_area)) {
			*involved_stations_state[(uint)_ca_controller.lists[LT_STATIONS][i].IsPositive()].Append() = st;
		}
	}
	//reset area to check
	TILE_AREA_LOOP(tile, check_area) {
		_ca_layer[tile] = 0;
	}

	for (uint setting_tiles_to = 2; setting_tiles_to--;) {
		//for each station, update
		for (uint i = involved_stations_state[setting_tiles_to].Length(); i--;) {
			SetZoning(involved_stations_state[setting_tiles_to][i], check_area, (bool)setting_tiles_to);
		}
	}

	TILE_AREA_LOOP(tile, check_area) MarkTileDirtyByTile(tile);
}

/**
 * Update the _ca_layer tiles affected by a station
 * @param station station that has the affected area
 */
void UpdateCALayer(StationID station)
{
	UpdateCALayer(Station::Get(station)->GetStationCatchmentArea());
}

/**
 * Update the CA layer, after touching an option (and always redraw whole screen)
 * @param list_type of the element touched
 * @param id of the element clicked (station id basically)
 * @note if list type is:
 *		_properties:	just redraw the screen
 * 		_stations:	check if stations list is modified... and update layer for touched station
 * 		_end:		catchment has been reseted
 * 		_other:		reset layer and stations list; recalculate stations depending on the state of filter
 */
void UpdateCALayer(ListType list_type, const uint id)
{
	extern uint _map_size;
	switch (list_type) {
		case LT_CATCHMENT_AREA_PROPERTIES:
			break;
		case LT_STATIONS:
			if (_stations_modified && _ca_controller.lists[LT_STATIONS].Length() == 0) {
				_stations_modified = false;
			} else {
				if (_stations_modified == false) {
					_stations_modified = true;
					_ca_controller.lists[LT_TOWNS].Clear();
					_ca_controller.lists[LT_COMPANIES].Clear();
					_ca_controller.lists[LT_CARGO].Clear();
					_ca_controller.lists[LT_CARGO_ACCEPTANCE].Clear();
					_ca_controller.lists[LT_STATION_FACILITIES].Clear();
				}
			}
			UpdateCALayer(id);
			break;
		case LT_END:
			_stations_modified = false;
		case LT_TOWNS:
		case LT_COMPANIES:
		case LT_CARGO:
		case LT_CARGO_ACCEPTANCE:
		case LT_STATION_FACILITIES: {
			_ca_controller.lists[LT_STATIONS].Clear();
			Station *st;
			FOR_ALL_STATIONS(st) {
				if (_ca_controller.FilterTestCA(st)) {
					*_ca_controller.lists[LT_STATIONS].Append() = FilterElement(st->index, IS_POSITIVE);
				}
			}

			MemSetT<bool>(_ca_layer, 0, _map_size);

			for (uint i = _ca_controller.lists[LT_STATIONS].Length(); i--;) {
				Station *st = Station::Get(_ca_controller.lists[LT_STATIONS][i].GetElement());
				SetZoning(st, st->GetStationCatchmentArea(), true);
			}

			InvalidateWindowClassesData(WC_CATCHMENT_AREA_WINDOW, 2);
			break;
		}
		default: NOT_REACHED();
	}
	MarkWholeScreenDirty();
}

/**
 * Handle CTRL+Click on a station: rotate the status of the station on the CA Layer
 * @param station id of the station clicked
 */
void RotateStationOnCALayer(StationID station)
{
	FilterElement *object = _ca_controller.lists[LT_STATIONS].Find(FilterElement(station));
	if (object == _ca_controller.lists[LT_STATIONS].End()) {
		*_ca_controller.lists[LT_STATIONS].Append() = FilterElement(station);
		object = _ca_controller.lists[LT_STATIONS].Find(FilterElement(station));
	}
	object->Rotate();
	if (!object->IsActive()) _ca_controller.lists[LT_STATIONS].Erase(object);
	UpdateCALayer(LT_STATIONS, station);
	InvalidateWindowClassesData(WC_CATCHMENT_AREA_WINDOW, 2);
}

/**
 * Check if a station should be automatically added/removed from catchment area
 * due to a change of some internal stuff (accepted cargoes, produced cargoes...)
 * @param station to check
 */
void CheckSingleStationCA(StationID station)
{
	Station *st = Station::Get(station);
	/* If the station list has been manually modified, nothing changes */
	if (_stations_modified) return;

	/* Found the station in the active filter */
	FilterElement *object = _ca_controller.lists[LT_STATIONS].Find(FilterElement(station));

	/* Check if station should be active and positive */
	bool should_be_positive = _ca_controller.FilterTestCA(st);

	if (should_be_positive) { /* station must be active and state must be positive */
		if (object != _ca_controller.lists[LT_STATIONS].End()) {
			if (object->IsPositive()) return;
			object->SetState(IS_POSITIVE);
		} else {
			*_ca_controller.lists[LT_STATIONS].Append() = FilterElement(station, IS_POSITIVE);
		}
	} else { /* should be erased if it is in the active filter */
		if (object != _ca_controller.lists[LT_STATIONS].End()) _ca_controller.lists[LT_STATIONS].Erase(object);
	}

	UpdateCALayer(st->index);
	_ca_controller.Sort(LT_STATIONS);
	SetWindowDirty(WC_CATCHMENT_AREA_WINDOW, 0);
}