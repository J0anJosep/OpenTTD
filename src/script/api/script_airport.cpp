/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_airport.cpp Implementation of ScriptAirport. */

#include "../../stdafx.h"
#include "script_airport.hpp"
#include "script_station.hpp"
#include "../../station_base.h"
#include "../../town.h"
#include "../../landscape_cmd.h"
#include "../../station_cmd.h"
#include "../../airport_cmd.h"
#include "../../depot_base.h"
#include "../../air.h"

#include "../../safeguards.h"

/* static */ bool ScriptAirport::IsValidAirportType(AirportType type)
{
	return IsAirportInformationAvailable(type) && ::AirportSpec::Get(type)->IsAvailable();
}

/* static */ bool ScriptAirport::IsAirportInformationAvailable(AirportType type)
{
	return type >= 0 && type < (AirportType)NUM_AIRPORTS && AirportSpec::Get(type)->enabled;
}

/* static */ Money ScriptAirport::GetPrice(AirportType type)
{
	if (!IsValidAirportType(type)) return -1;

	const AirportSpec *as = ::AirportSpec::Get(type);
	return _price[PR_BUILD_STATION_AIRPORT] * as->layouts[0].size_x * as->layouts[0].size_y;
}

/* static */ bool ScriptAirport::IsHangarTile(TileIndex tile)
{
	if (!::IsValidTile(tile)) return false;

	return ::IsTileType(tile, MP_STATION) && ::IsAirport(tile) && ::IsHangar(tile);
}

/* static */ bool ScriptAirport::IsAirportTile(TileIndex tile)
{
	if (!::IsValidTile(tile)) return false;

	return ::IsTileType(tile, MP_STATION) && ::IsAirport(tile);
}

/* static */ SQInteger ScriptAirport::GetAirportWidth(AirportType type)
{
	if (!IsAirportInformationAvailable(type)) return -1;

	return ::AirportSpec::Get(type)->layouts[0].size_x;
}

/* static */ SQInteger ScriptAirport::GetAirportHeight(AirportType type)
{
	if (!IsAirportInformationAvailable(type)) return -1;

	return ::AirportSpec::Get(type)->layouts[0].size_y;
}

/* static */ SQInteger ScriptAirport::GetAirportCoverageRadius(AirportType type)
{
	if (!IsAirportInformationAvailable(type)) return -1;

	const AirportSpec *as = ::AirportSpec::Get(type);
	const AirTypeInfo *ati = GetAirTypeInfo(as->airtype);
	assert(ati->catchment_radius <= MAX_CATCHMENT);

	return _settings_game.station.modified_catchment ? ati->catchment_radius : (uint)CA_UNMODIFIED;
}

/* static */ bool ScriptAirport::BuildAirport(TileIndex tile, AirportType type, StationID station_id)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, ::IsValidTile(tile));
	EnforcePrecondition(false, IsValidAirportType(type));
	EnforcePrecondition(false, station_id == ScriptStation::STATION_NEW || station_id == ScriptStation::STATION_JOIN_ADJACENT || ScriptStation::IsValidStation(station_id));

	return ScriptObject::Command<CMD_BUILD_AIRPORT>::Do(tile, type, 0, INVALID_AIRTYPE, DIAGDIR_NE, (ScriptStation::IsValidStation(station_id) ? station_id : INVALID_STATION), station_id != ScriptStation::STATION_JOIN_ADJACENT);
}

/* static */ bool ScriptAirport::RemoveAirport(TileIndex tile)
{
	EnforceCompanyModeValid(false);
	EnforcePrecondition(false, ::IsValidTile(tile))
	EnforcePrecondition(false, IsAirportTile(tile) || IsHangarTile(tile));

	return ScriptObject::Command<CMD_LANDSCAPE_CLEAR>::Do(tile);
}

/* static */ SQInteger ScriptAirport::GetNumHangars(TileIndex tile)
{
	EnforceDeityOrCompanyModeValid(-1);
	if (!::IsValidTile(tile)) return -1;
	if (!::IsTileType(tile, MP_STATION)) return -1;

	const Station *st = ::Station::GetByTile(tile);
	if (st->owner != ScriptObject::GetCompany() && ScriptCompanyMode::IsValid()) return -1;
	if ((st->facilities & FACIL_AIRPORT) == 0) return -1;

	if (!st->airport.HasHangar()) return 0;

	return (int32_t)st->airport.hangar->depot_tiles.size();
}

/* static */ TileIndex ScriptAirport::GetHangarOfAirport(TileIndex tile)
{
	EnforceDeityOrCompanyModeValid(INVALID_TILE);
	if (!::IsValidTile(tile)) return INVALID_TILE;
	if (!::IsTileType(tile, MP_STATION)) return INVALID_TILE;

	const Station *st = ::Station::GetByTile(tile);
	if (st->owner != ScriptObject::GetCompany() && ScriptCompanyMode::IsValid()) return INVALID_TILE;
	if ((st->facilities & FACIL_AIRPORT) == 0) return INVALID_TILE;
	if (st->airport.hangar == nullptr) return INVALID_TILE;

	return st->airport.hangar->depot_tiles[0];
}

/* static */ ScriptAirport::AirportType ScriptAirport::GetAirportType(TileIndex tile)
{
	if (!ScriptTile::IsStationTile(tile)) return AT_INVALID;

	StationID station_id = ::GetStationIndex(tile);

	if (!ScriptStation::HasStationType(station_id, ScriptStation::STATION_AIRPORT)) return AT_INVALID;

	return (AirportType)::Station::Get(station_id)->airport.type;
}

extern CommandCost AddAirportTileTableToBitmapTileArea(const AirportTileLayout &atl, BitmapTileArea *bta, DiagDirection rotation, uint cost_multiplier);
extern Town *AirportGetNearestTown(BitmapTileArea bta, uint &mindist);
extern uint8_t GetAirportNoiseLevelForDistance(uint noise_level, uint distance);

/* static */ SQInteger ScriptAirport::GetNoiseLevelIncrease(TileIndex tile, AirportType type)
{
	if (!::IsValidTile(tile)) return -1;
	if (!IsAirportInformationAvailable(type)) return -1;

	const AirportSpec *as = ::AirportSpec::Get(type);
	if (!as->IsWithinMapBounds(0, tile, 0)) return -1;

	if (_settings_game.economy.station_noise_level) {
		BitmapTileArea bta;
		const AirTypeInfo *ati = GetAirTypeInfo(as->airtype);
		TileArea ta(tile, as->layouts[0].size_x, as->layouts[0].size_y);
		AddAirportTileTableToBitmapTileArea(as->layouts[0], &bta, DIAGDIR_BEGIN, ati->cost_multiplier);
		uint dist;

		AirportGetNearestTown(bta, dist);
		return GetAirportNoiseLevelForDistance(as->GetAirportNoise(as->airtype), dist);
	}

	return 1;
}

/* static */ TownID ScriptAirport::GetNearestTown(TileIndex tile, AirportType type)
{
	if (!::IsValidTile(tile)) return INVALID_TOWN;
	if (!IsAirportInformationAvailable(type)) return INVALID_TOWN;

	const AirportSpec *as = AirportSpec::Get(type);
	if (!as->IsWithinMapBounds(0, tile, 0)) return INVALID_TOWN;

	BitmapTileArea bta;
	const AirTypeInfo *ati = GetAirTypeInfo(as->airtype);
	TileArea ta(tile, as->layouts[0].size_x, as->layouts[0].size_y);
	AddAirportTileTableToBitmapTileArea(as->layouts[0], &bta, DIAGDIR_BEGIN, ati->cost_multiplier);
	uint dist;
	return AirportGetNearestTown(bta, dist)->index;
}

/* static */ SQInteger ScriptAirport::GetAirportNumHelipads(AirportType type)
{
	if (!IsAirportInformationAvailable(type)) return -1;

	return ::AirportSpec::Get(type)->num_helipads;
}
