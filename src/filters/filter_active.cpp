/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filter_active.cpp Functions related to the active elements of a filter. */


#include "../stdafx.h"
#include "../window_func.h"
#include "../viewport_func.h"
#include "../company_gui.h"
#include "../zoning.h"
#include "../company_func.h"
#include "filter_active.h"

/** Sort by town name */
static int CDECL TownNameSorter(FilterElement const *a, FilterElement const *b)
{
	static char buf_cache[64];
	const Town *ta = Town::GetIfValid(a->GetElement());
	const Town *tb = Town::GetIfValid(b->GetElement());
	char buf[64];

	SetDParam(0, ta->index);
	GetString(buf, STR_TOWN_NAME, lastof(buf));
	SetDParam(0, tb->index);
	GetString(buf_cache, STR_TOWN_NAME, lastof(buf_cache));

	return strnatcmp(buf, buf_cache); // Sort by name (natural sorting).
}

/** Sort by station name */
static int CDECL StationNameSorter(FilterElement const *a, FilterElement const *b)
{
	static char buf_cache[64];
	const Station *ta = Station::GetIfValid(a->GetElement());
	const Station *tb = Station::GetIfValid(b->GetElement());
	char buf[64];

	/* Stations are sorted by the name of their hometown.
	 * When hometowns are equal, stations are sorted by the name of the station */
	if (ta->town == tb->town) {
		SetDParam(0, ta->index);
		GetString(buf, STR_STATION_NAME, lastof(buf));
		SetDParam(0, tb->index);
		GetString(buf_cache, STR_STATION_NAME, lastof(buf_cache));
	} else {
		SetDParam(0, ta->town->index);
		GetString(buf, STR_TOWN_NAME, lastof(buf));
		SetDParam(0, tb->town->index);
		GetString(buf_cache, STR_TOWN_NAME, lastof(buf_cache));
	}

	return strnatcmp(buf, buf_cache); // Sort by natural name sorting
}

/** Sort by element index, default sorter */
static int CDECL DefaultSorter(FilterElement const *a, FilterElement const *b)
{
	return a->GetElement() - b->GetElement();
}

/**
 * Sort the list of the given type
 * @param type of the list to sort
 */
void FilterBase::Sort(ListType type)
{
	this->lists[type].ForceResort();
	switch (type) {
		case LT_TOWNS:
			this->lists[type].Sort(&TownNameSorter);
			break;
		case LT_STATIONS:
			this->lists[type].Sort(&StationNameSorter);
			break;
		default:
			this->lists[type].Sort(&DefaultSorter);
	}
}

/**
 * When CTRL-click on an element of the active list, shows/scrolls to town, station or company_gui
 * @param number number of the element clicked
 */
void FilterActive::CTRLClickOnActive(uint number)
{
	uint list = LT_BEGIN;
	uint number_of_elements = 0;
	for (; list < LT_END; list++) {
		if (number_of_elements + this->lists[list].Length() > number) break;
		number_of_elements += this->lists[list].Length();
	}

	number -= number_of_elements;

	switch (list) {
		case LT_TOWNS: {
			Town *t = Town::GetIfValid(this->lists[LT_TOWNS][number].GetElement());
			if (t != NULL) ScrollMainWindowToTile(t->xy);
			return;
		}

		case LT_COMPANIES:
			ShowCompany((CompanyID)this->lists[LT_COMPANIES][number].GetElement());
			return;

		case LT_STATIONS: {
			Station *st = Station::GetIfValid(this->lists[LT_STATIONS][number].GetElement());
			ScrollMainWindowToTile(st->xy);
			return;
		}

		case LT_END: NOT_REACHED();

		default: break;
	}
}

/**
 * Given a vehicle, say whether it satisfies a property
 * @param v the vehicle
 * @param property like: being stopped, crashed, lost, old...
 * @return true or false
 */
bool TestProperty(const Vehicle *v, VehicleGroupProperties property)
{
	switch (property) {
		case VGP_STOPPED:
			return v->IsStopped();
		case VGP_STOPPED_IN_DEPOT:
			return v->IsStoppedInDepot();
		case VGP_CRASHED:
			return v->IsCrashedVehicle();
		case VGP_PATHFINDER_LOST:
			return v->IsLostVehicle() || (v->type == VEH_AIRCRAFT && HasBit(Aircraft::From(v)->flags, VAF_DEST_TOO_FAR));
		case VGP_OLD_VEHICLE:
			return v->IsOldVehicle();
		case VGP_NEW_VEHICLE:
			return !v->HasReachedProfitAge();
		case VGP_NEGATIVE_PROFIT:
			return v->profit_last_year < 0;
		case VGP_MAKING_GOOD_PROFIT:
			return v->GetDisplayProfitLastYear() > VEHICLE_PROFIT_THRESHOLD;
		default: NOT_REACHED();
	}
}

/**
 * Given a group, say whether it satisfies a property
 * @param g the group
 * @param property like: being stopped, crashed, lost, old...
 * @return true if the group contains AT LEAST ONE VEHICLE with the property
 */
bool TestProperty(const Group *g, VehicleGroupProperties property, bool value)
{
	switch (property) {
		case VGP_NEW_VEHICLE: return (g->statistics.num_profit_vehicle != g->statistics.num_vehicle) == value;

		case VGP_NEGATIVE_PROFIT: return (g->statistics.min_profit_vehicle < 0) == value;

		case VGP_MAKING_GOOD_PROFIT: return (g->statistics.num_profit_vehicle != 0 && g->statistics.min_profit_vehicle >> 8 > VEHICLE_PROFIT_THRESHOLD) == value;

		default: break;
	}

	for (uint i = g->statistics.order_lists.Length(); i--;) {
		for (Vehicle *v = g->statistics.order_lists[i]->GetFirstSharedVehicle(); v != NULL; v = v->NextShared()) {
			if (v->group_id != g->index) continue;
			if (TestProperty(v, property) == value) return true;
		}
	}

	return false;
}

/**
 * Given a town, say whether it satisfies a property
 * @param t the town
 * @param property like: local company has statue, town is growing...
 * @return true if the town satisfies the property
 */
bool TestProperty(const Town *t, TownProperties property)
{
	switch (property) {
		case TP_LOCAL_COMPANY_HAS_STATUE:
			if (_local_company >= MAX_COMPANIES) return false;
			return HasBit(t->statues, _local_company);

		case TP_TOWN_IS_GROWING:
			return HasBit(t->flags, TOWN_IS_GROWING);

		default: NOT_REACHED();
	}
}

/**
 * Check if v satisfies ALL the properties of the filter
 * @param v the vehicle
 * @return true if v passes the test, false otherwise
 */
bool FilterActive::FilterTest(const Vehicle *v) const
{
	SmallVector<StationID, 32> visited_stations;
	SmallVector<TownID, 32> visited_towns;
	uint32 cargo_types = GetCargoType(v, true);

	/* revise: maybe repeated code??? */
	const Order *order;
	FOR_VEHICLE_ORDERS(v, order) {
		if ( (order->IsType(OT_GOTO_STATION) || order->IsType(OT_IMPLICIT)) && !order->IsType(OT_GOTO_WAYPOINT) ) {
			visited_stations.Include(order->GetDestination());
			const Station *st = Station::GetIfValid(order->GetDestination());
			if (st != NULL) visited_towns.Include(st->town->index);
		}
	}

	for (uint i = LT_END; i--;) {
		for (uint j = lists[i].Length(); j--;) {
			assert(lists[i][j].IsActive());
			bool state_is = lists[i][j].IsPositive();
			switch (i) {
				case LT_TOWNS:
					if (state_is != visited_towns.Contains(lists[i][j].GetElement())) return false;
					break;
				case LT_STATIONS:
					if (state_is != visited_stations.Contains(lists[i][j].GetElement())) return false;
					break;
				case LT_CARGO:
					if (state_is != HasBit(cargo_types, lists[i][j].GetElement())) return false;
					break;
				case LT_ORDERLIST:
					if (state_is != v->orders.list->IsListType((OrderListType)lists[i][j].GetElement())) return false;
					break;
				case LT_VEHICLE_GROUP_PROPERTIES:
					if (state_is != TestProperty(v, (VehicleGroupProperties)lists[i][j].GetElement())) return false;
					break;
				default: NOT_REACHED();
			}
		}
	}
	return true;
}

/**
 * Check whether FOR EACH PROPERTY the Filter has, g has AT LEAST A VEHICLE that satisfies it
 * @param g, the group
 * @return true if g passes the test, false otherwise
 */
bool FilterActive::FilterTest(const Group *g) const
{
	const GroupStatistics *stat = &g->statistics;
	SmallVector<Vehicle *, 32> vector = stat->GetListOfFirstSharedVehicles();
	SmallVector<StationID, 32> visited_stations;
	SmallVector<TownID, 32> visited_towns;
	for (uint i = vector.Length(); i--;) {
		Vehicle *v = vector[i];
		if (v == NULL) continue;
		const Order *order;
		FOR_VEHICLE_ORDERS(v, order) {
			if ( (order->IsType(OT_GOTO_STATION) || order->IsType(OT_IMPLICIT)) && !order->IsType(OT_GOTO_WAYPOINT) ) {
				visited_stations.Include(order->GetDestination());
				const Station *st = Station::GetIfValid(order->GetDestination());
				if (st != NULL) visited_towns.Include(st->town->index);
			}
		}
	}

	for (uint i = LT_END; i--;) {
		for (uint j = lists[i].Length(); j--;) {
			assert(lists[i][j].IsActive());
			bool state_is = lists[i][j].IsPositive();
			switch (i) {
				case LT_TOWNS:
					if (state_is != visited_towns.Contains(lists[i][j].GetElement())) return false;
					break;
				case LT_STATIONS:
					if (state_is != visited_stations.Contains(lists[i][j].GetElement())) return false;
					break;
				case LT_CARGO:
					if (state_is != stat->DoesGroupCarryCargoType(lists[i][j].GetElement())) return false;
					break;
				case LT_ORDERLIST:
					if (state_is != stat->HasGroupOrderListType((OrderListType)lists[i][j].GetElement())) return false;
					break;
				case LT_VEHICLE_GROUP_PROPERTIES:
					if (!TestProperty(g, (VehicleGroupProperties)lists[i][j].GetElement(), state_is)) return false;
					break;
				default: NOT_REACHED();
			}
		}
	}
	return true;
}

/**
 * Check if industry satisfies all the conditions the filter requires
 * @param industry, the industry
 * @return 1 if industry passes the test, 0 otherwise
 */
bool FilterActive::FilterTest(const Industry *industry) const
{
	for (uint i = LT_END; i--;) {
		bool one_is_active = false;
		for (uint j = lists[i].Length(); j--;) {
			assert(lists[i][j].IsActive());
			bool state_is = lists[i][j].IsPositive();
			if (state_is) one_is_active = true;
			switch (i) {
				case LT_TOWNS:
					if (industry->town->index == lists[i][j].GetElement()) {
						if (state_is)	goto next_property;
						else		return false;
					}
					break;
				case LT_CARGO:
					if ((industry->produced_cargo[0] == lists[i][j].GetElement() || industry->produced_cargo[1] == lists[i][j].GetElement()) != state_is) {
						return false;
					}
					break;
				case LT_CARGO_ACCEPTANCE:
					if ((industry->accepts_cargo[0] == lists[i][j].GetElement() || industry->accepts_cargo[1] == lists[i][j].GetElement() || industry->accepts_cargo[2] == lists[i][j].GetElement()) != state_is) {
						return false;
					}
					break;
				default: NOT_REACHED();
			}
		}

		if (one_is_active && i == LT_TOWNS) return false;
		next_property:;
	}

	return true;
}

/**
 * Check if town satisfies all the conditions the filter requires
 * @param town, the town
 * @return 1 if town passes the test, 0 otherwise
 */
bool FilterActive::FilterTest(const Town *town) const
{
	for (uint i = LT_END; i--;) {
		bool one_is_active = false;
		for (uint j = lists[i].Length(); j--;) {
			assert(lists[i][j].IsActive());
			bool state_is = lists[i][j].IsPositive();
			if (state_is) one_is_active = true;
			switch (i) {
				case LT_COMPANIES:
					if (HasBit(town->have_ratings, lists[i][j].GetElement())) {
						if (state_is)	goto next_property;
						else		return false;
					}
					break;
				case LT_TOWN_PROPERTIES:
					if (state_is != TestProperty(town, (TownProperties)lists[i][j].GetElement())) return false;
					break;
				default: NOT_REACHED();
			}
		}
		if (one_is_active && i == LT_COMPANIES) return false;
		next_property:;
	}

	return true;
}

/**
 * Check if cargo_spec satisfies all the conditions the filter requires
 * @param cs, the CargoSpec
 * @return 1 if cs passes the test, 0 otherwise
 * @see WC_STATION_FILTER window
 */
bool FilterActive::FilterTest(const CargoSpec *cs) const
{
	bool one_is_active = false;
	for (uint j = lists[LT_CARGO].Length(); j--;) {
		assert(lists[LT_CARGO][j].IsActive());
		bool state_is = lists[LT_CARGO][j].IsPositive();
		if (state_is) {
			if (lists[LT_CARGO][j].GetElement() == cs->Index()) return true;
			one_is_active = true;
		}
		if (!state_is && lists[LT_CARGO][j].GetElement() == cs->Index()) return false;
	}

	if (one_is_active) return false;

	return true;
}

/**
 * Check if station satisfies all the conditions the filter requires
 * @param station, the station
 * @return 1 if station passes the test, 0 otherwise
 */
bool FilterActive::FilterTest(const Station *st) const
{
	for (uint i = LT_END; i--;) {
		bool one_is_active = false;
		for (uint j = lists[i].Length(); j--;) {
			assert(lists[i][j].IsActive());
			bool state_is = lists[i][j].IsPositive();
			if (state_is) one_is_active = true;
			switch (i) {
				case LT_TOWNS:
					if (st->town->index == lists[i][j].GetElement()) {
						if (state_is)	goto next_property;
						else		return false;
					}
					break;
				case LT_CARGO:
					if (state_is != st->goods[lists[i][j].GetElement()].IsSourceStationForCargo()) {
						return false;
					}
					break;
				case LT_CARGO_ACCEPTANCE:
					if (state_is != HasBit(st->goods[lists[i][j].GetElement()].status, GoodsEntry::GES_EVER_ACCEPTED)) {
						return false;
					}
					break;
				case LT_STATION_FACILITIES:
					if (((st->facilities & (1 << lists[i][j].GetElement())) != 0) != state_is) {
						return false;
					}
					break;
				default: NOT_REACHED();
			}
		}

		if (one_is_active && i == LT_TOWNS) return false;
		next_property:;
	}

	return true;
}

/**
 * Check if station satisfies all the conditions the filter requires
 * @param station, the station
 * @return 1 if station passes the test, 0 otherwise
 */
bool FilterActive::FilterTestCA(const Station *st) const
{
	if (_stations_modified) {
		const FilterElement *f_element = lists[LT_STATIONS].Find(st->index);
		if (f_element == lists[LT_STATIONS].End()) {
			return false;
		} else {
			return f_element->IsPositive();
		}
	}

	/* Station list is not modified */
	if (lists[LT_COMPANIES].Length() == 0 && lists[LT_TOWNS].Length() == 0 && lists[LT_CARGO].Length() == 0 && lists[LT_CARGO_ACCEPTANCE].Length() == 0 && lists[LT_STATION_FACILITIES].Length() == 0) return false;

	for (uint i = LT_END; i--;) {
		bool one_is_active = false;
		for (uint j = lists[i].Length(); j--;) {
			assert(lists[i][j].IsActive());
			bool state_is = lists[i][j].IsPositive();
			if (state_is) one_is_active = true;
			switch (i) {
				case LT_TOWNS:
					if (st->town->index == lists[i][j].GetElement()) {
						if (state_is)	goto next_property;
						else		return false;
					}
					break;
				case LT_COMPANIES:
					if (st->owner == lists[i][j].GetElement()) {
						if (state_is)	goto next_property;
						else		return false;
					}
					break;
				case LT_CARGO:
					if (st->goods[lists[i][j].GetElement()].IsSourceStationForCargo() != state_is) {
						return false;
					}
					break;
				case LT_CARGO_ACCEPTANCE:
					if (HasBit(st->goods[lists[i][j].GetElement()].status, GoodsEntry::GES_EVER_ACCEPTED) != state_is) {
						return false;
					}
					break;
				case LT_STATION_FACILITIES:
					if (((st->facilities & (1 << lists[i][j].GetElement())) != 0) != state_is) {
						return false;
					}
					break;
				case LT_STATIONS:
				case LT_CATCHMENT_AREA_PROPERTIES:
					goto next_property;
					break;
				default: NOT_REACHED();
			}
		}

		if (one_is_active && (i == LT_COMPANIES || i == LT_TOWNS)) return false;
		next_property:;
	}

	return true;
}