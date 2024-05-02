/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_cmd.cpp Handling of airport commands. */

#include "stdafx.h"
#include "command_func.h"
#include "core/backup_type.hpp"
#include "air.h"
#include "air_map.h"
#include "aircraft.h"
#include "airport_cmd.h"
#include "animated_tile_func.h"
#include "autoslope.h"
#include "company_base.h"
#include "company_gui.h"
#include "depot_base.h"
#include "industry.h"
#include "landscape.h"
#include "landscape_cmd.h"
#include "newgrf_debug.h"
#include "newgrf_airport.h"
#include "newgrf_airporttiles.h"
#include "order_backup.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "sound_func.h"
#include "station_func.h"
#include "station_kdtree.h"
#include "strings_func.h"
#include "town.h"
#include "vehicle_func.h"
#include "viewport_func.h"
#include "water.h"
#include "window_func.h"

#include "table/airporttile_ids.h"
#include "table/airport_defaults.h"
#include "table/airtypes.h"
#include "table/strings.h"
#include "table/track_land.h"

#include "widgets/station_widget.h"

extern CommandCost CheckBuildableTile(TileIndex tile, uint invalid_dirs, int &allowed_z, bool allow_steep, bool check_bridge = true);

extern CommandCost FindJoiningAirport(StationID existing_station, StationID station_to_join, bool adjacent, TileArea ta, Station **st);

extern CommandCost BuildStationPart(Station **st, DoCommandFlag flags, bool reuse, TileArea area, StationNaming name_class);

/**
 * Get a possible noise reduction factor based on distance from town center.
 * The further you get, the less noise you generate.
 * So all those folks at city council can now happily slee...  work in their offices
 * @param as airport information
 * @param distance minimum distance between town and airport
 * @return the noise that will be generated, according to distance
 */
uint8_t GetAirportNoiseLevelForDistance(const AirportSpec *as, uint distance)
{
	/* 0 cannot be accounted, and 1 is the lowest that can be reduced from town.
	 * So no need to go any further*/
	if (as->noise_level < 2) return as->noise_level;

	/* The steps for measuring noise reduction are based on the "magical" (and arbitrary) 8 base distance
	 * adding the town_council_tolerance 4 times, as a way to graduate, depending of the tolerance.
	 * Basically, it says that the less tolerant a town is, the bigger the distance before
	 * an actual decrease can be granted */
	uint8_t town_tolerance_distance = 8 + (_settings_game.difficulty.town_council_tolerance * 4);

	/* now, we want to have the distance segmented using the distance judged bareable by town
	 * This will give us the coefficient of reduction the distance provides. */
	uint noise_reduction = distance / town_tolerance_distance;

	/* If the noise reduction equals the airport noise itself, don't give it for free.
	 * Otherwise, simply reduce the airport's level. */
	return noise_reduction >= as->noise_level ? 1 : as->noise_level - noise_reduction;
}

/**
 * Finds the town nearest to given airport. Based on minimal manhattan distance to any airport's tile.
 * If two towns have the same distance, town with lower index is returned.
 * @param as airport's description
 * @param tile origin tile (top corner of the airport)
 * @param it An iterator over all airport tiles
 * @param[out] mindist Minimum distance to town
 * @return nearest town to airport
 */
Town *AirportGetNearestTown(const AirportSpec *as, TileIndex tile, const TileIterator &&it, uint &mindist)
{
	assert(Town::GetNumItems() > 0);

	Town *nearest = nullptr;

	auto width = as->size_x;
	auto height = as->size_y;
	if (rotation == DIR_E || rotation == DIR_W) std::swap(width, height);

	uint perimeter_min_x = TileX(tile);
	uint perimeter_min_y = TileY(tile);
	uint perimeter_max_x = perimeter_min_x + width - 1;
	uint perimeter_max_y = perimeter_min_y + height - 1;

	mindist = UINT_MAX - 1; // prevent overflow

	for (TileIndex cur_tile = *it; cur_tile != INVALID_TILE; cur_tile = ++it) {
		assert(IsInsideBS(TileX(cur_tile), perimeter_min_x, width));
		assert(IsInsideBS(TileY(cur_tile), perimeter_min_y, height));
		if (TileX(cur_tile) == perimeter_min_x || TileX(cur_tile) == perimeter_max_x || TileY(cur_tile) == perimeter_min_y || TileY(cur_tile) == perimeter_max_y) {
			Town *t = CalcClosestTownFromTile(cur_tile, mindist + 1);
			if (t == nullptr) continue;

			uint dist = DistanceManhattan(t->xy, cur_tile);
			if (dist == mindist && t->index < nearest->index) nearest = t;
			if (dist < mindist) {
				nearest = t;
				mindist = dist;
			}
		}
	}

	return nearest;
}


/** Recalculate the noise generated by the airports of each town */
void UpdateAirportsNoise()
{
	for (Town *t : Town::Iterate()) t->noise_reached = 0;

	for (const Station *st : Station::Iterate()) {
		if (st->airport.tile != INVALID_TILE && st->airport.type != AT_OILRIG) {
			const AirportSpec *as = st->airport.GetSpec();
			AirportTileIterator it(st);
			uint dist;
			Town *nearest = AirportGetNearestTown(as, st->airport.tile, it, dist);
			nearest->noise_reached += GetAirportNoiseLevelForDistance(as, dist);
		}
	}
}

/**
 * Checks if an airport can be built at the given location and clear the area.
 * @param tile_iter Airport tile iterator.
 * @param flags Operation to perform.
 * @return The cost in case of success, or an error code if it failed.
 */
static CommandCost CheckFlatLandAirport(AirportTileTableIterator tile_iter, DoCommandFlag flags)
{
	CommandCost cost(EXPENSES_CONSTRUCTION);
	int allowed_z = -1;

	for (; tile_iter != INVALID_TILE; ++tile_iter) {
		CommandCost ret = CheckBuildableTile(tile_iter, 0, allowed_z, true);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);

		ret = Command<CMD_LANDSCAPE_CLEAR>::Do(flags, tile_iter);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);
	}

	return cost;
}

/**
 * Place an Airport.
 * @param flags operation to perform
 * @param tile tile where airport will be built
 * @param airport_type airport type, @see airport.h
 * @param layout airport layout
 * @param station_to_join station ID to join (NEW_STATION if build new one)
- * @param allow_adjacent allow airports directly adjacent to other airports.
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildAirport(DoCommandFlag flags, TileIndex tile, uint8_t airport_type, uint8_t layout, StationID station_to_join, bool allow_adjacent)
{
	bool reuse = (station_to_join != NEW_STATION);
	if (!reuse) station_to_join = INVALID_STATION;
	bool distant_join = (station_to_join != INVALID_STATION);

	if (distant_join && (!_settings_game.station.distant_join_stations || !Station::IsValidID(station_to_join))) return CMD_ERROR;

	if (airport_type >= NUM_AIRPORTS) return CMD_ERROR;

	CommandCost ret = CheckIfAuthorityAllowsNewStation(tile, flags);
	if (ret.Failed()) return ret;

	/* Check if a valid, buildable airport was chosen for construction */
	const AirportSpec *as = AirportSpec::Get(airport_type);

	if (_description_airport_hangars[airport_type] && !Depot::CanAllocateItem()) return CMD_ERROR;
	if (!as->IsAvailable() || layout >= as->num_table) return CMD_ERROR;
	if (!as->IsWithinMapBounds(layout, tile)) return CMD_ERROR;

	Direction rotation = as->rotation[layout];
	int w = as->size_x;
	int h = as->size_y;
	if (rotation == DIR_E || rotation == DIR_W) Swap(w, h);
	TileArea airport_area = TileArea(tile, w, h);

	if (w > _settings_game.station.station_spread || h > _settings_game.station.station_spread) {
		return_cmd_error(STR_ERROR_STATION_TOO_SPREAD_OUT);
	}

	AirportTileTableIterator iter(as->table[layout], tile);
	CommandCost cost = CheckFlatLandAirport(iter, flags);
	if (cost.Failed()) return cost;

	/* The noise level is the noise from the airport and reduce it to account for the distance to the town center. */
	uint dist;
	Town *nearest = AirportGetNearestTown(as, tile, iter, dist);
	uint newnoise_level = GetAirportNoiseLevelForDistance(as, dist);

	/* Check if local auth would allow a new airport */
	StringID authority_refuse_message = STR_NULL;
	Town *authority_refuse_town = nullptr;

	if (_settings_game.economy.station_noise_level) {
		/* do not allow to build a new airport if this raise the town noise over the maximum allowed by town */
		if ((nearest->noise_reached + newnoise_level) > nearest->MaxTownNoise()) {
			authority_refuse_message = STR_ERROR_LOCAL_AUTHORITY_REFUSES_NOISE;
			authority_refuse_town = nearest;
		}
	} else if (_settings_game.difficulty.town_council_tolerance != TOWN_COUNCIL_PERMISSIVE) {
		Town *t = ClosestTownFromTile(tile, UINT_MAX);
		uint num = 0;
		for (const Station *st : Station::Iterate()) {
			if (st->town == t && (st->facilities & FACIL_AIRPORT) && st->airport.type != AT_OILRIG) num++;
		}
		if (num >= 2) {
			authority_refuse_message = STR_ERROR_LOCAL_AUTHORITY_REFUSES_AIRPORT;
			authority_refuse_town = t;
		}
	}

	if (authority_refuse_message != STR_NULL) {
		SetDParam(0, authority_refuse_town->index);
		return_cmd_error(authority_refuse_message);
	}

	Station *st = nullptr;
	ret = FindJoiningAirport(INVALID_STATION, station_to_join, allow_adjacent, airport_area, &st);
	if (ret.Failed()) return ret;

	/* Distant join */
	if (st == nullptr && distant_join) st = Station::GetIfValid(station_to_join);

	ret = BuildStationPart(&st, flags, reuse, airport_area, (GetAirport(airport_type)->flags & AirportFTAClass::AIRPLANES) ? STATIONNAMING_AIRPORT : STATIONNAMING_HELIPORT);
	if (ret.Failed()) return ret;

	if (st != nullptr && st->airport.tile != INVALID_TILE) {
		return_cmd_error(STR_ERROR_TOO_CLOSE_TO_ANOTHER_AIRPORT);
	}

	for (AirportTileTableIterator iter(as->table[layout], tile); iter != INVALID_TILE; ++iter) {
		cost.AddCost(_price[PR_BUILD_STATION_AIRPORT]);
	}

	if (flags & DC_EXEC) {
		/* Always add the noise, so there will be no need to recalculate when option toggles */
		nearest->noise_reached += newnoise_level;

		st->AddFacility(FACIL_AIRPORT, tile);
		st->airport.type = airport_type;
		st->airport.layout = layout;
		st->airport.flags = 0;
		st->airport.rotation = rotation;

		st->rect.BeforeAddRect(tile, w, h, StationRect::ADD_TRY);

		for (AirportTileTableIterator iter(as->table[layout], tile); iter != INVALID_TILE; ++iter) {
			MakeAirport(iter, st->owner, st->index, iter.GetStationGfx(), WATER_CLASS_INVALID);
			SetStationTileRandomBits(iter, GB(Random(), 0, 4));
			st->airport.Add(iter);
		}

		st->UpdateToMultitileAirport();

		for (AirportTileTableIterator iter(as->table[layout], tile); iter != INVALID_TILE; ++iter) {
			if (AirportTileSpec::Get(GetTranslatedAirportTileID(iter.GetStationGfx()))->animation.status != ANIM_STATUS_NO_ANIMATION) AddAnimatedTile(iter);
		}

		/* Only call the animation trigger after all tiles have been built */
		for (AirportTileTableIterator iter(as->table[layout], tile); iter != INVALID_TILE; ++iter) {
			AirportTileAnimationTrigger(st, iter, AAT_BUILT);
		}

		UpdateAirplanesOnNewStation(st);

		Company::Get(st->owner)->infrastructure.airport++;

		st->AfterStationTileSetChange(true, STATION_AIRPORT);
		InvalidateWindowData(WC_STATION_VIEW, st->index, -1);

		if (_settings_game.economy.station_noise_level) {
			SetWindowDirty(WC_TOWN_VIEW, nearest->index);
		}
	}

	return cost;
}

/**
 * Remove an airport
 * @param tile TileIndex been queried
 * @param flags operation to perform
 * @return cost or failure of operation
 */
CommandCost RemoveAirport(TileIndex tile, DoCommandFlag flags)
{
	Station *st = Station::GetByTile(tile);

	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckOwnership(st->owner);
		if (ret.Failed()) return ret;
	}

	tile = st->airport.tile;

	CommandCost cost(EXPENSES_CONSTRUCTION);

	for (const Aircraft *a : Aircraft::Iterate()) {
		if (!a->IsNormalAircraft()) continue;
		if (a->targetairport == st->index && a->state != FLYING) {
			return_cmd_error(STR_ERROR_AIRCRAFT_IN_THE_WAY);
		}
	}

	if (flags & DC_EXEC) {
		CloseWindowById(WC_VEHICLE_DEPOT, st->airport.depot_id);
		if (st->airport.depot_id != INVALID_DEPOT) OrderBackup::Reset(st->airport.depot_id, false);

		const AirportSpec *as = st->airport.GetSpec();
		/* The noise level is the noise from the airport and reduce it to account for the distance to the town center.
		 * And as for construction, always remove it, even if the setting is not set, in order to avoid the
		 * need of recalculation */
		AirportTileIterator it(st);
		uint dist;
		Town *nearest = AirportGetNearestTown(as, st->airport.tile, it, dist);
		nearest->noise_reached -= GetAirportNoiseLevelForDistance(as, dist);

		if (_settings_game.economy.station_noise_level) {
			SetWindowDirty(WC_TOWN_VIEW, nearest->index);
		}
	}

	for (TileIndex tile_cur : st->airport) {
		if (!st->TileBelongsToAirport(tile_cur)) continue;

		CommandCost ret = EnsureNoVehicleOnGround(tile_cur);
		if (ret.Failed()) return ret;

		cost.AddCost(_price[PR_CLEAR_STATION_AIRPORT]);

		if (flags & DC_EXEC) {
			DeleteAnimatedTile(tile_cur);
			DoClearSquare(tile_cur);
			DeleteNewGRFInspectWindow(GSF_AIRPORTTILES, static_cast<uint32_t>(tile_cur));
		}
	}

	if (flags & DC_EXEC) {
		/* Clear the persistent storage. */
		delete st->airport.psa;

		st->rect.AfterRemoveRect(st, st->airport);

		st->UpdateAirportDataStructure();
		st->airport.Clear();
		st->facilities &= ~FACIL_AIRPORT;

		InvalidateWindowData(WC_STATION_VIEW, st->index, -1);

		Company::Get(st->owner)->infrastructure.airport--;

		st->AfterStationTileSetChange(false, STATION_AIRPORT);

		DeleteNewGRFInspectWindow(GSF_AIRPORTS, st->index);
	}

	return cost;
}

void BuildBuiltInHeliport(Tile tile)
{
	if (!Station::CanAllocateItem()) {
		Debug(misc, 0, "Can't allocate built in station for industry at 0x{}. Built in station won't be built.", static_cast<uint32_t>(tile));
		return;
	}

	Station *st = new Station(tile);
	_station_kdtree.Insert(st->index);
	st->town = ClosestTownFromTile(tile, UINT_MAX);

	st->string_id = GenerateStationName(st, tile, STATIONNAMING_OILRIG);

	assert(IsTileType(tile, MP_INDUSTRY));
	/* Mark industry as associated both ways */
	st->industry = Industry::GetByTile(tile);
	st->industry->neutral_station = st;
	DeleteAnimatedTile(tile);
	MakeStation(tile, OWNER_NONE, st->index, STATION_AIRPORT, 0, GetWaterClass(tile));

	const TileDescription *description = &_description_oilrig_0[0];
	SetAirType(tile, description->ground);
	SetAirportTileType(tile, description->type);
	tile.m4() = 0;
	tile.m5() = 0;
	SetAirportTileTracks(tile, description->trackbits);
	SetTerminalType(tile, description->terminal_type);
	st->airport.flags = 0;

	st->owner = OWNER_NONE;
	st->airport.type = AT_OILRIG;
	st->airport.Add(tile);
	st->ship_station.Add(tile);
	st->facilities = FACIL_AIRPORT | FACIL_DOCK;
	st->build_date = TimerGameCalendar::date;
	st->UpdateAirportDataStructure();
	UpdateStationDockingTiles(st);

	st->rect.BeforeAddTile(tile, StationRect::ADD_FORCE);

	st->UpdateVirtCoord();

	/* An industry tile has now been replaced with a station tile, this may change the overlap between station catchments and industry tiles.
	 * Recalculate the station catchment for all stations currently in the industry's nearby list.
	 * Clear the industry's station nearby list first because Station::RecomputeCatchment cannot remove nearby industries in this case. */
	if (_settings_game.station.serve_neutral_industries) {
		StationList nearby = std::move(st->industry->stations_near);
		st->industry->stations_near.clear();
		for (Station *near : nearby) {
			near->RecomputeCatchment(true);
			UpdateStationAcceptance(near, true);
		}
	}

	st->RecomputeCatchment();
	UpdateStationAcceptance(st, false);
}

void DeleteBuiltInHeliport(TileIndex tile)
{
	Station *st = Station::GetByTile(tile);

	MakeWaterKeepingClass(tile, OWNER_NONE);

	/* The oil rig station is not supposed to be shared with anything else */
	assert(st->facilities == (FACIL_AIRPORT | FACIL_DOCK) && st->airport.type == AT_OILRIG);
	if (st->industry != nullptr && st->industry->neutral_station == st) {
		/* Don't leave dangling neutral station pointer */
		st->industry->neutral_station = nullptr;
	}
	delete st;
}

/**
 * Open/close an airport to incoming aircraft.
 * @param flags Operation to perform.
 * @param station_id Station ID of the airport.
 * @return the cost of this operation or an error
 */
CommandCost CmdOpenCloseAirport(DoCommandFlag flags, StationID station_id)
{
	if (!Station::IsValidID(station_id)) return CMD_ERROR;
	Station *st = Station::Get(station_id);

	if (!(st->facilities & FACIL_AIRPORT) || st->owner == OWNER_NONE) return CMD_ERROR;

	CommandCost ret = CheckOwnership(st->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		st->airport.flags ^= AIRPORT_CLOSED_block;
		SetWindowWidgetDirty(WC_STATION_VIEW, st->index, WID_SV_CLOSE_AIRPORT);
	}
	return CommandCost();
}
