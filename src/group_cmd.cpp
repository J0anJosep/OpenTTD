/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file group_cmd.cpp Handling of the engine groups */

#include "stdafx.h"
#include "cmd_helper.h"
#include "command_func.h"
#include "train.h"
#include "vehiclelist.h"
#include "vehicle_func.h"
#include "autoreplace_base.h"
#include "autoreplace_func.h"
#include "string_func.h"
#include "company_func.h"
#include "core/pool_func.hpp"
#include "order_backup.h"
#include "strings_func.h"
#include "window_gui.h"

#include "table/strings.h"

#include "safeguards.h"

GroupID _new_group_id;

GroupPool _group_pool("Group");
INSTANTIATE_POOL_METHODS(Group)

GroupStatistics::GroupStatistics() : min_profit_vehicle(INT64_MAX), ol_type(OLT_EMPTY_GROUP)
{
	this->num_engines = CallocT<uint16>(Engine::GetPoolSize());
}

GroupStatistics::~GroupStatistics()
{
	free(this->num_engines);
}

/**
 * Clear all caches.
 */
void GroupStatistics::Clear()
{
	this->num_vehicle = 0;
	this->ClearProfits();
	this->order_lists.Clear();
	this->ol_type = OLT_EMPTY_GROUP;
	this->ClearCargo();

	/* This is also called when NewGRF change. So the number of engines might have changed. Reallocate. */
	free(this->num_engines);
	this->num_engines = CallocT<uint16>(Engine::GetPoolSize());
}

/**
 * Returns the GroupStatistics for a specific group.
 * @param company Owner of the group.
 * @param id_g    GroupID of the group.
 * @param type    VehicleType of the vehicles in the group.
 * @return Statistics for the group.
 */
/* static */ GroupStatistics &GroupStatistics::Get(CompanyID company, GroupID id_g, VehicleType type)
{
	if (Group::IsValidID(id_g)) {
		Group *g = Group::Get(id_g);
		assert(g->owner == company);
		assert(g->vehicle_type == type);
		return g->statistics;
	}

	if (IsDefaultGroupID(id_g)) return Company::Get(company)->group_default[type];
	if (IsAllGroupID(id_g)) return Company::Get(company)->group_all[type];

	NOT_REACHED();
}

/**
 * Returns the GroupStatistic for the group of a vehicle.
 * @param v Vehicle.
 * @return GroupStatistics for the group of the vehicle.
 */
/* static */ GroupStatistics &GroupStatistics::Get(const Vehicle *v)
{
	return GroupStatistics::Get(v->owner, v->group_id, v->type);
}

/**
 * Returns the GroupStatistic for the ALL_GROUPO of a vehicle type.
 * @param v Vehicle.
 * @return GroupStatistics for the ALL_GROUP of the vehicle type.
 */
/* static */ GroupStatistics &GroupStatistics::GetAllGroup(const Vehicle *v)
{
	return GroupStatistics::Get(v->owner, ALL_GROUP, v->type);
}

/**
 * Update all caches after loading a game, changing NewGRF, etc.
 */
/* static */ void GroupStatistics::UpdateAfterLoad()
{
	/* Set up the engine count for all companies */
	Company *c;
	FOR_ALL_COMPANIES(c) {
		for (VehicleType type = VEH_BEGIN; type < VEH_COMPANY_END; type++) {
			c->group_all[type].Clear();
			c->group_default[type].Clear();
		}
	}

	/* Recalculate */
	Group *g;
	FOR_ALL_GROUPS(g) {
		g->statistics.Clear();
	}

	const Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		if (!v->IsEngineCountable()) continue;

		GroupStatistics::CountEngine(v, 1);
		if (v->IsPrimaryVehicle()) GroupStatistics::CountVehicle(v, 1);
	}

	FOR_ALL_COMPANIES(c) {
		GroupStatistics::UpdateAutoreplace(c->index);
	}

	const OrderList *order_list;
	FOR_ALL_ORDER_LISTS(order_list) {
		v = order_list->GetFirstSharedVehicle();
		//revise
		if (v == NULL) continue;
		GroupStatistics::GetAllGroup(v).order_lists.Include(order_list);
	}

	FOR_ALL_GROUPS(g) {
		g->statistics.UpdateCargoTypes();
	}

	FOR_ALL_COMPANIES(c) {
		for (VehicleType type = VEH_BEGIN; type < VEH_COMPANY_END; type++) {
			GroupStatistics::UpdateCargo(c->index, ALL_GROUP, type);
			GroupStatistics::UpdateCargo(c->index, DEFAULT_GROUP, type);
		}
	}
}

/**
 * Update num_vehicle when adding or removing a vehicle.
 * @param v Vehicle to count.
 * @param delta +1 to add, -1 to remove.
 */
/* static */ void GroupStatistics::CountVehicle(const Vehicle *v, int delta)
{
	assert(delta == 1 || delta == -1);

	GroupStatistics &stats_all = GroupStatistics::GetAllGroup(v);
	GroupStatistics &stats = GroupStatistics::Get(v);

	stats_all.num_vehicle += delta;
	stats.num_vehicle += delta;

	if (v->orders.list != NULL) {
		if (delta == 1) {
			stats.order_lists.Include(v->orders.list);
			stats.AddOrderListType(v->GetOrderListType());
		} else {
			stats.RemoveOrderList(v);
			stats.RemoveOrderListType(v);
		}
	}

	/* Cargo */
	for (const Vehicle *u = v; u != NULL; u = u->Next()) {
		stats.act_cargo[u->cargo_type]	+= u->cargo.StoredCount() * delta;
		stats.max_cargo[u->cargo_type]	+= u->cargo_cap * delta;
	}

	if (v->age > VEHICLE_PROFIT_MIN_AGE) VehicleReachedProfitAge(v, delta);
}

/**
 * Update num_engines when adding/removing an engine.
 * @param v Engine to count.
 * @param delta +1 to add, -1 to remove.
 */
/* static */ void GroupStatistics::CountEngine(const Vehicle *v, int delta)
{
	assert(delta == 1 || delta == -1);
	GroupStatistics::GetAllGroup(v).num_engines[v->engine_type] += delta;
	GroupStatistics::Get(v).num_engines[v->engine_type] += delta;
}

/**
 * Add a vehicle to the profit sum of its group.
 */
/* static */ void GroupStatistics::VehicleReachedProfitAge(const Vehicle *v, int delta)
{
	assert(delta == 1 || delta == -1);

	GroupStatistics &stats_all = GroupStatistics::GetAllGroup(v);
	GroupStatistics &stats = GroupStatistics::Get(v);

	stats_all.num_profit_vehicle += delta;
	stats_all.profit_last_year += v->GetDisplayProfitLastYear() * delta;
	stats_all.min_profit_vehicle = min(stats_all.min_profit_vehicle, v->profit_last_year);
	stats.num_profit_vehicle += delta;
	stats.profit_last_year += v->GetDisplayProfitLastYear() * delta;

	if (delta == 1) {
		stats.min_profit_vehicle = min(stats.min_profit_vehicle, v->profit_last_year);
		stats_all.min_profit_vehicle = min(stats_all.min_profit_vehicle, v->profit_last_year);
	} else {
		if (stats_all.min_profit_vehicle == v->profit_last_year) stats_all.UpdateMinProfit(v, ALL_GROUP);
		if (stats.min_profit_vehicle == v->profit_last_year) stats.UpdateMinProfit(v, v->group_id);
	}
}

/**
 * Recompute the profits for all groups.
 */
/* static */ void GroupStatistics::UpdateProfits()
{
	/* Set up the engine count for all companies */
	Company *c;
	FOR_ALL_COMPANIES(c) {
		for (VehicleType type = VEH_BEGIN; type < VEH_COMPANY_END; type++) {
			c->group_all[type].ClearProfits();
			c->group_default[type].ClearProfits();
		}
	}

	/* Recalculate */
	Group *g;
	FOR_ALL_GROUPS(g) {
		g->statistics.ClearProfits();
	}

	const Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		if (v->IsPrimaryVehicle() && v->age > VEHICLE_PROFIT_MIN_AGE) GroupStatistics::VehicleReachedProfitAge(v, 1);
	}
}

/**
 * Update autoreplace_defined and autoreplace_finished of all statistics of a company.
 * @param company Company to update statistics for.
 */
/* static */ void GroupStatistics::UpdateAutoreplace(CompanyID company)
{
	/* Set up the engine count for all companies */
	Company *c = Company::Get(company);
	for (VehicleType type = VEH_BEGIN; type < VEH_COMPANY_END; type++) {
		c->group_all[type].ClearAutoreplace();
		c->group_default[type].ClearAutoreplace();
	}

	/* Recalculate */
	Group *g;
	FOR_ALL_GROUPS(g) {
		if (g->owner != company) continue;
		g->statistics.ClearAutoreplace();
	}

	for (EngineRenewList erl = c->engine_renew_list; erl != NULL; erl = erl->next) {
		const Engine *e = Engine::Get(erl->from);
		GroupStatistics &stats = GroupStatistics::Get(company, erl->group_id, e->type);
		if (!stats.autoreplace_defined) {
			stats.autoreplace_defined = true;
			stats.autoreplace_finished = true;
		}
		if (stats.num_engines[erl->from] > 0) stats.autoreplace_finished = false;
	}
}

/**
 * Update the num engines of a groupID. Decrease the old one and increase the new one
 * @note called in SetTrainGroupID and UpdateTrainGroupID
 * @param v     Vehicle we have to update
 * @param old_g index of the old group
 * @param new_g index of the new group
 */
static inline void UpdateNumEngineGroup(const Vehicle *v, GroupID old_g, GroupID new_g)
{
	if (old_g != new_g) {
		/* Decrease the num engines in the old group */
		GroupStatistics::Get(v->owner, old_g, v->type).num_engines[v->engine_type]--;

		/* Increase the num engines in the new group */
		GroupStatistics::Get(v->owner, new_g, v->type).num_engines[v->engine_type]++;
	}
}

/**
 * Update the cargo a specific group is carrying
 * @param company owner of the group
 * @param id_g specific groupID
 */
/* static */ void GroupStatistics::UpdateCargo(CompanyID company, GroupID id_g, VehicleType type)
{
	GroupStatistics &stats = GroupStatistics::Get(company, id_g, type);
	stats.ClearCargo();

	for (uint i = stats.order_lists.Length(); i--;) {
		for (Vehicle *v = stats.order_lists[i]->GetFirstSharedVehicle(); v != NULL; v = v->NextShared()) {
			if (v->group_id == id_g || id_g == ALL_GROUP) {
				for (const Vehicle *u = v; u != NULL; u = u->Next()) {
					stats.act_cargo[u->cargo_type] += u->cargo.StoredCount();
					stats.max_cargo[u->cargo_type] += u->cargo_cap;
				}
			}
		}
	}
	stats.UpdateCargoTypes();
}

/* static */ void GroupStatistics::UpdateCargoForVehicleType(CompanyID company, VehicleType vtype)
{
	GroupStatistics::UpdateCargo(company, ALL_GROUP, vtype);
	GroupStatistics::UpdateCargo(company, DEFAULT_GROUP, vtype);

	/* rest of groups */
	Group *g;
	FOR_ALL_GROUPS(g) {
		if (g->vehicle_type == vtype && g->owner == company) {
			GroupStatistics::UpdateCargo(company, g->index, vtype);
		}
	}

	SetWindowClassesDirty(WC_GROUP_DETAILS);
}

/**
 * Update group orderlisttype when a vehicle is added to the group
 * @param new_ol new type of orderlist added to the group
 */
void GroupStatistics::AddOrderListType(const OrderListType new_ol)
{
	if (this->ol_type == OLT_AUTOFILLING && (new_ol == OLT_INCOMPLETE || new_ol == OLT_UNPUNCTUAL)) {
		this->ol_type = new_ol;
		return;
	}
	if ((this->ol_type == OLT_INCOMPLETE || this->ol_type == OLT_UNPUNCTUAL) && new_ol == OLT_AUTOFILLING)
		return;
	this->ol_type = min(this->ol_type, new_ol);
}

/**
 * Vehicle is taken out; update orderlist_type
 */
void GroupStatistics::RemoveOrderListType(const Vehicle *vehicle_to_take_out)
{
	if (this->ol_type < vehicle_to_take_out->GetOrderListType()) return;
	this->ol_type = OLT_EMPTY_GROUP;
	Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		if (v->IsPrimaryVehicle() && v != vehicle_to_take_out && v->group_id == vehicle_to_take_out->group_id)
			this->AddOrderListType(v->GetOrderListType());
	}
}

/**
 * When a vehicle is taken out and the min profit of a group is the one of the vehicle
 * recalculates which is the new min profit
 */
void GroupStatistics::UpdateMinProfit(const Vehicle *vehicle_to_take_out, const GroupID g_id)
{
	min_profit_vehicle = INT64_MAX;
	Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		/* revise: nasty */
		if (v->IsPrimaryVehicle() && (v->group_id == g_id || g_id == ALL_GROUP) && v->owner == vehicle_to_take_out->owner && v->type == vehicle_to_take_out->type && v->age > VEHICLE_PROFIT_MIN_AGE && v != vehicle_to_take_out)
			this->min_profit_vehicle = min(this->min_profit_vehicle, v->profit_last_year);
	}
}

/**
 * Remove an OrderList from group if no other vehicle of the group has it
 * v is a vehicle being removed from this group
 */
void GroupStatistics::RemoveOrderList(const Vehicle *v)
{
	assert(v != NULL);
	if (v->orders.list == NULL) return;
	for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
		if (v2->group_id == v->group_id && v2 != v)
			return;
	}
	/* look for the order list and delete it */
	this->order_lists.FindAndErase(v->orders.list);
}

/**
 * Updates the cargo_types field,
 * looking for cargos that can be carried with vehicles of this group
 * also looking for refit orders
 */
void GroupStatistics::UpdateCargoTypes()
{
	this->cargo_types = 0;

	//for all refit orders for vehicles of this group
	for (uint i = order_lists.Length(); i--;) {
		for (Order *order = order_lists[i]->GetFirstOrder(); order != NULL; order = order->next) {
			if ((order->IsType(OT_GOTO_DEPOT) || order->IsType(OT_GOTO_STATION)) && order->IsRefit()) SetBit(this->cargo_types, order->GetRefitCargo());
		}
	}

	//for the 64 bits of cargo_types...
	for (CargoID i = 0; i < NUM_CARGO; i++) {
		if (this->max_cargo[i] > 0) SetBit(this->cargo_types, i);
	}
}

/**
 * Returns a DropDownList containing the different orderlists on the group
 */
DropDownList *GroupStatistics::BuildSharedOrdersDropdown() const
{
	if (this->order_lists.Length() < 2) return NULL;
	DropDownList *list = new DropDownList();
	uint length = this->order_lists.Length();
	for (uint i = 0; i < length; i++) {
		uint param_number = 0;
		uint orders_to_show = 3;
		DropDownListParamStringItem *new_item = new DropDownListParamStringItem(STR_GROUP_VEHICLE_WINDOW_STR, this->order_lists[i]->GetFirstSharedVehicle()->index, false);
		new_item->SetParam(param_number, STR_GROUP_LIST_TIMETABLE_ABBREV_INVALID + this->order_lists[i]->GetOrderListType());
		new_item->SetParam(++param_number, this->order_lists[i]->GetNumVehicles());
		new_item->SetParam(++param_number, this->order_lists[i]->GetNumOrders());
		SmallVector<DestinationID, 32> destinations = GetDestinations(this->order_lists[i]);
		for (uint j = 0; j < min(orders_to_show, destinations.Length()); j++) {
			new_item->SetParam(++param_number, j == 0 ? STR_GROUP_VEHICLE_WINDOW_STR_1ST : STR_GROUP_VEHICLE_WINDOW_STR_ADDED);
			new_item->SetParam(++param_number, destinations[j]);
		}
		new_item->SetParam(++param_number, destinations.Length() > orders_to_show ? STR_GROUP_VEHICLE_WINDOW_STR_ETC : STR_EMPTY);
		*list->Append() = new_item;
	}
	return list;
}

/**
 * For all vehicles in this group, return only the first vehicle of each shared schedule
 */
SmallVector<Vehicle *, 32> GroupStatistics::GetListOfFirstSharedVehicles() const
{
	SmallVector<Vehicle *, 32> vector;
	Vehicle *v;
	for (uint i = this->order_lists.Length(); i--;) {
		v = this->order_lists[i]->GetFirstSharedVehicle();
		if (v == NULL) continue;
		*vector.Append() = v;
	}
	return vector;
}

/**
 * Check if this group has an orderlist of the given type
 */
bool GroupStatistics::HasGroupOrderListType(OrderListType ol_type) const
{
	if (ol_type == OLT_EMPTY_GROUP) {
		for (uint i = order_lists.Length(); i--;) {
			if (!order_lists[i]->IsListType(OLT_EMPTY)) return false;
		}
		return true;
	}
	for (uint i = order_lists.Length(); i--;) {
		if (order_lists[i]->IsListType(ol_type)) return true;
	}
	return false;
}

/**
 * Chek if a group has capacity for a type of cargo (maybe hidden as a refit order)
 * @return 1 if has capacity or a refit order for the cargo; 0 otherwise
 */
bool GroupStatistics::DoesGroupCarryCargoType(CargoID cargo) const
{
	return HasBit(this->cargo_types, cargo);
}

/**
 * Return the group profit icon
 * it is calculated using the profit of the vehicle with worst profit last year
 * @return SpriteID
 */
SpriteID GroupStatistics::SetGroupProfitSpriteID() const
{
	if (this->num_profit_vehicle == 0 ) return SPR_PROFIT_NA;
	if (this->min_profit_vehicle < 0) return SPR_PROFIT_NEGATIVE;
	if (this->min_profit_vehicle >> 8 < VEHICLE_PROFIT_THRESHOLD) return SPR_PROFIT_SOME;
	return SPR_PROFIT_LOT;
}

Group::Group(Owner owner)
{
	this->owner = owner;
	this->parent = INVALID_GROUP;
}

Group::~Group()
{
	if (!CleaningPool()) {
		VehicleListIdentifier vli(VL_GROUP_LIST, this->vehicle_type, this->owner, this->index);
		DeleteWindowById(GetWindowClassForVehicleType(this->vehicle_type), vli.Pack());
		DeleteWindowById(WC_GROUP_DETAILS, vli.Pack());
	}
	free(this->name);
}

/**
 * Create a new vehicle group.
 * @param tile unused
 * @param flags type of operation
 * @param p1   vehicle type
 * @param p2   unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdCreateGroup(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleType vt = Extract<VehicleType, 0, 3>(p1);
	if (!IsCompanyBuildableVehicleType(vt)) return CMD_ERROR;

	if (!Group::CanAllocateItem()) return CMD_ERROR;

	if (flags & DC_EXEC) {
		Group *g = new Group(_current_company);
		g->replace_protection = false;
		g->vehicle_type = vt;
		g->parent = INVALID_GROUP;

		_new_group_id = g->index;

		InvalidateWindowData(GetWindowClassForVehicleType(vt), VehicleListIdentifier(VL_GROUPS_WINDOW, vt, _current_company).Pack());
	}

	return CommandCost();
}


/**
 * Add all vehicles in the given group to the default group and then deletes the group.
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of array group
 *      - p1 bit 0-15 : GroupID
 * @param p2   unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdDeleteGroup(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Group *g = Group::GetIfValid(p1);
	if (g == NULL || g->owner != _current_company) return CMD_ERROR;

	/* Remove all vehicles from the group */
	DoCommand(0, p1, 0, flags, CMD_REMOVE_ALL_VEHICLES_GROUP);

	/* Delete sub-groups */
	Group *gp;
	FOR_ALL_GROUPS(gp) {
		if (gp->parent == g->index) {
			DoCommand(0, gp->index, 0, flags, CMD_DELETE_GROUP);
		}
	}

	if (flags & DC_EXEC) {
		/* Update backupped orders if needed */
		OrderBackup::ClearGroup(g->index);

		/* If we set an autoreplace for the group we delete, remove it. */
		if (_current_company < MAX_COMPANIES) {
			Company *c;
			EngineRenew *er;

			c = Company::Get(_current_company);
			FOR_ALL_ENGINE_RENEWS(er) {
				if (er->group_id == g->index) RemoveEngineReplacementForCompany(c, er->from, g->index, flags);
			}
		}

		VehicleType vt = g->vehicle_type;

		/* Delete the Replace Vehicle Windows */
		DeleteWindowById(WC_REPLACE_VEHICLE, g->vehicle_type);
		delete g;

		InvalidateWindowClassesData(GetWindowClassForVehicleType(vt));
	}

	return CommandCost();
}

static bool IsUniqueGroupName(const char *name, const Owner owner, const VehicleTypeByte vehicle_type)
{
	const Group *g;

	FOR_ALL_GROUPS(g) {
		if (g->owner == owner &&
				g->vehicle_type == vehicle_type &&
				g->name != NULL &&
				strcmp(g->name, name) == 0)
			return false;
	}

	return true;
}

/**
 * Delete group names of a given company and a vehicle type.
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of array group
 *   - p1 bit 0-31 : VehicleListIdentifier
 * @param p2 unused
 * @param text the new name or an empty string when resetting to the default
 * @return the cost of this operation or an error
 */
CommandCost CmdDefaultName(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleListIdentifier vli;
	if (!vli.UnpackIfValid(p1)) return CMD_ERROR;
	if (vli.company != _current_company) return CMD_ERROR;

	if (flags & DC_EXEC) {
		Group *g;
		FOR_ALL_GROUPS(g) {
			if (g->owner != _current_company) continue;
			if (g->vehicle_type != vli.vtype) continue;
			/* Delete the old name */
			free(g->name);
			g->name = NULL;
		}

		SetWindowDirty(WC_REPLACE_VEHICLE, vli.vtype);
		SetWindowClassesDirty(WC_GROUP_DETAILS);
		InvalidateWindowClassesData(GetWindowClassForVehicleType(vli.vtype));
	}

	return CommandCost();
}

/**
 * Alter a group
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of array group
 *   - p1 bit 0-15 : GroupID (p2 == 0)
 *   - p1 bit 16-31: Parent Group ID (p2 == 0)
 *   - p1 bit 0-31 : VehicleListIdentifier (p2 == 1)
 * @param p2   0: only rename the name of the given group
 *             1: rename all groups of a company and given vehicle type to NULL
 *             2: set group parent
 * @param text the new name or an empty string when resetting to the default
 * @return the cost of this operation or an error
 */
CommandCost CmdAlterGroup(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	assert(p2 < 3);
	assert(p2 == 0 || text == NULL);

	if (p2 == 1) return CmdDefaultName(tile, flags, p1, p2, text);

	Group *g = Group::GetIfValid(GB(p1, 0, 16));
	if (g == NULL || g->owner != _current_company) return CMD_ERROR;

	if (p2 == 0) {
		// just rename this group
		bool reset = StrEmpty(text);
		if (!reset) {
			if (Utf8StringLength(text) >= MAX_LENGTH_GROUP_NAME_CHARS) return CMD_ERROR;
			if (!IsUniqueGroupName(text, g->owner, g->vehicle_type)) return_cmd_error(STR_ERROR_NAME_MUST_BE_UNIQUE);
		}

		if (flags & DC_EXEC) {
			/* Delete the old name */
			free(g->name);
			/* Assign the new one */
			g->name = reset ? NULL : stredup(text);

			SetWindowDirty(WC_GROUP_DETAILS, VehicleListIdentifier(VL_GROUP_LIST, g->vehicle_type, g->owner, p1).Pack());
		}
	} else {
		assert(p2 == 2);
		/* Set group parent */
		const Group *pg = Group::GetIfValid(GB(p1, 16, 16));

		if (pg != NULL) {
			if (pg->index == g->index) return CMD_ERROR;
			if (pg->index == g->parent) return CMD_ERROR;

			if (pg->owner != _current_company) return CMD_ERROR;
			if (pg->vehicle_type != g->vehicle_type) return CMD_ERROR;

			/* Ensure request parent isn't child of group.
			 * This is the only place that infinite loops are prevented. */
			if (GroupIsInGroup(pg->index, g->index)) return CMD_ERROR;
		}
		if (flags & DC_EXEC) {
			g->parent = (pg == NULL) ? INVALID_GROUP : pg->index;
		}
	}

	if (flags & DC_EXEC) {
		SetWindowDirty(WC_REPLACE_VEHICLE, g->vehicle_type);
		InvalidateWindowClassesData(GetWindowClassForVehicleType(g->vehicle_type));
	}

	return CommandCost();
}

/**
 * Insert a vehicle to a group, without removing it from the previous group
 * @param v vehicle to add
 * @param g_id group index where to add the vehicle
 * @note Sometimes, when fast rebuilding of groups, you do not need to remove the vehicle from the old group
 */
static void InsertVehicleToGroup(Vehicle *v, GroupID g_id)
{
	switch (v->type) {
		default: NOT_REACHED();
		case VEH_TRAIN:
			SetTrainGroupID(Train::From(v), g_id);
			break;
		case VEH_ROAD:
		case VEH_SHIP:
		case VEH_AIRCRAFT:
			if (v->IsEngineCountable()) UpdateNumEngineGroup(v, v->group_id, g_id);
			v->group_id = g_id;
			break;
	}
	GroupStatistics::CountVehicle(v, 1);
}

/**
 * Do add a vehicle to a group: remove from old group and add to new group
 * @param v Vehicle to add.
 * @param new_g Group to add to.
 */
static void AddVehicleToGroup(Vehicle *v, GroupID new_g)
{
	GroupStatistics::CountVehicle(v, -1);
	InsertVehicleToGroup(v, new_g);
}

/**
 * Add a vehicle to a group
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of array group
 *   - p1 bit 0-15 : GroupID
 * @param p2   vehicle to add to a group
 *   - p2 bit 0-19 : VehicleID
 *   - p2 bit   31 : Add shared vehicles as well.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdAddVehicleGroup(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Vehicle *v = Vehicle::GetIfValid(GB(p2, 0, 20));
	GroupID new_g = p1;

	if (v == NULL || (!Group::IsValidID(new_g) && !IsDefaultGroupID(new_g) && new_g != NEW_GROUP)) return CMD_ERROR;

	if (Group::IsValidID(new_g)) {
		Group *g = Group::Get(new_g);
		if (g->owner != _current_company || g->vehicle_type != v->type) return CMD_ERROR;
	}

	if (v->owner != _current_company || !v->IsPrimaryVehicle()) return CMD_ERROR;

	if (new_g == NEW_GROUP) {
		/* Create new group. */
		CommandCost ret = CmdCreateGroup(0, flags, v->type, 0, NULL);
		if (ret.Failed()) return ret;

		new_g = _new_group_id;
	}

	if (flags & DC_EXEC) {
		AddVehicleToGroup(v, new_g);

		if (HasBit(p2, 31)) {
			/* Add vehicles in the shared order list as well. */
			for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
				if (v2->group_id != new_g) AddVehicleToGroup(v2, new_g);
			}
		}

		GroupStatistics::UpdateAutoreplace(v->owner);

		/* Update the Replace Vehicle Windows */
		SetWindowDirty(WC_REPLACE_VEHICLE, v->type);
		SetWindowClassesDirty(WC_GROUP_DETAILS);
		InvalidateWindowClassesData(GetWindowClassForVehicleType(v->type));
	}

	return CommandCost();
}

/**
 * Add all shared vehicles of all vehicles from a group
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of group array
 *  - p1 bit 0-15 : GroupID
 * @param p2   type of vehicles
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdAddSharedVehicleGroup(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleType type = Extract<VehicleType, 0, 3>(p2);
	GroupID id_g = p1;
	if (!(Group::IsValidID(id_g) || IsDefaultGroupID(id_g)) || !IsCompanyBuildableVehicleType(type)) return CMD_ERROR;
	if (!IsDefaultGroupID(id_g)) {
		Group *g = Group::GetIfValid(id_g);
		if (g == NULL || g->owner != _current_company) return CMD_ERROR;
	}

	if (flags & DC_EXEC) {
		Vehicle *v;

		/* Find the first front engine which belong to the group id_g
		 * then add all shared vehicles of this front engine to the group id_g */
		FOR_ALL_VEHICLES(v) {
			if (v->type == type && v->IsPrimaryVehicle()) {
				if (v->group_id != id_g || v->owner != _current_company) continue;
				Vehicle *v2 = v->FirstShared();
				if (v2 != v && v2->group_id == id_g) continue;
				DoCommand(tile, id_g,  v2->index | 1 << 31, flags, CMD_ADD_VEHICLE_GROUP, text);
			}
		}
	}

	return CommandCost();
}


/**
 * Remove all vehicles from a group
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of group array
 * - p1 bit 0-15 : GroupID
 * @param p2   unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdRemoveAllVehiclesGroup(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	GroupID old_g = p1;
	Group *g = Group::GetIfValid(old_g);

	if (g == NULL || g->owner != _current_company) return CMD_ERROR;

	if (flags & DC_EXEC) {
		Vehicle *v;

		/* Find each Vehicle that belongs to the group old_g and add it to the default group */
		FOR_ALL_VEHICLES(v) {
			if (v->IsPrimaryVehicle()) {
				if (v->group_id != old_g) continue;

				/* Add The Vehicle to the default group */
				DoCommand(tile, DEFAULT_GROUP, v->index, flags, CMD_ADD_VEHICLE_GROUP, text);
			}
		}
	}

	return CommandCost();
}

/**
 * Set replace protection for a group and its sub-groups.
 * @param g initial group.
 * @param protect 1 to set or 0 to clear protection.
 */
static void SetGroupReplaceProtection(Group *g, bool protect, bool subgroups)
{
	g->replace_protection = protect;

	if (!subgroups) return;

	Group *pg;
	FOR_ALL_GROUPS(pg) {
		if (pg->parent == g->index) SetGroupReplaceProtection(pg, protect, subgroups);
	}
}

/**
 * (Un)set global replace protection from a group
 * @param tile unused
 * @param flags type of operation
 * @param p1   index of group array
 * - p1 bit 0-15 : GroupID
 * @param p2
 * - p2 bit 0    : 1 to set or 0 to clear protection.
 * - p2 bit 1    : 1 to apply to sub-groups.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdSetGroupReplaceProtection(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Group *g = Group::GetIfValid(p1);
	if (g == NULL || g->owner != _current_company) return CMD_ERROR;

	if (flags & DC_EXEC) {
		SetGroupReplaceProtection(g, HasBit(p2, 0), HasBit(p2, 1));

		SetWindowDirty(GetWindowClassForVehicleType(g->vehicle_type), VehicleListIdentifier(VL_GROUPS_WINDOW, g->vehicle_type, _current_company).Pack());
		InvalidateWindowData(WC_REPLACE_VEHICLE, g->vehicle_type);
	}

	return CommandCost();
}

/**
 * Decrease the num_vehicle variable before delete an front engine from a group
 * @note Called in CmdSellRailWagon and DeleteLasWagon,
 * @param v     FrontEngine of the train we want to remove.
 */
void RemoveVehicleFromGroup(const Vehicle *v)
{
	if (!v->IsPrimaryVehicle()) return;

	if (!IsDefaultGroupID(v->group_id)) GroupStatistics::CountVehicle(v, -1);
}


/**
 * Affect the groupID of a train to new_g.
 * @note called in CmdAddVehicleGroup and CmdMoveRailVehicle
 * @param v     First vehicle of the chain.
 * @param new_g index of array group
 */
void SetTrainGroupID(Train *v, GroupID new_g)
{
	if (!Group::IsValidID(new_g) && !IsDefaultGroupID(new_g)) return;

	assert(v->IsFrontEngine() || IsDefaultGroupID(new_g));

	for (Vehicle *u = v; u != NULL; u = u->Next()) {
		if (u->IsEngineCountable()) UpdateNumEngineGroup(u, u->group_id, new_g);

		u->group_id = new_g;
	}

	/* Update the Replace Vehicle Windows */
	GroupStatistics::UpdateAutoreplace(v->owner);
	SetWindowDirty(WC_REPLACE_VEHICLE, VEH_TRAIN);
}


/**
 * Recalculates the groupID of a train. Should be called each time a vehicle is added
 * to/removed from the chain,.
 * @note this needs to be called too for 'wagon chains' (in the depot, without an engine)
 * @note Called in CmdBuildRailVehicle, CmdBuildRailWagon, CmdMoveRailVehicle, CmdSellRailWagon
 * @param v First vehicle of the chain.
 */
void UpdateTrainGroupID(Train *v)
{
	assert(v->IsFrontEngine() || v->IsFreeWagon());

	GroupID new_g = v->IsFrontEngine() ? v->group_id : (GroupID)DEFAULT_GROUP;
	for (Vehicle *u = v; u != NULL; u = u->Next()) {
		if (u->IsEngineCountable()) UpdateNumEngineGroup(u, u->group_id, new_g);

		u->group_id = new_g;
	}

	/* Update the Replace Vehicle Windows */
	GroupStatistics::UpdateAutoreplace(v->owner);
	SetWindowDirty(WC_REPLACE_VEHICLE, VEH_TRAIN);
}

/**
 * Get the number of engines with EngineID id_e in the group with GroupID
 * id_g and its sub-groups.
 * @param company The company the group belongs to
 * @param id_g The GroupID of the group used
 * @param id_e The EngineID of the engine to count
 * @return The number of engines with EngineID id_e in the group
 */
uint GetGroupNumEngines(CompanyID company, GroupID id_g, EngineID id_e)
{
	uint count = 0;
	const Engine *e = Engine::Get(id_e);
	if (Company::Get(company)->settings.group_hierarchy) {
		const Group *g;
		FOR_ALL_GROUPS(g) {
			if (g->parent == id_g) count += GetGroupNumEngines(company, g->index, id_e);
		}
	}

	return count + GroupStatistics::Get(company, id_g, e->type).num_engines[id_e];
}

/**
 * Get the number of groups of a company and a certain vehicle type.
 * @param company The company the group belongs to.
 * @param type Vehicle Type.
 * @return The number of groups of a company and vehicle type.
 */
uint GetGroupNumByVehicleType(CompanyID company, VehicleType type)
{
	uint count = 0;
	Group *g;
	FOR_ALL_GROUPS(g) {
		if (g->owner == company && g->vehicle_type == type) count++;
	}

	return count;
}


void RemoveAllGroupsForCompany(const CompanyID company)
{
	Group *g;

	FOR_ALL_GROUPS(g) {
		if (company == g->owner) delete g;
	}
}

/**
 * Test if GroupID group is a descendant of (or is) GroupID search
 * @param search The GroupID to search in
 * @param group The GroupID to search for
 * @return True iff group is search or a descendant of search
 */
bool GroupIsInGroup(GroupID search, GroupID group)
{
	if (!Group::IsValidID(search)) return search == group;

	do {
		if (search == group) return true;
		search = Group::Get(search)->parent;
	} while (search != INVALID_GROUP);

	return false;
}

/**
 * Test if GroupID group is a descendant of (or is) GroupID search
 * and if it should inherit properties from parent.
 * @param search The GroupID to search in
 * @param group The GroupID to search for
 * @return True iff group is search or a descendant of search and must inherit properties.
 */
bool GroupInheritsFromGroup(GroupID search, GroupID group)
{
	if (search == group) return true;

	if (search == NEW_GROUP ||
	    search == ALL_GROUP ||
	    search == DEFAULT_GROUP ||
	    search == INVALID_GROUP) return false;

	Group *g = Group::Get(search);
	if (!Company::Get(g->owner)->settings.group_hierarchy) return false;

	for (;;) {
		if (g->parent == group) return true;
		if (g->parent == INVALID_GROUP) return false;
		g = Group::Get(g->parent);
	}
}

/**
 * Move all vehicles of a company and vehicle type to DEFAULT_GROUP (no update of statistics)
 * And those groups are deleted
 * @see CmdBuildGroupsOfVehicleType
 */
void MoveVehiclesToVoidAndRemoveGroups(CompanyID company, VehicleType type)
{
	assert(company == _current_company);

	Vehicle *v;
	Company *c = Company::Get(company);
	FOR_ALL_VEHICLES(v) {
		if (v->type == type && v->owner == company) v->group_id = DEFAULT_GROUP;
	}

	Group *g;
	FOR_ALL_GROUPS(g) {
		if (g->owner == company && g->vehicle_type == type) {
			/* If we set an autoreplace for the group we delete, remove it. */
			if (company < MAX_COMPANIES) {
				EngineRenew *er;
				FOR_ALL_ENGINE_RENEWS(er) {
					if (er->group_id == g->index) RemoveEngineReplacementForCompany(c, er->from, g->index, DC_EXEC);
				}
			}
			delete g;
		}
	}
}

/**
 * Decide if two vehicles should be in the same group
 * @param v first vehicle
 * @param v2 second vehicle
 * @param grouped_by_type criteria to be used
 * @return true if the two vehicles should be in the same group, false otherwise
 * @see CmdBuildGroupsOfVehicleType
 */
bool ShouldBeGrouped(Vehicle *v, Vehicle *v2, GroupedByType grouped_by_type)
{
	if (v == v2) return true;
	switch (grouped_by_type) {
		case GBT_ORDER_SIMPLE:
			return v->orders.list == v2->orders.list;
		case GBT_ORDER_STATIONS: {
			if (v2 != v2->FirstShared()) return false; //try this to speed up things
			if (v->orders.list == v2->orders.list) return true;
			if (v->orders.list == NULL || v2->orders.list == NULL) return false;
			SmallVector<DestinationID, 32> destinations_1 = GetDestinations(v->orders.list, 1);
			SmallVector<DestinationID, 32> destinations_2 = GetDestinations(v2->orders.list, 1);
			if (destinations_1.Length() != destinations_2.Length()) return false;
			for (uint i = destinations_1.Length(); i--;) {
				if (destinations_1[i] != destinations_2[i]) return false;
			}
			return true;
		}
		case GBT_CARGO:
			return GetCargoType(v) == GetCargoType(v2);
		case GBT_1ST_ENGINE_CLASS_CARGO:
			if (GetEngineLiveryScheme(v->engine_type, INVALID_ENGINE, v) != GetEngineLiveryScheme(v2->engine_type, INVALID_ENGINE, v2)) return false;
			return GetCargoType(v) == GetCargoType(v2);
		case GBT_1ST_ENGINE:
			return v->engine_type == v2->engine_type;
		default: NOT_REACHED();
	}
}

/**
 * Writes the cargo of a GroupStatistics to a string
 * @param buffer first position to write of the string
 * @param last last position of the string
 * @param stat GroupStatistics with the cargo
 * @see CmdBuildGroupsOfVehicleType
 */
void AddCargoString(char *buffer, char *last, GroupStatistics &stat)
{
	bool cargo_written = false;

	for (CargoID i = 0; i < NUM_CARGO && buffer < last; i++) {
		if (stat.DoesGroupCarryCargoType(i)) {
			/* Marker */
			if (cargo_written) buffer += seprintf(buffer, last, "/");
			else cargo_written = true;

			/* Write cargo */
			buffer = GetString(buffer, CargoSpec::Get(i)->name, last);
		}
	}

	if (!cargo_written) seprintf(buffer, last, "No cargo");
}

/**
 * Given a vehicle, add its destinations to a string
 * @param buffer first position to write of the string
 * @param last last position of the string_func
 * @param v vehicle which has the order_list with destinations to append
 * @see CmdBuildGroupsOfVehicleType
 */
void AddDestinationsString(char *buffer, char *last, Vehicle *v)
{
	SmallVector<DestinationID, 32> destinations = GetDestinations(v->orders.list, 1);
	bool is_first = true;

	for (uint i = 0; i < destinations.Length() && buffer < last; i++) {
		if (!is_first) buffer += seprintf(buffer, last, "/");
		else is_first = false;

		SetDParam(0, destinations[i]);
		buffer = GetString(buffer, STR_STATION_NAME, last);
	}
}

/**
 * Rebuilds groups for a vehicle type given a certain criteria.
 * @param tile unused
 * @param flags type of operation
 * @param p1 vehicle type
 * @param p2 how to group vehicles (@see #GroupedByType enum)
 * @param text unused
 */
CommandCost CmdBuildGroupsOfVehicleType(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleType vt = Extract<VehicleType, 0, 3>(p1);
	GroupedByType grouped_by_type = (GroupedByType)p2;

	if (!IsCompanyBuildableVehicleType(vt)) return CMD_ERROR;

	Vehicle *v, *v2;

	/* We must check if we can allocate enough groups.
	 * It can be done roughly as twice
	 * the number of different involved order lists.
	 * So we would need to allocate
	 * 2 * involved_order_lists - existing groups.
	 */
	uint used = GetGroupNumByVehicleType(_current_company, vt);
	uint needed = 0;
	FOR_ALL_VEHICLES(v) {
		if (v->owner == _current_company && v->type == vt &&
				v == v->FirstShared()) needed++;
	}

	if (!Group::CanAllocateItem(2 * needed - used)) return CMD_ERROR;
	if (!(flags & DC_EXEC)) return CommandCost();

	/* Delete all groups and move their vehicles to default group. */
	Window *w = FindWindowById(WC_REPLACE_VEHICLE, vt);
	if (w != NULL && w->owner == _current_company) delete w;
	MoveVehiclesToVoidAndRemoveGroups(_current_company, vt);

	/* Reset the ALL_GROUP statistics. */
	GroupStatistics::Get(_current_company, ALL_GROUP, vt).num_vehicle = 0;
	GroupStatistics::Get(_current_company, ALL_GROUP, vt).ClearProfits();

	uint i = 0; // Number of vehicles already checked.
	uint j = 0; // Number of already created groups.

	FOR_ALL_VEHICLES(v) {
		if (v->type != vt || v->owner != _current_company || v->group_id != DEFAULT_GROUP || !v->IsPrimaryVehicle()) {
			// These vehicles do not need a new group.
			i++;
			continue;
		}

		/* A new group for vehicle v is needed. */
		if (!Group::CanAllocateItem()) return CMD_ERROR;

		Group *g = new Group(_current_company);
		g->replace_protection = false;
		g->vehicle_type = vt;
		j++;

		FOR_ALL_VEHICLES_FROM(v2, i) {
			/* If v and v2 not in the same group, continue. */
			if (v2->type != vt || v2->owner != _current_company || v2->group_id != DEFAULT_GROUP || !v2->IsPrimaryVehicle() || !ShouldBeGrouped(v, v2, grouped_by_type)) continue;

			/* v and v2 must be grouped; to make things faster,
			 * if all shared vehicles must also be in the same group, add them;
			 * this way, we don't need to do unnecessary checks. */
			if (grouped_by_type == GBT_ORDER_SIMPLE || grouped_by_type == GBT_CARGO_ORDER || grouped_by_type == GBT_ORDER_STATIONS) {
				for (v2 = v2->FirstShared(); v2 != NULL; v2 = v2->NextShared())
					InsertVehicleToGroup(v2, g->index);
				if (grouped_by_type == GBT_ORDER_SIMPLE || grouped_by_type == GBT_CARGO_ORDER) break;
			} else {
				InsertVehicleToGroup(v2, g->index);
			}
		}

		/* Once we have all vehicles on group, update the cargo_types. */
		g->statistics.UpdateCargoTypes();

		/* Assign group name. */
		char new_name[MAX_LENGTH_GROUP_NAME_CHARS];
		char *buffer = new_name, *last = &new_name[MAX_LENGTH_GROUP_NAME_CHARS - 1];
		switch (grouped_by_type) {
			case GBT_ORDER_SIMPLE:
				SetDParam(0, j);
				buffer = GetString(buffer, STR_GROUP_SCHEDULE, last);
				AddCargoString(buffer, last, g->statistics);
				break;
			case GBT_ORDER_STATIONS:
				buffer += seprintf(buffer, last, "(%u) ", g->statistics.order_lists.Length());
				AddDestinationsString(buffer, last, v);
				break;
			case GBT_CARGO:
				AddCargoString(buffer, last, g->statistics);
				break;
			case GBT_1ST_ENGINE_CLASS_CARGO:
				buffer = GetString(buffer, STR_LIVERY_DEFAULT + GetEngineLiveryScheme(v->engine_type, INVALID_ENGINE, v), last);
				buffer += seprintf(buffer, last, ": ");
				AddCargoString(buffer, last, g->statistics);
				break;
			case GBT_1ST_ENGINE:
				SetDParam(0, v->engine_type);
				buffer = GetString(buffer, STR_ENGINE_NAME, last);
				break;
			case GBT_CARGO_ORDER:
				SetDParam(0, j);
				buffer = GetString(buffer, STR_GROUP_SCHEDULE, last);
				AddDestinationsString(buffer, last, v);
				break;
			default: NOT_REACHED();
		}

		if (IsUniqueGroupName(new_name, g->owner, g->vehicle_type)) {
			g->name = stredup(new_name);
		}

		i++;
	}

	/* Build tree for cargoes. */
	if (grouped_by_type == GBT_CARGO_ORDER) {
		SmallVector<GroupID, 2> involved_groups;
		Group *g;
		FOR_ALL_GROUPS(g) {
			if (g->owner != _current_company) continue;
			if (g->vehicle_type != vt) continue;
			*involved_groups.Append() = g->index;
		}

		while (involved_groups.Length() > 0) {
			if (!Group::CanAllocateItem()) return CMD_ERROR;
			Group *g_cargo = new Group(_current_company);
			g_cargo->replace_protection = false;
			g_cargo->vehicle_type = vt;
			CargoTypes cargoes = Group::Get(involved_groups[0])->statistics.cargo_types;

			/* Assign group name. */
			char new_name[MAX_LENGTH_GROUP_NAME_CHARS];
			char *buffer = new_name, *last = &new_name[MAX_LENGTH_GROUP_NAME_CHARS - 1];
			AddCargoString(buffer, last, Group::Get(involved_groups[0])->statistics);
			if (IsUniqueGroupName(new_name, _current_company, g_cargo->vehicle_type)) {
				g_cargo->name = stredup(new_name);
			}

			for (uint i = involved_groups.Length(); i--; ) {
				Group *iter = Group::Get(involved_groups[i]);
				if (iter->statistics.cargo_types != cargoes) continue;
				iter->parent = g_cargo->index;
				involved_groups.Erase(&involved_groups[i]);
			}
		}
	}

	/* Update DEFAULT_GROUP statistics. */
	GroupStatistics::Get(_current_company, DEFAULT_GROUP, vt).Clear();
	GroupStatistics::UpdateCargo(_current_company, ALL_GROUP, vt);

	/* Update and invalidate. */
	GroupStatistics::UpdateAutoreplace(_current_company);
	InvalidateWindowClassesData(GetWindowClassForVehicleType(vt), 0);
	SetWindowClassesDirty(WC_GROUP_DETAILS);

	return CommandCost();
}
