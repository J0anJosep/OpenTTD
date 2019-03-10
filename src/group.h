/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file group.h Base class for groups and group functions. */

#ifndef GROUP_H
#define GROUP_H

#include "group_type.h"
#include "core/pool_type.hpp"
#include "company_type.h"
#include "vehicle_type.h"
#include "engine_type.h"
#include "livery.h"
#include "order_type.h"
#include "widgets/dropdown_type.h"

typedef Pool<Group, GroupID, 16, 64000> GroupPool;
typedef SmallVector<const OrderList *, 32> OrderListList;
extern GroupPool _group_pool; ///< Pool of groups.

/** Statistics and caches on the vehicles in a group. */
struct GroupStatistics {
	uint16 num_vehicle;                     ///< Number of vehicles.
	uint16 *num_engines;                    ///< Caches the number of engines of each type the company owns.

	bool autoreplace_defined;               ///< Are any autoreplace rules set?
	bool autoreplace_finished;              ///< Have all autoreplacement finished?

	/* Profit information */
	uint16 num_profit_vehicle;              ///< Number of vehicles considered for profit statistics;
	Money profit_last_year;                 ///< Sum of profits for all vehicles.
	Money min_profit_vehicle;               ///< The min of the profits of the vehicles of this group last year; it determines the group profit icon

	/* Timetable information */
	OrderListType ol_type;                  ///< type of orders/timetables this group has

	/* Order Lists of vehicles of this group */
	OrderListList order_lists;              ///< all different OrderLists of the group

	/* Cargo information */
	CargoTypes cargo_types;                 ///< cargo types this group carries (also refit orders)
	CargoArray act_cargo;                   ///< cargo carried
	CargoArray max_cargo;                   ///< total cargo capacity


	GroupStatistics();
	~GroupStatistics();

	void Clear();

	void ClearProfits()
	{
		this->num_profit_vehicle = 0;
		this->profit_last_year = 0;
		this->min_profit_vehicle = INT64_MAX;
	}

	void ClearAutoreplace()
	{
		this->autoreplace_defined = false;
		this->autoreplace_finished = false;
	}

	void ClearCargo()
	{
		this->act_cargo.Clear();
		this->max_cargo.Clear();
		cargo_types = 0;
	}

	static GroupStatistics &Get(CompanyID company, GroupID id_g, VehicleType type);
	static GroupStatistics &Get(const Vehicle *v);
	static GroupStatistics &GetAllGroup(const Vehicle *v);

	static void CountVehicle(const Vehicle *v, int delta);
	static void CountEngine(const Vehicle *v, int delta);
	static void VehicleReachedProfitAge(const Vehicle *v, int delta);

	static void UpdateProfits();
	static void UpdateAfterLoad();
	static void UpdateAutoreplace(CompanyID company);
	static void UpdateCargo(CompanyID company, GroupID id_g, VehicleType type);
	static void UpdateCargoForVehicleType(CompanyID company, VehicleType type);

	/* Update info */
	void AddOrderListType(const OrderListType new_ol);
	void RemoveOrderListType(const Vehicle *vehicle_to_take_out);
	void UpdateMinProfit(const Vehicle *vehicle_to_take_out, const GroupID g_id);
	void RemoveOrderList(const Vehicle *v);
	void UpdateCargoTypes();

	/* Return objects */
	DropDownList *BuildSharedOrdersDropdown() const;
	bool HasGroupOrderListType(OrderListType ol_type) const;
	bool DoesGroupCarryCargoType(CargoID cargo) const;
	SpriteID SetGroupProfitSpriteID() const;
};

/** Group data. */
struct Group : GroupPool::PoolItem<&_group_pool> {
	char *name;                             ///< Group Name
	OwnerByte owner;                        ///< Group Owner
	VehicleTypeByte vehicle_type;           ///< Vehicle type of the group

	bool replace_protection;                ///< If set to true, the global autoreplace have no effect on the group
	Livery livery;                          ///< Custom colour scheme for vehicles in this group
	GroupStatistics statistics;             ///< NOSAVE: Statistics and caches on the vehicles in the group.

	GroupID parent;                         ///< Parent group

	Group(CompanyID owner = INVALID_COMPANY);
	~Group();

	const Vehicle *GetVehicleOfGroup() const;
};


static inline bool IsDefaultGroupID(GroupID index)
{
	return index == DEFAULT_GROUP;
}

/**
 * Checks if a GroupID stands for all vehicles of a company
 * @param id_g The GroupID to check
 * @return true is id_g is identical to ALL_GROUP
 */
static inline bool IsAllGroupID(GroupID id_g)
{
	return id_g == ALL_GROUP;
}

#define FOR_ALL_GROUPS_FROM(var, start) FOR_ALL_ITEMS_FROM(Group, group_index, var, start)
#define FOR_ALL_GROUPS(var) FOR_ALL_GROUPS_FROM(var, 0)


uint GetGroupNumEngines(CompanyID company, GroupID id_g, EngineID id_e);

void SetTrainGroupID(Train *v, GroupID grp);
void UpdateTrainGroupID(Train *v);
void RemoveVehicleFromGroup(const Vehicle *v);
void RemoveAllGroupsForCompany(const CompanyID company);
bool GroupIsInGroup(GroupID search, GroupID group);
bool GroupInheritsFromGroup(GroupID search, GroupID group);
void MoveVehiclesToVoidAndRemoveGroups(CompanyID company, VehicleType type);
bool AreGroupsAutoManaged(VehicleType veh_type, CompanyID company);

void DispatchAutoGroupByCompaniesMerged(CompanyID company_id);
void DispatchAutoGroupByNewVehicle(VehicleID v_id); //done
void DispatchAutoGroupByVehicleReplaced(VehicleID v_id);
void DispatchAutoGroupByVehicleDeleted(VehicleID v_id); //done
void DispatchAutoGroupByVehicleRefitted(Vehicle v_id); //done
void DispatchAutoGroupByOrdersChanged(VehicleID v_id);
void DispatchAutoGroupByOrdersDeleted(VehicleID v_id);

extern GroupID _new_group_id;

#endif /* GROUP_H */
