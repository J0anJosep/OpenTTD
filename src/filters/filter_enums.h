/** @file filter_enums.h Enumerations related to the filters. */

#ifndef FILTER_ENUMS_H
#define FILTER_ENUMS_H

/** Status an item on filter may have: inactive/positive/negative. */
enum ItemStatus {
	IS_BEGIN = 0,
	IS_NEUTRAL = 0,
	IS_POSITIVE,
	IS_NEGATIVE,
	IS_END
};
DECLARE_POSTFIX_INCREMENT(ItemStatus)


/**
 * Set of available default lists of items.
 * @note this enum provides in which list are towns stored, where are companies stored, etc.
 */
enum ListType {
	LT_BEGIN = 0,
	LT_TOWNS = LT_BEGIN,
	LT_COMPANIES,
	LT_CARGO, /* on catchment area window, it is cargo production. */
	LT_CARGO_ACCEPTANCE,
	LT_STATION_FACILITIES,
	LT_STATIONS,
	LT_ORDERLIST,

	/* For each specific filter, we may add a specific list of items. */
	LT_VEHICLE_GROUP_PROPERTIES,
	LT_CATCHMENT_AREA_PROPERTIES,
	LT_TOWN_PROPERTIES,

	LT_END,
};

enum StationFacilities {
	SF_BEGIN = 0,
	SF_RAIL = SF_BEGIN,
	SF_TRUCK,
	SF_BUS,
	SF_PLANE,
	SF_SHIP,
	SF_END,
};

/** Vehicle/Group properties used for filtering. */
enum VehicleGroupProperties {
	VGP_BEGIN,
	VGP_STOPPED = VGP_BEGIN,
	VGP_STOPPED_IN_DEPOT,
	VGP_CRASHED,
	VGP_PATHFINDER_LOST,
	VGP_OLD_VEHICLE,
	VGP_NEW_VEHICLE,
	VGP_NEGATIVE_PROFIT,
	VGP_MAKING_GOOD_PROFIT,
	VGP_END
};

/** Catchment area properties available. */
enum CatchmentAreaProperties {
	ZW_BEGIN,
	ZW_SHOW_ALL_UNCAUGHT_TILES = ZW_BEGIN,
	ZW_SHOW_UNCAUGHT_BUILDINGS,
	ZW_SHOW_UNCAUGHT_INDUSTRIES,
	ZW_END
};

/** Town properties. */
enum TownProperties {
	TP_BEGIN,
	TP_LOCAL_COMPANY_HAS_STATUE = TP_BEGIN,
	TP_END
};

#endif /* FILTER_ENUMS_H */