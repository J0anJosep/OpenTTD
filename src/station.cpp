/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file station.cpp Implementation of the station base class. */

#include "stdafx.h"
#include "company_func.h"
#include "company_base.h"
#include "roadveh.h"
#include "viewport_func.h"
#include "viewport_kdtree.h"
#include "date_func.h"
#include "command_func.h"
#include "news_func.h"
#include "aircraft.h"
#include "vehiclelist.h"
#include "core/pool_func.hpp"
#include "station_base.h"
#include "station_kdtree.h"
#include "roadstop_base.h"
#include "dock_base.h"
#include "industry.h"
#include "core/random_func.hpp"
#include "linkgraph/linkgraph.h"
#include "linkgraph/linkgraphschedule.h"
#include "filters/filter_window_gui.h"
#include "depot_base.h"

#include "table/strings.h"

#include "safeguards.h"

/** The pool of stations. */
StationPool _station_pool("Station");
INSTANTIATE_POOL_METHODS(Station)


StationKdtree _station_kdtree(Kdtree_StationXYFunc);

void RebuildStationKdtree()
{
	std::vector<StationID> stids;
	BaseStation *st;
	FOR_ALL_STATIONS(st) {
		stids.push_back(st->index);
	}
	_station_kdtree.Build(stids.begin(), stids.end());
}


BaseStation::~BaseStation()
{
	free(this->name);
	free(this->speclist);

	if (CleaningPool()) return;

	DeleteWindowById(WC_TRAINS_LIST,   VehicleListIdentifier(VL_STATION_LIST, VEH_TRAIN,    this->owner, this->index).Pack());
	DeleteWindowById(WC_ROADVEH_LIST,  VehicleListIdentifier(VL_STATION_LIST, VEH_ROAD,     this->owner, this->index).Pack());
	DeleteWindowById(WC_SHIPS_LIST,    VehicleListIdentifier(VL_STATION_LIST, VEH_SHIP,     this->owner, this->index).Pack());
	DeleteWindowById(WC_AIRCRAFT_LIST, VehicleListIdentifier(VL_STATION_LIST, VEH_AIRCRAFT, this->owner, this->index).Pack());

	this->sign.MarkDirty();
}

Station::Station(TileIndex tile) :
	SpecializedStation<Station, false>(tile),
	footprint(NULL),
	bus_station(INVALID_TILE, 0, 0),
	truck_station(INVALID_TILE, 0, 0),
	dock_station(INVALID_TILE, 0, 0),
	indtype(IT_INVALID),
	time_since_load(255),
	time_since_unload(255),
	last_vehicle_type(VEH_INVALID)
{
	/* this->random_bits is set in Station::AddFacility() */
}

/**
 * Clean up a station by clearing vehicle orders, invalidating windows and
 * removing link stats.
 * Aircraft-Hangar orders need special treatment here, as the hangars are
 * actually part of a station (tiletype is STATION), but the order type
 * is OT_GOTO_DEPOT.
 */
Station::~Station()
{
	delete [] this->footprint;

	if (CleaningPool()) {
		for (CargoID c = 0; c < NUM_CARGO; c++) {
			this->goods[c].cargo.OnCleanPool();
		}
		return;
	}

	while (!this->loading_vehicles.empty()) {
		this->loading_vehicles.front()->LeaveStation();
	}

	Aircraft *a;
	FOR_ALL_AIRCRAFT(a) {
		if (!a->IsNormalAircraft()) continue;
		if (a->targetairport == this->index) a->targetairport = INVALID_STATION;
	}

	for (CargoID c = 0; c < NUM_CARGO; ++c) {
		LinkGraph *lg = LinkGraph::GetIfValid(this->goods[c].link_graph);
		if (lg == NULL) continue;

		for (NodeID node = 0; node < lg->Size(); ++node) {
			Station *st = Station::Get((*lg)[node].Station());
			st->goods[c].flows.erase(this->index);
			if ((*lg)[node][this->goods[c].node].LastUpdate() != INVALID_DATE) {
				st->goods[c].flows.DeleteFlows(this->index);
				RerouteCargo(st, c, this->index, st->index);
			}
		}
		lg->RemoveNode(this->goods[c].node);
		if (lg->Size() == 0) {
			LinkGraphSchedule::instance.Unqueue(lg);
			delete lg;
		}
	}

	Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		/* Forget about this station if this station is removed */
		if (v->last_station_visited == this->index) {
			v->last_station_visited = INVALID_STATION;
		}
		if (v->last_loading_station == this->index) {
			v->last_loading_station = INVALID_STATION;
		}
	}

	/* Clear the persistent storage. */
	delete this->airport.psa;

	if (this->owner == OWNER_NONE) {
		/* Invalidate all in case of oil rigs. */
		InvalidateWindowClassesData(WC_STATION_LIST, 0);
	} else {
		InvalidateWindowData(WC_STATION_LIST, this->owner, 0);
	}

	DeleteWindowById(WC_STATION_VIEW, index);

	/* Now delete all orders that go to the station */
	RemoveOrderFromAllVehicles(OT_GOTO_STATION, this->index);

	/* Remove all news items */
	DeleteStationNews(this->index);

	for (CargoID c = 0; c < NUM_CARGO; c++) {
		this->goods[c].cargo.Truncate();
	}

	CargoPacket::InvalidateAllFrom(this->index);

	_station_kdtree.Remove(this->index);
	_viewport_sign_kdtree.Remove(ViewportSignKdtreeItem::MakeStation(this->index));
}


/**
 * Invalidating of the JoinStation window has to be done
 * after removing item from the pool.
 * @param index index of deleted item
 */
void BaseStation::PostDestructor(size_t index)
{
	StationUpdateFilters((StationID)index);
	InvalidateWindowData(WC_SELECT_STATION, 0, 0);
}

/**
 * Get the primary road stop (the first road stop) that the given vehicle can load/unload.
 * @param v the vehicle to get the first road stop for
 * @return the first roadstop that this vehicle can load at
 */
RoadStop *Station::GetPrimaryRoadStop(const RoadVehicle *v) const
{
	RoadStop *rs = this->GetPrimaryRoadStop(v->IsBus() ? ROADSTOP_BUS : ROADSTOP_TRUCK);

	for (; rs != NULL; rs = rs->next) {
		/* The vehicle cannot go to this roadstop (different roadtype) */
		if ((GetRoadTypes(rs->xy) & v->compatible_roadtypes) == ROADTYPES_NONE) continue;
		/* The vehicle is articulated and can therefore not go to a standard road stop. */
		if (IsStandardRoadStopTile(rs->xy) && v->HasArticulatedPart()) continue;

		/* The vehicle can actually go to this road stop. So, return it! */
		break;
	}

	return rs;
}

/**
 * Called when new facility is built on the station. If it is the first facility
 * it initializes also 'xy' and 'random_bits' members
 */
void Station::AddFacility(StationFacility new_facility_bit, TileIndex facil_xy)
{
	if (this->facilities == FACIL_NONE) {
		this->xy = facil_xy;
		this->random_bits = Random();
	}
	this->facilities |= new_facility_bit;
	this->owner = _current_company;
	this->build_date = _date;
}

/**
 * Marks the tiles of the station as dirty.
 *
 * @ingroup dirty
 */
void Station::MarkTilesDirty(bool cargo_change) const
{
	TileIndex tile = this->train_station.tile;
	int w, h;

	if (tile == INVALID_TILE) return;

	/* cargo_change is set if we're refreshing the tiles due to cargo moving
	 * around. */
	if (cargo_change) {
		/* Don't waste time updating if there are no custom station graphics
		 * that might change. Even if there are custom graphics, they might
		 * not change. Unfortunately we have no way of telling. */
		if (this->num_specs == 0) return;
	}

	for (h = 0; h < train_station.h; h++) {
		for (w = 0; w < train_station.w; w++) {
			if (this->TileBelongsToRailStation(tile)) {
				MarkTileDirtyByTile(tile);
			}
			tile += TileDiffXY(1, 0);
		}
		tile += TileDiffXY(-w, 1);
	}
}

/**
 * Determine the catchment radius of a station of the given type
 * @param type the type of station: bus/truck stop, train station, airport...
 * @return The radius
 */
/* static */ CatchmentArea Station::GetCatchmentRadius(StationType type)
{
	if (_settings_game.station.modified_catchment) {
		switch (type) {
			case STATION_RAIL:
				return CA_TRAIN;
			case STATION_TRUCK:
				return CA_TRUCK;
			case STATION_BUS:
				return CA_BUS;
			case STATION_DOCK:
				return CA_DOCK;
			case STATION_OILRIG:
				return CA_OILRIG;
			case STATION_AIRPORT:
				return MAX_CATCHMENT;
			case STATION_BUOY:
			case STATION_WAYPOINT:
			default:
				return CA_NONE;
		}
	} else {
		switch (type) {
			case STATION_RAIL:
			case STATION_TRUCK:
			case STATION_BUS:
			case STATION_DOCK:
			case STATION_AIRPORT:
			case STATION_OILRIG:
				return CA_UNMODIFIED;
			case STATION_BUOY:
			case STATION_WAYPOINT:
			default:
				return CA_NONE;
		}
	}
}

/**
 * Determines the catchment radius of the station
 * @return The radius
 */
uint Station::GetCatchmentRadius() const
{
	uint ret = CA_NONE;

	if (_settings_game.station.modified_catchment) {
		if (this->bus_stops          != NULL)         ret = max<uint>(ret, CA_BUS);
		if (this->truck_stops        != NULL)         ret = max<uint>(ret, CA_TRUCK);
		if (this->train_station.tile != INVALID_TILE) ret = max<uint>(ret, CA_TRAIN);
		if (this->docks              != NULL)         ret = max<uint>(ret, CA_DOCK);
		if (this->airport.tile       != INVALID_TILE) ret = max<uint>(ret, this->airport.GetSpec()->catchment);
	} else {
		if (this->bus_stops != NULL || this->truck_stops != NULL || this->train_station.tile != INVALID_TILE || this->docks != NULL || this->airport.tile != INVALID_TILE) {
			ret = CA_UNMODIFIED;
		}
	}

	return ret;
}

/**
 * Determines catchment rectangle of this station
 * @return clamped catchment rectangle
 */
Rect Station::GetCatchmentRect() const
{
	assert(!this->rect.IsEmpty());

	/* Compute acceptance rectangle */
	int catchment_radius = this->GetCatchmentRadius();

	Rect ret = {
		max<int>(this->rect.left   - catchment_radius, 0),
		max<int>(this->rect.top    - catchment_radius, 0),
		min<int>(this->rect.right  + catchment_radius, MapMaxX()),
		min<int>(this->rect.bottom + catchment_radius, MapMaxY())
	};

	return ret;
}

/**
 * Get all the tiles of a station (get only one tile for the dock and one for the airport)
 * @return a vector containing all the bus and truck stops, rail station tiles and the base tiles for dock and airport
  */
SmallVector<TileIndex, 32>  Station::GetStationTiles() const
 {
	SmallVector<TileIndex, 32> list_of_tiles;

	for (RoadStop *road_stop = this->bus_stops; road_stop != NULL; road_stop = road_stop->next) {
		*list_of_tiles.Append() = road_stop->xy;
	}

	for (RoadStop *road_stop = this->truck_stops; road_stop != NULL; road_stop = road_stop->next) {
		*list_of_tiles.Append() = road_stop->xy;
	}

	if (this->airport.tile != INVALID_TILE) *list_of_tiles.Append() = this->airport.tile;

	for (Dock *dock = this->docks; dock != NULL; dock = dock->next) {
		*list_of_tiles.Append() = dock->sloped;
		/* Exclude oilrig ghost dock */
		if (IsTileType(dock->flat, MP_STATION)) *list_of_tiles.Append() = dock->flat;
	}

	TILE_AREA_LOOP(tile, this->train_station) {
		if (IsTileType(tile, MP_STATION) && GetStationType(tile) == STATION_RAIL && this->index == GetStationIndex(tile)) *list_of_tiles.Append() = tile;
	}

	return list_of_tiles;
}

/**
 * Recomputes the cached catchment variables (location and footprint)
 */
void Station::UpdateCatchment()
{
	if (this->industry == NULL || _settings_game.station.serve_neutral_industries) {
		/* Calculate the area this station could catch */
		this->catchment.tile = TileXY(this->rect.left, this->rect.top);
		this->catchment.w = this->rect.right - this->rect.left + 1;
		this->catchment.h = this->rect.bottom - this->rect.top + 1;
		this->catchment.AddRadius(this->GetCatchmentRadius());
	} else {
		this->catchment = this->industry->location;
	}

	/* Calculate the footprint (tiles inside the location area that are really caught) */
	delete [] footprint;
	this->footprint = this->GetStationCatchmentFootprint(this->catchment);
}

/**
 * Calculate the area associated to a single tile
 * @param tile station tile of st we want to find the associated TileArea
 * @return TileArea associated to tile: single tile, or the two tiles of a dock, or the TileArea of an airport
 */
/* static */ TileArea Station::GetStationCatchmentAreaByTile(TileIndex tile)
{
	CatchmentArea rad;
	Station *st = GetByTile(tile);

	if (GetStationType(tile) == STATION_AIRPORT) {
		rad = _settings_game.station.modified_catchment ? (CatchmentArea)st->airport.GetSpec()->catchment : CA_UNMODIFIED;
		return TileArea(st->airport.tile, st->airport.w, st->airport.h, rad);
	}

	rad = Station::GetCatchmentRadius(GetStationType(tile));

	if (GetStationType(tile) == STATION_DOCK) {
		Dock *dock = Dock::GetByTile(tile);
		TileArea tile_area(dock->sloped, dock->flat);
		tile_area.AddRadius(rad);
		return tile_area;
	}

	return TileArea(tile, 1, 1, rad);
}

/**
 * Return a tile area containing all tiles this station could catch
 */
const TileArea Station::GetStationCatchmentArea() const { return this->catchment; }

/**
 * Returns a mask containing all tiles of given ta that this station really can catch
 */
CustomBitMap *Station::GetStationCatchmentFootprint(const TileArea ta) const
{
	if (!_settings_game.station.precise_catchment) return NULL;

	CustomBitMap *new_footprint = NewCustomBitMap(ta.w * ta.h);
	SmallVector<TileIndex, 32> list_of_tiles = this->GetStationTiles();

	for (uint i = list_of_tiles.Length(); i--;) {
		TileArea catchment_area = this->GetStationCatchmentAreaByTile(list_of_tiles[i]);
		catchment_area.IntersectionWith(ta);

		BitMapIndex mask_index;
		TILE_AREA_LOOP(tile, ta) {
			if (catchment_area.Contains(tile)) {
				if (!_settings_game.station.serve_neutral_industries && tile != list_of_tiles[i]) {
					if (this->industry != NULL) {
						if (!IsTileType(tile, MP_INDUSTRY) || GetIndustryIndex(tile) != this->industry->index) {
							++mask_index;
							continue;
						}
					} else {
						if (IsTileType(tile, MP_INDUSTRY) && Industry::GetByTile(tile)->neutral_station != this) {
							++mask_index;
							continue;
						} else if (IsTileType(tile, MP_STATION) && GetStationIndex(tile) != this->index) {
							Station *st = Station::GetByTile(tile);
							if (st != NULL && st->industry != NULL) {
								++mask_index;
								continue;
							}
						}
					}
				}
				SetBit(new_footprint[mask_index.word_index], mask_index.bit_index);
			}
			++mask_index;
		}
	}

	BitMapIndex check_complete;
	TILE_AREA_LOOP(tile, ta) {
		if (!HasBit(new_footprint[check_complete.word_index], check_complete.bit_index)) return new_footprint;
		++check_complete;
	}

	delete [] new_footprint;
	return NULL;
}

/**
 * Return the associated mask of this->catchment
 * @note	mask[i] is true if tile number i of this->catchment is caught by station
 *		mask[i] is false if tile cannot reach the station
 */
const CustomBitMap *Station::GetStationCatchmentFootprint() const { return this->footprint; };


/** Rect and pointer to IndustryVector */
struct RectAndIndustryVector {
	Rect rect;                     ///< The rectangle to search the industries in.
	IndustryList *industries_near; ///< The nearby industries.
};

/**
 * Callback function for Station::RecomputeIndustriesNear()
 * Tests whether tile is an industry and possibly adds
 * the industry to station's industries_near list.
 * @param ind_tile tile to check
 * @param user_data pointer to RectAndIndustryVector
 * @return always false, we want to search all tiles
 */
static bool FindIndustryToDeliver(TileIndex ind_tile, void *user_data)
{
	/* Only process industry tiles */
	if (!IsTileType(ind_tile, MP_INDUSTRY)) return false;

	RectAndIndustryVector *riv = (RectAndIndustryVector *)user_data;
	Industry *ind = Industry::GetByTile(ind_tile);

	/* Don't check further if this industry is already in the list */
	if (riv->industries_near->find(ind) != riv->industries_near->end()) return false;

	/* Only process tiles in the station acceptance rectangle */
	int x = TileX(ind_tile);
	int y = TileY(ind_tile);
	if (x < riv->rect.left || x > riv->rect.right || y < riv->rect.top || y > riv->rect.bottom) return false;

	/* Include only industries that can accept cargo */
	uint cargo_index;
	for (cargo_index = 0; cargo_index < lengthof(ind->accepts_cargo); cargo_index++) {
		if (ind->accepts_cargo[cargo_index] != CT_INVALID) break;
	}
	if (cargo_index >= lengthof(ind->accepts_cargo)) return false;

	riv->industries_near->insert(ind);

	return false;
}

/**
 * Recomputes Station::industries_near, list of industries possibly
 * accepting cargo in station's catchment area
 */
void Station::RecomputeIndustriesNear()
{
	this->industries_near.clear();
	if (this->rect.IsEmpty()) return;

	if (!_settings_game.station.serve_neutral_industries && this->industry != NULL) {
		/* Station is associated with an industry, so we only need to deliver to that industry. */
		this->industries_near.insert(this->industry);
		return;
	}

	const TileArea ta = this->GetStationCatchmentArea();
	const CustomBitMap *mask = this->GetStationCatchmentFootprint();
	Industry *ind;

	MASKED_TILE_AREA_LOOP(tile, ta, mask) {
		/* Only process industry tiles */
		if (!IsTileType(tile, MP_INDUSTRY)) continue;
		ind = Industry::GetByTile(tile);
		/* Don't check further if this industry is already in the list */
		if (this->industries_near.find(ind) != this->industries_near.end()) continue;

		/* Include only industries that can accept cargo */
		uint cargo_index;
		for (cargo_index = 0; cargo_index < lengthof(ind->accepts_cargo); cargo_index++) {
			if (ind->accepts_cargo[cargo_index] != CT_INVALID) break;
		}
		if (cargo_index >= lengthof(ind->accepts_cargo)) continue;

		this->industries_near.insert(ind);
	}
}

/**
 * Find stations near an area and updates Station::industries_near
 * @param ta area modified
 * @param mask tiles modified
 * @note ta will be expanded appropriately
 */
/* static */ void Station::RecomputeIndustriesNearArea(const TileArea ta, const CustomBitMap *mask)
{
	StationList stations;
	FindStationsAroundTiles(ta, mask, &stations);

	for (Station *st : stations) {
		st->RecomputeIndustriesNear();
	}
}

/**
 * Recomputes Station::industries_near for all stations
 */
/* static */ void Station::RecomputeIndustriesNearForAll()
{
	Station *st;
	FOR_ALL_STATIONS(st) st->RecomputeIndustriesNear();
}

/************************************************************************/
/*                     StationRect implementation                       */
/************************************************************************/

StationRect::StationRect()
{
	this->MakeEmpty();
}

void StationRect::MakeEmpty()
{
	this->left = this->top = this->right = this->bottom = 0;
}

/**
 * Determines whether a given point (x, y) is within a certain distance of
 * the station rectangle.
 * @note x and y are in Tile coordinates
 * @param x X coordinate
 * @param y Y coordinate
 * @param distance The maximum distance a point may have (L1 norm)
 * @return true if the point is within distance tiles of the station rectangle
 */
bool StationRect::PtInExtendedRect(int x, int y, int distance) const
{
	return this->left - distance <= x && x <= this->right + distance &&
			this->top - distance <= y && y <= this->bottom + distance;
}

bool StationRect::IsEmpty() const
{
	return this->left == 0 || this->left > this->right || this->top > this->bottom;
}

CommandCost StationRect::BeforeAddTile(TileIndex tile, StationRectMode mode)
{
	int x = TileX(tile);
	int y = TileY(tile);
	if (this->IsEmpty()) {
		/* we are adding the first station tile */
		if (mode != ADD_TEST) {
			this->left = this->right = x;
			this->top = this->bottom = y;
		}
	} else if (!this->PtInExtendedRect(x, y)) {
		/* current rect is not empty and new point is outside this rect
		 * make new spread-out rectangle */
		Rect new_rect = {min(x, this->left), min(y, this->top), max(x, this->right), max(y, this->bottom)};

		/* check new rect dimensions against preset max */
		int w = new_rect.right - new_rect.left + 1;
		int h = new_rect.bottom - new_rect.top + 1;
		if (mode != ADD_FORCE && (w > _settings_game.station.station_spread || h > _settings_game.station.station_spread)) {
			assert(mode != ADD_TRY);
			return_cmd_error(STR_ERROR_STATION_TOO_SPREAD_OUT);
		}

		/* spread-out ok, return true */
		if (mode != ADD_TEST) {
			/* we should update the station rect */
			*this = new_rect;
		}
	} else {
		; // new point is inside the rect, we don't need to do anything
	}
	return CommandCost();
}

CommandCost StationRect::BeforeAddRect(TileIndex tile, int w, int h, StationRectMode mode)
{
	if (mode == ADD_FORCE || (w <= _settings_game.station.station_spread && h <= _settings_game.station.station_spread)) {
		/* Important when the old rect is completely inside the new rect, resp. the old one was empty. */
		CommandCost ret = this->BeforeAddTile(tile, mode);
		if (ret.Succeeded()) ret = this->BeforeAddTile(TILE_ADDXY(tile, w - 1, h - 1), mode);
		return ret;
	}
	return CommandCost();
}

/**
 * Check whether station tiles of the given station id exist in the given rectangle
 * @param st_id    Station ID to look for in the rectangle
 * @param left_a   Minimal tile X edge of the rectangle
 * @param top_a    Minimal tile Y edge of the rectangle
 * @param right_a  Maximal tile X edge of the rectangle (inclusive)
 * @param bottom_a Maximal tile Y edge of the rectangle (inclusive)
 * @return \c true if a station tile with the given \a st_id exists in the rectangle, \c false otherwise
 */
/* static */ bool StationRect::ScanForStationTiles(StationID st_id, int left_a, int top_a, int right_a, int bottom_a)
{
	TileArea ta(TileXY(left_a, top_a), TileXY(right_a, bottom_a));
	TILE_AREA_LOOP(tile, ta) {
		if (IsTileType(tile, MP_STATION) && GetStationIndex(tile) == st_id) return true;
	}

	return false;
}

bool StationRect::AfterRemoveTile(BaseStation *st, TileIndex tile)
{
	int x = TileX(tile);
	int y = TileY(tile);

	/* look if removed tile was on the bounding rect edge
	 * and try to reduce the rect by this edge
	 * do it until we have empty rect or nothing to do */
	for (;;) {
		/* check if removed tile is on rect edge */
		bool left_edge = (x == this->left);
		bool right_edge = (x == this->right);
		bool top_edge = (y == this->top);
		bool bottom_edge = (y == this->bottom);

		/* can we reduce the rect in either direction? */
		bool reduce_x = ((left_edge || right_edge) && !ScanForStationTiles(st->index, x, this->top, x, this->bottom));
		bool reduce_y = ((top_edge || bottom_edge) && !ScanForStationTiles(st->index, this->left, y, this->right, y));
		if (!(reduce_x || reduce_y)) break; // nothing to do (can't reduce)

		if (reduce_x) {
			/* reduce horizontally */
			if (left_edge) {
				/* move left edge right */
				this->left = x = x + 1;
			} else {
				/* move right edge left */
				this->right = x = x - 1;
			}
		}
		if (reduce_y) {
			/* reduce vertically */
			if (top_edge) {
				/* move top edge down */
				this->top = y = y + 1;
			} else {
				/* move bottom edge up */
				this->bottom = y = y - 1;
			}
		}

		if (left > right || top > bottom) {
			/* can't continue, if the remaining rectangle is empty */
			this->MakeEmpty();
			return true; // empty remaining rect
		}
	}
	return false; // non-empty remaining rect
}

bool StationRect::AfterRemoveRect(BaseStation *st, TileArea ta)
{
	assert(this->PtInExtendedRect(TileX(ta.tile), TileY(ta.tile)));
	assert(this->PtInExtendedRect(TileX(ta.tile) + ta.w - 1, TileY(ta.tile) + ta.h - 1));

	bool empty = this->AfterRemoveTile(st, ta.tile);
	if (ta.w != 1 || ta.h != 1) empty = empty || this->AfterRemoveTile(st, TILE_ADDXY(ta.tile, ta.w - 1, ta.h - 1));
	return empty;
}

StationRect& StationRect::operator = (const Rect &src)
{
	this->left = src.left;
	this->top = src.top;
	this->right = src.right;
	this->bottom = src.bottom;
	return *this;
}

/**
 * Calculates the maintenance cost of all airports of a company.
 * @param owner Company.
 * @return Total cost.
 */
Money AirportMaintenanceCost(Owner owner)
{
	Money total_cost = 0;

	const Station *st;
	FOR_ALL_STATIONS(st) {
		if (st->owner == owner && (st->facilities & FACIL_AIRPORT)) {
			total_cost += _price[PR_INFRASTRUCTURE_AIRPORT] * st->airport.GetSpec()->maintenance_cost;
		}
	}
	/* 3 bits fraction for the maintenance cost factor. */
	return total_cost >> 3;
}

void Airport::SetDepot(bool adding)
{
	if (adding) {
		assert(this->depot_id == INVALID_DEPOT);
		if (!Depot::CanAllocateItem()) NOT_REACHED();
		assert(this->GetNumHangars() > 0);
		Station *st = Station::GetByTile(this->GetHangarTile(0));
		Depot *dep = new Depot(this->GetHangarTile(0));
		this->depot_id = dep->index;
		dep->build_date = st->build_date;
		dep->town = st->town;
		dep->company = GetTileOwner(dep->xy);
		dep->veh_type = VEH_AIRCRAFT;
		dep->ta.tile = st->airport.tile;
		dep->ta.w = st->airport.w;
		dep->ta.h = st->airport.h;
		for (uint i = 0; i < this->GetNumHangars(); i++) {
			*dep->depot_tiles.Append() = this->GetHangarTile(i);
		}
	} else {
		delete Depot::GetIfValid(this->depot_id);
		this->depot_id = INVALID_DEPOT;
	}
}

DepotID GetHangarIndex(TileIndex t) {
	assert(IsAirportTile(t));
	return Station::GetByTile(t)->airport.depot_id;
}

bool StationCompare::operator() (const Station *lhs, const Station *rhs) const
{
	return lhs->build_date < rhs->build_date;
}
