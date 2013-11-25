/* $Id: zoning.h $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file zoning.h Functions related with the catchment area layer. */

#ifndef ZONING_H
#define ZONING_H

#include "station_type.h"
#include "tilearea_type.h"
#include "filters/filter_enums.h"
#include "filters/filter_active.h"

extern FilterActive _ca_controller;
extern BitMap *_ca_layer;
extern bool _stations_modified;

void ResetCatchmentAreaLayer();
void UpdateCatchmentAreaLayer();

void UpdateCALayer(const TileArea check_area);
void UpdateCALayer(ListType list_type, const uint id);
void UpdateCALayer(StationID station);

void RotateStationOnCALayer(StationID station);
void CheckSingleStationCA(StationID station);

#endif /* ZONING_H */