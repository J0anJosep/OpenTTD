/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rail_cmd.cpp Handling of rail tiles. */

#include "stdafx.h"
#include "cmd_helper.h"
#include "viewport_func.h"
#include "command_func.h"
#include "depot_base.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "newgrf_debug.h"
#include "air.h"
#include "aircraft.h"
#include "autoslope.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "town.h"
#include "pbs_air.h"
#include "company_base.h"
#include "core/backup_type.hpp"
#include "date_func.h"
#include "strings_func.h"
#include "company_gui.h"
#include "newgrf_airtype.h"
#include "station_base.h"
#include "bridge_map.h"
#include "zoning.h"
#include "window_func.h"
#include "landscape.h"
#include "animated_tile_func.h"
#include "newgrf_airporttiles.h"
#include "order_backup.h"
#include "water_map.h"

#include "table/strings.h"
#include "table/airtypes.h"
#include "table/track_land.h"


/** Helper type for lists/vectors of trains */
typedef SmallVector<Aircraft *, 16> AircraftList;

AirTypeInfo _airtypes[AIRTYPE_END];

CommandCost inline CheckSettingBuildByTile()
{
	if (!_settings_game.station.allow_modify_airports) return_cmd_error(STR_ERROR_AIRPORT_DISABLED_BY_TILE);
	return CommandCost();
}

void ResolveAirTypeGUISprites(AirTypeInfo *ati)
{
	SpriteID cursors_base = GetCustomAirSprite(ati, INVALID_TILE, ATSG_CURSORS);
	if (cursors_base != 0) {
		SpriteID *sprite_id_end = &ati->cursor.convert_air;
		for (SpriteID *sprite_id_seq = &ati->gui_sprites.build_infrastructure;
		     sprite_id_seq < sprite_id_end; sprite_id_seq++) *sprite_id_seq = cursors_base++;
	}
}

/**
 * Reset all air type information to its default values.
 */
void ResetAirTypes()
{
	assert_compile(lengthof(_original_airtypes) <= lengthof(_airtypes));

	uint i = 0;
	for (; i < lengthof(_original_airtypes); i++) _airtypes[i] = _original_airtypes[i];

	static const AirTypeInfo empty_airtype = {
		{0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0},
		{0,0,0,0},
		0, AIRTYPES_NONE, AIRTYPES_NONE, 0, 0, 0, 0, 0, 0, AirTypeLabelList(), 0, 0,
		AIRTYPES_NONE, AIRTYPES_NONE, 0,
		{}, {}, 0, 0, 0, 0, 0 };
	for (; i < lengthof(_airtypes);          i++) _airtypes[i] = empty_airtype;
}

/**
 * Resolve sprites of custom air types
 */
void InitAirTypes()
{
	for (AirType at = AIRTYPE_BEGIN; at != AIRTYPE_END; at++) {
		AirTypeInfo *ati = &_airtypes[at];
		ResolveAirTypeGUISprites(ati);
	}
}

/**
 * Allocate a new air type label
 */
AirType AllocateAirType(AirTypeLabel label)
{
	for (AirType at = AIRTYPE_BEGIN; at != AIRTYPE_END; at++) {
		AirTypeInfo *ati = &_airtypes[at];

		if (ati->label == 0) {
			/* Set up new air type */
			*ati = _original_airtypes[AIRTYPE_BEGIN];
			ati->label = label;
			ati->alternate_labels.Clear();

			/* Make us compatible with ourself. */
			ati->compatible_airtypes = (AirTypes)(1 << at);

			/* We also introduce ourself. */
			ati->introduces_airtypes = (AirTypes)(1 << at);

			/* Default sort order; order of allocation, but with some
			 * offsets so it's easier for NewGRF to pick a spot without
			 * changing the order of other (original) air types.
			 * The << is so you can place other airtypes in between the
			 * other airtypes, the 7 is to be able to place something
			 * before the first (default) air type. */
			ati->sorting_order = at << 4 | 7;
			return at;
		}
	}

	return INVALID_AIRTYPE;
}


bool Airport::HasLanding() const
{
	for (const TileIndex *tile = this->runways.Begin(); tile < this->runways.End(); tile++) {
		if (IsLandingTypeTile(*tile)) return true;
	}

	return false;
}


static const byte _track_sloped_sprites[14] = {
	14, 15, 22, 13,
	 0, 21, 17, 12,
	23,  0, 18, 20,
	19, 16
};


static const TileIndexDiffC _trackdelta[] = {
	{ -1,  0 }, {  0,  1 }, { -1,  0 }, {  0,  1 }, {  1,  0 }, {  0,  1 },
	{  0,  0 },
	{  0,  0 },
	{  1,  0 }, {  0, -1 }, {  0, -1 }, {  1,  0 }, {  0, -1 }, { -1,  0 },
	{  0,  0 },
	{  0,  0 }
};

extern CommandCost ValidateAutoDrag(Trackdir *trackdir, TileIndex start, TileIndex end);

/**
 * The p2 parameter is an uint32, containing this information, if those bits have a meaning.
 * - p2 = (bit   0      ) - 0 = Remove, 1 = Add
 * - p2 = (bit   1      ) - 0 = Rect, 1 = Diagonal
 *      0 = No adjacent station, 1 = Join adjacent station (_ctrl_pressed)
 * - p2 = (bit   2      ) - Multiple purporse:
 *                        - 0 for small hangar, 1 for big hangar
 *                        - infrastructure 1-with / 0-without catchment
 *                        - runway 0-forbidding / 1-allowing landing
 * - p2 = (bit   3      ) - unused
 * - p2 = (bits  4 -  6 ) - track-orientation, valid values: 0-5
 *                          or terminal type.
 * - p2 = (bits  7 -  8 ) - Direction (hangar).
 * - p2 = (bits  9 - 12 ) - AirType (gravel, asphalt, water,...)
 * - p2 = (bits 13 - 15 ) - AirportTileType (hangar, helipad, hangar,...)
 * - p2 = (bits 16 - 31 ) - StationID where to apply the command
 *                          (NEW_STATION for a new station).
 */

inline bool IsAddingAction(uint32 p2)            { return GB(p2, 0, 1); }
inline bool GetCtrlState(uint32 p2)              { return GB(p2, 1, 1); }
inline bool GetMultiPurposeBit(uint32 p2)        { return GB(p2, 2, 1); }
inline Track GetTrack(uint32 p2)                 { return Extract<Track, 4, 3>(p2); }
inline TerminalType GetTerminalTypeP2(uint32 p2) { return (TerminalType)GB(p2, 4, 3); }
inline DiagDirection GetDirection(uint32 p2)     { return (DiagDirection)GB(p2, 7, 2); }
inline AirType GetAirType(uint32 p2)             { return (AirType)GB(p2, 9, 4); }
inline AirportTileType GetAirTileType(uint32 p2) { return (AirportTileType)GB(p2, 13, 3); }
inline StationID GetStationID(uint32 p2)         { return (StationID)GB(p2, 16, 16); }

/**
 * Checks if there is a vehicle in an airport given by one of its tiles.
 * @param st Station to check.
 * @return A command cost with an error if a vehicle is found on ground or in a runway.
 */
CommandCost AircraftInAirport(const Station *st)
{
	if (st == NULL) return CommandCost();

	MASKED_TILE_AREA_LOOP(tile, st->airport, st->airport.footprint) {
		assert(st->TileBelongsToAirport(tile));

		if ((MayHaveAirTracks(tile) && HasAirportTrackReservation(tile)) ||
				(IsRunway(tile) && GetReservationAsRunway(tile)))
			return_cmd_error(STR_ERROR_AIRPORT_AIRCRAFT_ON_TILES);

		/* Aircraft can be hidden inside depots with no associated reservation. */
		if (IsSmallHangarTile(tile)) {
			CommandCost ret = EnsureFreeHangar(tile);
			if (ret.Failed()) return ret;
		}
	}

	return CommandCost();
}


CommandCost CheckRunwayLength(AirType air_type, uint length)
{
	if (GetAirTypeInfo(air_type)->min_runway_length > length) return_cmd_error(STR_ERROR_AIRPORT_RUNWAY_TOO_SHORT);
	return CommandCost();
}

CommandCost AddAirportTrack(TileIndex tile, Track track, DoCommandFlag flags)
{
	assert(IsAirportTile(tile) && !IsHangar(tile));

	if (!IsValidTile(tile)) return CMD_ERROR;
	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;
	if (!MayHaveAirTracks(tile)) return_cmd_error(STR_ERROR_AIRPORT_CANNOT_HAVE_TRACKS);
	if (HasAirportTileTrack(tile, track)) return_cmd_error(STR_ERROR_ALREADY_BUILT);
	AirportTileType att = GetAirportTileType(tile);
	if ((att == ATT_HANGAR) && !IsDiagonalTrack(track)) return_cmd_error(STR_ERROR_AIRPORT_CANNOT_ADD_TRACK_HANGAR);

	if ((GetAllowedTracks(tile) & TrackToTrackBits(track)) == TRACK_BIT_NONE) return_cmd_error(STR_ERROR_AIRPORT_NO_COMPATIBLE_NEIGHBOURS);
	if ((GetAirportTileTracks(tile) & TrackToTrackBits(track)) != TRACK_BIT_NONE) return_cmd_error(STR_ERROR_ALREADY_BUILT);

	//check allowed and return a proper cmd_error string

	// get associated tile

	if (flags & DC_EXEC) {
		SetAirportTileTracks(tile, GetAirportTileTracks(tile) | TrackToTrackBits(track));
		// if (tile != neighbour && HasAirportTrackReserved(neighbour, TrackToOppositeTrack(track))) {
		//	SetAirportTrackReservation(tile, track);
		//} reserve associated tile and track if needed
		MarkTileDirtyByTile(tile);
	}

	return CommandCost();
}

CommandCost RemoveAirportTrack(TileIndex tile, Track track, DoCommandFlag flags)
{
	assert(IsAirportTile(tile) && !IsHangar(tile));

	if (!IsValidTile(tile)) return CMD_ERROR;
	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	if (!MayHaveAirTracks(tile) || !HasAirportTileTrack(tile, track)) return CommandCost();

	if (flags & DC_EXEC) {
		SetAirportTileTracks(tile, GetAirportTileTracks(tile) & ~TrackToTrackBits(track));
		MarkTileDirtyByTile(tile);
	}

	return CommandCost();
}

/**
 * Add/Remove tracks for an airport.
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit   0      ) - 0 = Remove, 1 = Add
 * - p2 = (bits  4 -  6 ) - Starting track.
 * - p2 = (bits  8 - 11 ) - Air type (gravel, asphalt, water,...)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdAddRemoveTracksToAirport(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Track track = GetTrack(p2);
	AirType air_type = GetAirType(p2);

	Trackdir trackdir = TrackToTrackdir(track);

	CommandCost ret = ValidateAutoDrag(&trackdir, tile, p1);
	if (ret.Failed()) return ret;

	if ((flags & DC_EXEC) && _settings_client.sound.confirm) SndPlayTileFx(SND_1F_SPLAT_OTHER, tile);

	bool had_success = false;
	bool add = IsAddingAction(p2);
	SmallVector<Station *, 4> affected_stations;
	CommandCost last_error = CMD_ERROR;
	CommandCost total_cost;
	for (;;) {
		if (!IsValidTile(tile) ||
				!IsAirportTile(tile) ||
				!MayHaveAirTracks(tile) || IsHangar(tile))
			goto fill_next_track;

		if (air_type != GetAirportType(tile)) {
			ret.MakeError(STR_ERROR_AIRPORT_INVALID_AIR_TYPE);
		} else {
			if (add) {
				ret = AddAirportTrack(tile, TrackdirToTrack(trackdir), flags);
			} else {
				ret = RemoveAirportTrack(tile, TrackdirToTrack(trackdir), flags);
			}
		}

		if (ret.Failed()) {
			last_error = ret;
			if (last_error.GetErrorMessage() != STR_ERROR_ALREADY_BUILT) {
				return last_error;
			}

			/* Ownership errors are more important. */
			if (last_error.GetErrorMessage() == STR_ERROR_OWNED_BY) break;
		} else {
			had_success = true;
			affected_stations.Include(Station::GetByTile(tile));
			total_cost.AddCost(ret);
		}

fill_next_track:
		if (tile == p1) break; // end tile

		tile += ToTileIndexDiff(_trackdelta[trackdir]);

		/* toggle railbit for the non-diagonal tracks. */
		if (!IsDiagonalTrackdir(trackdir)) ToggleBit(trackdir, 0);
	}

	if (flags & DC_EXEC) {
		/* Do all station specific functions here. */
		for (Station **stp = affected_stations.Begin(); stp != affected_stations.End(); stp++) {
			Station *st = *stp;
			assert(st != NULL);
			st->UpdateAirportDataStructure();
		}
	}

	if (had_success) return total_cost;

	return last_error;
}

/**
 * Define/undefine a runway.
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = ()    - airport type (terminal/helipad/heliport)
 * - p2 = (bit 7)   - 0 = build, 1 = remove
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost AddRunway(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	assert(IsValidTile(tile));
	assert(IsAirportTile(tile));

	AirType air_type = GetAirType(p2);
	if (!ValParamAirType(air_type)) return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);

	if (air_type != GetAirportType(tile)) return_cmd_error(STR_ERROR_AIRPORT_INVALID_AIR_TYPE);

	Station *st = Station::GetByTile(tile);
	assert(st != NULL);

	if (st->airport.runways.Length() + 1 > GetAirTypeInfo(air_type)->max_num_runways) return_cmd_error(STR_ERROR_AIRPORT_TOO_MUCH_RUNWAYS);

	TileArea ta(tile, p1);
	assert(ta.h == 1 || ta.w == 1); // Diagonal area. //
	uint8 length = ta.h * ta.w;
	CommandCost ret = CheckRunwayLength(GetAirType(p2), length);
	if (ret.Failed()) return ret;

	TILE_AREA_LOOP(tile_iter, ta) {
		if (!IsAirportTile(tile_iter)) return_cmd_error(STR_ERROR_AIRPORT_RUNWAY_INCOMPLETE);
		if (GetStationIndex(tile_iter) != st->index) return_cmd_error(STR_ERROR_AIRPORT_RUNWAY_INCOMPLETE);
		if (!IsSimpleTrack(tile_iter) && !IsPlainRunway(tile_iter)) return_cmd_error(STR_ERROR_AIRPORT_RUNWAY_CANNOT_BUILD_OVER);
		if (IsPlainRunway(tile_iter) && (tile_iter == tile || tile_iter == p1)) return_cmd_error(STR_ERROR_AIRPORT_RUNWAY_CANNOT_BUILD_OVER);

		if (HasAirportTrackReserved(tile_iter) ||
				(IsPlainRunway(tile_iter) && GetReservationAsRunway(tile_iter))) {
			return_cmd_error(STR_ERROR_AIRPORT_AIRCRAFT_ON_TILES);
		}
	}

	CommandCost cost(EXPENSES_CONSTRUCTION);
	cost.AddCost(_price[PR_BUILD_STATION_AIRPORT] * length);
	DiagDirection dir = DiagdirBetweenTiles(tile, p1);

	if (flags & DC_EXEC) {
		TILE_AREA_LOOP(tile_iter, ta) {
			SB(_m[tile_iter].m4, 4, 4, 0);
			if ((tile_iter == tile) || (tile_iter == p1)) {
				assert(!IsRunway(tile_iter));
				SetAirportTileType(tile_iter,
						(tile == tile_iter) ? ATT_RUNWAY_START : ATT_RUNWAY_END);
				SetLandingType(tile_iter, GetMultiPurposeBit(p2));
				SetRunwayDirection(tile_iter, dir);
			} else {
				Axis axis = DiagDirToAxis(dir);

				if (IsPlainRunway(tile_iter)) {
					TrackBits tracks = (axis == AXIS_X) ? TRACK_BIT_X : TRACK_BIT_Y;
					assert((GetRunwayTracks(tile_iter) & tracks) == TRACK_BIT_NONE);
					SetRunwayAxis(tile_iter, true, true);
				} else {
					SetAirportTileType(tile_iter, ATT_RUNWAY);
					SetRunwayAxis(tile_iter, axis == AXIS_X, axis == AXIS_Y);
				}
			}
		}

		st->UpdateAirportDataStructure();
	}

	return cost;
}

/**
 * Define/undefine a terminal.
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = ()    - airport type (terminal/helipad/heliport)
 * - p2 = (bit 7)   - 0 = build, 1 = remove
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost RemoveRunway(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (tile == p1 || !IsRunwayExtreme(tile) || !IsRunwayExtreme(p1) || IsLandingTypeTile(tile) != GetMultiPurposeBit(p2)) return_cmd_error(STR_ERROR_AIRPORT_RUNWAY_CANNOT_REMOVE);

	assert(GetOtherStartPlatformTile(tile) == p1);
	Axis axis = DiagDirToAxis(GetDirection(p2));

	TileArea ta(tile, p1);
	CommandCost cost(EXPENSES_CONSTRUCTION);
	cost.AddCost(_price[PR_CLEAR_STATION_AIRPORT] * ta.w * ta.h);

	TILE_AREA_LOOP(tile_iter, ta) {
		if (!HasAirportTrackReserved(tile_iter)) continue;
		return_cmd_error(STR_ERROR_AIRPORT_AIRCRAFT_ON_TILES);
	}

	if (flags & DC_EXEC) {
		Station *st = Station::Get(GetStationIndex(tile));
		assert(st != NULL);

		TILE_AREA_LOOP(tile_iter, ta) {
			assert(IsRunway(tile_iter));
			if (IsPlainRunway(tile_iter) && GetRunwayTracks(tile_iter) == TRACK_BIT_CROSS) {
				SetRunwayAxis(tile_iter, axis != AXIS_X, axis != AXIS_Y);
			} else {
				SetAirportTileType(tile_iter, ATT_SIMPLE_TRACK);
				SB(_m[tile_iter].m4, 4, 4, 0);
			}
		}

		st->UpdateAirportDataStructure();
	}

	return cost;
}


/**
 * Redefine use of airport tiles.
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit   0      ) - 0 = Remove, 1 = Add
 * - p2 = (bit   1      ) - 0 = Rect, 1 = Diagonal (_ctrl_pressed)
 * - p2 = (bit   2      ) - Multiple purporse:
 *                        - 0 for small hangar, 1 for big hangar
 *                        - infrastructure 1-with / 0-without catchment
 *                        - runway 0-forbidding / 1-allowing landing
 * - p2 = (bits  7 -  8 ) - Hangar exit direction.
 * - p2 = (bits  9 - 12 ) - AirType (gravel, asphalt, water,...)
 * - p2 = (bits 13 - 15 ) - AirportTileType (hangar, helipad, hangar,...)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdChangeAirportTiles(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (!IsAirportTile(tile)) return CMD_ERROR;

	AirportTileType air_tile_type = GetAirTileType(p2);
	switch(air_tile_type) {
		case ATT_INFRASTRUCTURE:
		case ATT_TERMINAL:
		case ATT_HANGAR:
		case ATT_RUNWAY_START:
			break;
		case ATT_SIMPLE_TRACK:
		case ATT_RUNWAY:
		case ATT_RUNWAY_END:
		default:
			/* Not handled in this command. */
			NOT_REACHED();
		case ATT_WAITING_POINT:
			/* Not implemented. */
			NOT_REACHED();
	}

	CommandCost ret = CheckSettingBuildByTile();
	if (ret.Failed()) return ret;

	TileArea ta(tile, p1);
	if (IsAddingAction(p2) && CheckTooCloseToHouses(ta)) return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_TOO_CLOSE);

	AirType air_type = GetAirType(p2);
	if (!ValParamAirType(air_type)) return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);

	if (GetAirTileType(p2) == ATT_RUNWAY_START) {
		if (air_type != GetAirportType(tile)) return_cmd_error(STR_ERROR_AIRPORT_INVALID_AIR_TYPE);
		if (GetTileOwner(tile) != _current_company) return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);

		if (IsAddingAction(p2)) {
			/* check same station and free */
			return AddRunway(tile, flags, p1, p2, text);
		} else {
			return RemoveRunway(tile, flags, p1, p2, text);
		}
	}

	SmallVector<StationID, 4> affected_stations_id;

	CommandCost cost(EXPENSES_CONSTRUCTION);
	TileIterator *iter = GetCtrlState(p2) ? (TileIterator *)new DiagonalTileIterator(tile, p1) : new OrthogonalTileIterator(TileArea(tile, p1));

	for (; *iter != INVALID_TILE; ++(*iter)) {
		TileIndex tile_iter = *iter;
		if (!IsAirportTile(tile_iter)) continue;
		if (GetTileOwner(tile_iter) != _current_company) continue;
		if (air_type != GetAirportType(tile_iter)) continue;

		affected_stations_id.Include(GetStationIndex(tile_iter));

		if (IsAddingAction(p2)) {
			if (!IsSimpleTrack(tile_iter)) continue;
			cost.AddCost(_price[PR_BUILD_STATION_AIRPORT]);

			if (flags & DC_EXEC) {
				switch (air_tile_type) {
					default: NOT_REACHED();
					case ATT_INFRASTRUCTURE:
						SetAirportTileTracks(tile_iter, TRACK_BIT_NONE);
						SetAirportTileType(tile_iter, air_tile_type);
						SetCatchmentAirportType(tile_iter, GetMultiPurposeBit(p2));
						break;
					case ATT_TERMINAL:
						SetAirportTileType(tile_iter, air_tile_type);
						SetTerminalType(tile_iter, GetTerminalTypeP2(p2));
						break;
					case ATT_HANGAR:
						SetAirportTileType(tile_iter, air_tile_type);
						SetBigHangar(tile_iter, GetMultiPurposeBit(p2));
						SetHangarDirection(tile_iter, GetDirection(p2));
						SetAirportTileTracks(tile_iter, HasBit(_m[tile_iter].m4, 4) ? TRACK_BIT_Y : TRACK_BIT_X);
						break;
				}
				MarkTileDirtyByTile(tile_iter);
			}

		} else {
			if (GetAirportTileType(tile_iter) != air_tile_type) continue;
			if (air_tile_type == ATT_HANGAR) {
				if (!IsHangar(tile_iter) || GetMultiPurposeBit(p2) != IsBigHangar(tile_iter)) continue;
			} else if (air_tile_type == ATT_INFRASTRUCTURE) {
				if (GetMultiPurposeBit(p2) != GetCatchmentAirportType(tile_iter)) continue;
			}

			cost.AddCost(_price[PR_CLEAR_STATION_AIRPORT]);
			if (flags & DC_EXEC) {
				switch (air_tile_type) {
					default: NOT_REACHED();
					case ATT_INFRASTRUCTURE:
					case ATT_TERMINAL:
					case ATT_HANGAR:
						SetAirportTileType(tile_iter, ATT_SIMPLE_TRACK);
						break;
				}
				MarkTileDirtyByTile(tile_iter);
			}
		}
	}

	for (StationID *stp = affected_stations_id.Begin(); stp != affected_stations_id.End(); stp++) {
		Station *st = Station::Get(*stp);
		assert(st != NULL);

		if (flags & DC_EXEC) {
			st->UpdateAirportDataStructure();
			st->UpdateCatchment();
			UpdateCALayer(st->index);
		}

		ret = AircraftInAirport(st);
		if (ret.Failed()) return ret;
	}

	return cost;
}

/**
 * Calculate the noise level an airport would have given an airtype.
 * @param
 */
uint8 CalculateNoiseLevelForAirtype(const Airport *airport, AirType airtype)
{
	if (airport->helipads.Length() == 1 && IsBuiltInHeliport(airport->helipads[0])) return 0;
	const AirTypeInfo *air_type_info = GetAirTypeInfo(airtype);
	return air_type_info->base_noise_level
		+ airport->runways.Length() * air_type_info->runway_noise_level
		+ airport->helipads.Length() + airport->terminals.Length();
}

uint8 GetAirportNoise(const Airport *airport)
{
	return CalculateNoiseLevelForAirtype(airport, airport->air_type);
}

/**
 * Change the air type of an airport.
 * @param tile with one tile of the airport.
 * @param flags operation to perform
 * @param p1 unused
 * @param p2 various bitstuffed elements
 * - p2 = (bits  9 - 12) - new air type for the airport
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdChangeAirType(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (!IsAirportTile(tile)) return_cmd_error(STR_ERROR_SITE_UNSUITABLE);
	if (!IsValidTile(tile)) return CMD_ERROR;

	CommandCost ret = CheckSettingBuildByTile();
	if (ret.Failed()) return ret;

	ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	assert(Station::IsValidID(GetStationIndex(tile)));
	Station *st = Station::Get(GetStationIndex(tile));
	assert(st != NULL);

	if (CheckTooCloseToHouses(st->airport)) return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_TOO_CLOSE);

	/* Check air type. */
	AirType air_type = GetAirType(p2);
	if (!ValParamAirType(air_type)) return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);
	if (st->airport.air_type == air_type) return_cmd_error(STR_ERROR_AIRPORT_ALREADY_AIRTYPE);
	if (st->airport.air_type == AIRTYPE_WATER) return_cmd_error(STR_ERROR_AIRPORT_CANT_CONVERT_TO_WATERED);
	if (air_type == AIRTYPE_WATER) return CMD_ERROR;
	if (st->airport.runways.Length() > GetAirTypeInfo(air_type)->max_num_runways) return_cmd_error(STR_ERROR_AIRPORT_TOO_MUCH_RUNWAYS);

	for (const TileIndex *tile = st->airport.runways.Begin();
			tile < st->airport.runways.End(); tile++) {
		uint length = GetPlatformLength(*tile);
		ret = CheckRunwayLength(air_type, length);
		if (ret.Failed()) return ret;
	}

	ret = AircraftInAirport(st);
	if (ret.Failed()) return ret;

	if (_settings_game.economy.station_noise_level) {
		if (CalculateNoiseLevelForAirtype(&st->airport, air_type) +
				st->town->noise_reached > GetAirportNoise(&st->airport) +
				st->town->MaxTownNoise())
			return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_NOISE);
	}

	if (flags & DC_EXEC) {
		st->town->noise_reached += CalculateNoiseLevelForAirtype(&st->airport, air_type) - GetAirportNoise(&st->airport);
		if (_settings_game.economy.station_noise_level) {
			SetWindowDirty(WC_TOWN_VIEW, st->town->index);
		}

		TILE_AREA_LOOP(tile_iter, st->airport) {
			if (!IsAirportTileOfStation(tile_iter, st->index)) continue;

			SetAirportType(tile_iter, air_type);
			MarkTileDirtyByTile(tile_iter);
		}

		st->UpdateCatchment();
		TileArea catchment_area(st->airport);
		catchment_area.AddRadius(MAX_CATCHMENT);
		UpdateCALayer(catchment_area);
		if (st->airport.depot_id != INVALID_DEPOT) InvalidateWindowData(WC_BUILD_VEHICLE, st->airport.depot_id);
	}
	return CommandCost();
}

static CommandCost RemoveAirportTiles(TileIndex org_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text = NULL)
{
	if (!IsValidTile(p1)) return CMD_ERROR;

	AirType air_type = GetAirType(p2);

	if (_current_company < MAX_COMPANIES && !ValParamAirType(air_type)) return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);

	TileArea ta(org_tile, p1);
	SmallVector<Station *, 4> affected_stations;

	CommandCost cost(EXPENSES_CONSTRUCTION);

	TILE_AREA_LOOP(tile, ta) {
		if (!IsAirportTile(tile)) continue;
		if (air_type != GetAirportType(tile)) continue;
		if (MayHaveAirTracks(tile) && HasAirportTrackReserved(tile)) continue;

		if (_current_company != OWNER_WATER) {
			CommandCost ret = CheckTileOwnership(tile);
			if (ret.Failed()) continue;
		}

		/* Check aircraft in airport. */
		Station *st = Station::Get(GetStationIndex(tile));
		if (!affected_stations.Contains(st)) {
			assert(st != NULL);

			CommandCost ret = AircraftInAirport(st);
			if (ret.Failed()) return ret;
			affected_stations.Include(st);
		}

		if (IsRunway(tile)) {
			if (_current_company < MAX_COMPANIES) return_cmd_error(STR_ERROR_AIRPORT_REMOVE_RUNWAYS_FIRST);
			if (GetReservationAsRunway(tile)) {
				// in fact, if water flooding, check for landing aircraft and crash it
				return_cmd_error();
			}
			TileIndex start_tile = GetStartPlatformTile(tile);
			CommandCost ret2 = RemoveRunway(start_tile, flags, GetOtherStartPlatformTile(start_tile), p2 |(IsLandingTypeTile(start_tile) << 2), NULL);
			assert(ret2.Succeeded());
		}

		cost.AddCost(AirClearCost(air_type));

		if (!IsSimpleTrack(tile)) cost.AddCost(AirClearCost(air_type));

		if (flags & DC_EXEC) {
			WaterClass wc = GetWaterClass(tile);
			DoClearSquare(tile);
			/* Maybe change to water */
			if (wc != WATER_CLASS_INVALID) {
				Owner o = (wc == WATER_CLASS_CANAL) ? st->owner : OWNER_WATER;
				MakeWater(tile, o, wc, Random());
				UpdateWaterTiles(tile, 1);
			}

			Company::Get(st->owner)->infrastructure.station--;
			Company::Get(st->owner)->infrastructure.airport--;
			DeleteNewGRFInspectWindow(GSF_STATIONS, tile);
			st->rect.AfterRemoveTile(st, tile);
			MarkTileDirtyByTile(tile);
		}
	}

	if (flags & DC_EXEC) {
		/* Do all station specific functions here. */
		for (Station **stp = affected_stations.Begin(); stp != affected_stations.End(); stp++) {
			Station *st = *stp;

			TileArea temp(ta.tile, ta.w, ta.h);
			st->airport.Clear();
			TILE_AREA_LOOP(tile, temp) {
				if (IsAirportTile(tile) && st->index == GetStationIndex(tile)) st->airport.Add(tile);
			}

			AirType air_type = st->airport.air_type;
			st->UpdateAirportDataStructure();
			if (st->airport.tile == INVALID_TILE) {
				st->facilities &= ~FACIL_AIRPORT;
				st->town->noise_reached -= GetAirTypeInfo(air_type)->base_noise_level;
			}

			st->AfterStationTileSetChange(false, ta, STATION_AIRPORT);
			st->MarkTilesDirty(false);
		}

		ta.AddRadius(Station::GetCatchmentRadius(STATION_AIRPORT));
		Industry::RecomputeStationsNearArea(ta);
		UpdateCALayer(ta);
	}

	for (Station **stp = affected_stations.Begin(); stp != affected_stations.End(); stp++) {
		Station *st = *stp;
		assert(st != NULL);

		CommandCost ret = AircraftInAirport(st);
		if (ret.Failed()) return ret;
	}

	/* Now apply the rail cost to the number that we deleted. */
	return cost;
}


/**
 * Calculate the noise level generated by an airport.
 * @param
 */
uint8 CalculateNoiseLevel(const Station *st)
{
	assert(IsValidTile(st->airport.tile));

	if (IsBuiltInHeliportTile(st->airport.tile)) return 0;

	return GetAirportNoise(&st->airport);
}

/** Recalculate the noise generated by the airports of each town. */
void UpdateAirportsNoise()
{
	Town *t;
	const Station *st;

	FOR_ALL_TOWNS(t) t->noise_reached = 0;

	FOR_ALL_STATIONS(st) {
		if (!st->HasFacilities(FACIL_AIRPORT)) continue;
		assert(st->town != NULL && Town::IsValidID(st->town->index)); // revise
		st->town->noise_reached += CalculateNoiseLevel(st);
	}
}

TileIndex GetAnAirportTile(Station* st)
{
	if (st == NULL || st->airport.tile == INVALID_TILE) return INVALID_TILE;

	TILE_AREA_LOOP(tile, st->airport) {
		if (IsAirportTile(tile) && GetStationIndex(tile) == st->index) return tile;
	}

	NOT_REACHED();
}

/**
 * Checks if an airport can be built at the given area.
 * @param tile_area Area to check.
 * @param flags Operation to perform.
 * @param station StationID to be queried and returned if available.
 * @return The cost in case of success, or an error code if it failed.
 */
static CommandCost CheckFlatLandAirport(TileArea tile_area, DoCommandFlag flags, StationID *station)
{
	CommandCost cost(EXPENSES_CONSTRUCTION);
	int allowed_z = -1;

	TILE_AREA_LOOP(tile_cur, tile_area) {
		CommandCost ret = CheckBuildableTile(tile_cur, 0, allowed_z, false);
		if (ret.Failed()) return ret;

		/* if station is set, then we have special handling to allow building on top of already existing stations.
		 * so station points to INVALID_STATION if we can build on any station.
		 * Or it points to a station if we're only allowed to build on exactly that station. */
		if (station != NULL && IsTileType(tile_cur, MP_STATION)) {
			if (!IsAirportTile(tile_cur)) {
				return ClearTile_Station(tile_cur, DC_AUTO); // get error message
			} else {
				StationID st = GetStationIndex(tile_cur);
				if (*station == INVALID_STATION) {
					*station = st;
				} else if (*station != st) {
					return_cmd_error(STR_ERROR_ADJOINS_MORE_THAN_ONE_EXISTING);
				}
			}
		}
	}

	return cost;
}


/**
 * Add/remove tiles for an airport.
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit   0      ) - 0 = Remove, 1 = Add
 * - p2 = (bit   1      ) - allow airports directly adjacent to other airports.
 * - p2 = (bits  8 - 11 ) - Air type (gravel, asphalt, water,...)  checked
 * - p2 = (bits 16 - 31 ) - StationID where to apply the command (NEW_STATION for a new station).
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdAddRemoveAirportTiles(TileIndex org_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CommandCost ret = CheckSettingBuildByTile();
	if (ret.Failed()) return ret;

	if (!IsAddingAction(p2)) return RemoveAirportTiles(org_tile, flags, p1, p2, text);

	/* Unpack parameters. */
	AirType at                = GetAirType(p2);
	bool adjacent             = GetCtrlState(p2);
	StationID station_to_join = GetStationID(p2);

	/* Does the authority allow this? */
	ret = CheckIfAuthorityAllowsNewStation(org_tile, flags);
	if (ret.Failed()) return ret;

	/* Check air type. */
	if (!ValParamAirType(at)) return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);

	TileArea new_location(org_tile, p1);

	/* Check far from towns. */
	if (CheckTooCloseToHouses(new_location)) return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_TOO_CLOSE);

	Station *st = NULL;
	if (Station::IsValidID(station_to_join)) {
		st = Station::Get(station_to_join);
		if (st->airport.tile != INVALID_TILE && st->airport.air_type != at)
				return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);
	}

	bool reuse = (station_to_join != NEW_STATION);
	if (!reuse) station_to_join = INVALID_STATION;
	bool distant_join = (station_to_join != INVALID_STATION);

	if (distant_join && (!_settings_game.station.distant_join_stations || !Station::IsValidID(station_to_join))) return CMD_ERROR;

	if (new_location.w > _settings_game.station.station_spread || new_location.h > _settings_game.station.station_spread) return CMD_ERROR;

	/* Make sure the area below consists of clear tiles. */
	StationID est = INVALID_STATION;
	/* Clear the land below the station. */
	CommandCost cost = CheckFlatLandAirport(new_location, flags, &est);
	if (cost.Failed()) return cost;

	st = NULL;
	ret = FindJoiningStation(est, station_to_join, adjacent, new_location, &st);
	if (ret.Failed()) return ret;

	ret = AircraftInAirport(st);
	if (ret.Failed()) return ret;

	if (st != NULL && st->airport.tile != INVALID_TILE && st->airport.air_type != at) return_cmd_error(STR_ERROR_AIRPORT_INCORRECT_AIRTYPE);

	/* This may be troublesome. */
	Town *town = st == NULL ? CalcClosestTownFromTile(org_tile, UINT_MAX) : st->town;
	assert(town != NULL);

	if (st == NULL || st->airport.tile == INVALID_TILE) {
		/* Check if local auth would allow a new airport due to airport limits. */
		StringID authority_refuse_message = STR_NULL;
		if (_settings_game.economy.station_noise_level) {
			/* do not allow to build a new airport if this raise the town noise over the maximum allowed by town */
			if (town->noise_reached + GetAirTypeInfo(at)->base_noise_level > town->MaxTownNoise()) {
				authority_refuse_message = STR_ERROR_LOCAL_AUTHORITY_REFUSES_NOISE;
			}
		} else {
			uint num = 0;
			const Station *st;
			FOR_ALL_STATIONS(st) {
				if (st->town == town && (st->facilities & FACIL_AIRPORT) && st->airport.type != AT_OILRIG) num++;
			}
			if (num >= 2) {
				authority_refuse_message = STR_ERROR_LOCAL_AUTHORITY_REFUSES_AIRPORT;
			}
		}
		if (authority_refuse_message != STR_NULL) {
			SetDParam(0, town->index);
			return_cmd_error(authority_refuse_message);
		}
	}

	ret = BuildStationPart(&st, flags, reuse, new_location, STATIONNAMING_AIRPORT);
	if (ret.Failed()) return ret;

	TileIndex airport_tile = GetAnAirportTile(st);
	int z = -1;
	if (airport_tile != INVALID_TILE) {
		Slope tileh = GetTileSlope(airport_tile, &z);
		z = z + GetSlopeMaxZ(tileh);
	}

	Company *c = Company::GetIfValid(_current_company);

	if ((flags & DC_EXEC) && st->airport.tile == INVALID_TILE) {
		town->noise_reached += GetAirTypeInfo(at)->base_noise_level;
	}

	bool watered = (at == AIRTYPE_WATER);
	TILE_AREA_LOOP(tile, new_location) {
		Slope tileh = GetTileSlope(tile);

		if (watered) {
			if (!IsWaterTile(tile) || !IsTileFlat(tile)) return_cmd_error(STR_ERROR_AIRPORT_PLAIN_WATER);
		} else {
			if (IsTileType(tile, MP_WATER)) {
				if (!IsSlopeWithOneCornerRaised(tileh)) return_cmd_error(STR_ERROR_CAN_T_BUILD_ON_WATER);
				cost.AddCost(-_price[PR_CLEAR_WATER]);
				cost.AddCost(_price[PR_CLEAR_ROUGH]);
			}

			ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
			if (ret.Failed()) continue;
			cost.AddCost(ret);
		}

		ret = CheckBuildableTile(tile, 0, z, false);
		if (ret.Failed()) return_cmd_error(STR_ERROR_AIRPORT_NOT_SAME_LEVEL);

		cost.AddCost(ret);

		if (flags & DC_EXEC) {
			st->airport.Add(tile);

			/* Initialize an empty station. */
			st->AddFacility(FACIL_AIRPORT, tile);

			st->rect.BeforeAddTile(tile, StationRect::ADD_TRY);

			MakeAirport(tile, st->owner, st->index, 0, watered ? GetWaterClass(tile) : WATER_CLASS_INVALID);
			SetAirportType(tile, at);
			SetAirportTileType(tile, ATT_SIMPLE_TRACK);

			c->infrastructure.airport++;
			c->infrastructure.station++;
			DirtyCompanyInfrastructureWindows(c->index);
			MarkTileDirtyByTile(tile);

		}
	}

	if (flags & DC_EXEC) {
		assert(st != NULL);
		if (watered) {
			TileArea ta(new_location.tile, new_location.w, new_location.h, 1);
			TILE_AREA_LOOP(tile, ta) {
				UpdateWaterTiles(tile, 0);
			}
		}
		st->UpdateAirportDataStructure();
		st->AfterStationTileSetChange(true, new_location, STATION_AIRPORT);
	}

	return cost;
}


/**
 * Place an Airport.
 * @param tile tile where airport will be built
 * @param flags operation to perform
 * @param p1
 * - p1 = (bit  0- 7) - airport type, @see airport.h
 * - p1 = (bit  8-15) - airport layout
 * @param p2 various bitstuffed elements
 * - p2 = (bit     0) - allow airports directly adjacent to other airports.
 * - p2 = (bit 16-31) - station ID to join (NEW_STATION if build new one)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildAirport(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	StationID station_to_join = GB(p2, 16, 16);
	bool reuse = (station_to_join != NEW_STATION);
	if (!reuse) station_to_join = INVALID_STATION;
	bool distant_join = (station_to_join != INVALID_STATION);
	byte airport_type = GB(p1, 0, 8);
	byte layout = GB(p1, 8, 8);

	if (distant_join && (!_settings_game.station.distant_join_stations || !Station::IsValidID(station_to_join))) return CMD_ERROR;

	if (airport_type >= NEW_AIRPORT_OFFSET) return CMD_ERROR;

	CommandCost ret = CheckIfAuthorityAllowsNewStation(tile, flags);
	if (ret.Failed()) return ret;

	/* Check if a valid, buildable airport was chosen for construction */
	const AirportSpec *as = AirportSpec::Get(airport_type);

	if (_translation_airport_hangars[airport_type] && !Depot::CanAllocateItem()) return CMD_ERROR;

	if (!as->IsAvailable() || layout >= as->num_table) return CMD_ERROR;

	Direction rotation = as->rotation[layout];
	int w = as->size_x;
	int h = as->size_y;

	if (rotation == DIR_E || rotation == DIR_W) Swap(w, h);
	TileArea airport_area = TileArea(tile, w, h);

	if (w > _settings_game.station.station_spread || h > _settings_game.station.station_spread) {
		return_cmd_error(STR_ERROR_STATION_TOO_SPREAD_OUT);
	}

	CommandCost cost = CheckFlatLand(airport_area, flags);
	if (cost.Failed()) return cost;

	Station *st = NULL;
	ret = FindJoiningStation(INVALID_STATION, station_to_join, HasBit(p2, 0), airport_area, &st);
	if (ret.Failed()) return ret;

	/* Distant join */
	if (st == NULL && distant_join) st = Station::GetIfValid(station_to_join);

	Town *town = st == NULL ? CalcClosestTownFromTile(tile, UINT_MAX) : st->town;
	assert(town != NULL);

	/* Check if local auth would allow a new airport due to airport limits. */
	StringID authority_refuse_message = STR_NULL;
	if (_settings_game.economy.station_noise_level) {
		/* do not allow to build a new airport if this raise the town noise over the maximum allowed by town */
		if ((town->noise_reached + as->noise_level) > town->MaxTownNoise()) {
			authority_refuse_message = STR_ERROR_LOCAL_AUTHORITY_REFUSES_NOISE;
		}
	} else {
		uint num = 0;
		const Station *st;
		FOR_ALL_STATIONS(st) {
			if (st->town != town) continue;
			if ((st->facilities & FACIL_AIRPORT) == 0) continue;
			if (st->airport.type != AT_OILRIG) num++;
		}
		if (num >= 2) {
			authority_refuse_message = STR_ERROR_LOCAL_AUTHORITY_REFUSES_AIRPORT;
		}
	}

	if (authority_refuse_message != STR_NULL) {
		SetDParam(0, town->index);
		return_cmd_error(authority_refuse_message);
	}

	/* Check far from towns. */
	if (CheckTooCloseToHouses(airport_area)) return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_TOO_CLOSE);

	// revise
	ret = BuildStationPart(&st, flags, reuse, airport_area,
			_translation_airport_heliport[airport_type] ?
			STATIONNAMING_HELIPORT : STATIONNAMING_AIRPORT);
	if (ret.Failed()) return ret;

	if (st != NULL && st->airport.tile != INVALID_TILE) {
		if (_settings_game.station.allow_modify_airports) {
			return_cmd_error(STR_ERROR_AIRPORT_CANT_JOIN_LAYOUT);
		} else {
			return_cmd_error(STR_ERROR_TOO_CLOSE_TO_ANOTHER_AIRPORT);
		}
	}

	cost.AddCost(_price[PR_BUILD_STATION_AIRPORT] * w * h);

	if (flags & DC_EXEC) {
		/* Always add the noise, so there will be no need to recalculate when option toggles */
		town->noise_reached += as->noise_level;

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

			if (AirportTileSpec::Get(GetTranslatedAirportTileID(iter.GetStationGfx()))->animation.status != ANIM_STATUS_NO_ANIMATION) AddAnimatedTile(iter);
		}

		/* Only call the animation trigger after all tiles have been built */
		for (AirportTileTableIterator iter(as->table[layout], tile); iter != INVALID_TILE; ++iter) {
			AirportTileAnimationTrigger(st, iter, AAT_BUILT);
		}

		st->TranslateAirport();

		UpdateAirplanesOnNewStation(st);

		Company::Get(st->owner)->infrastructure.airport++;

		st->AfterStationTileSetChange(true, st->airport, STATION_AIRPORT);
		InvalidateWindowData(WC_STATION_VIEW, st->index, -1);

		if (_settings_game.economy.station_noise_level) {
			SetWindowDirty(WC_TOWN_VIEW, st->town->index);
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
static CommandCost RemoveAirport(TileIndex tile, DoCommandFlag flags)
{
	Station *st = Station::GetByTile(tile);

	CommandCost ret = AircraftInAirport(st);
	if (ret.Failed()) return ret;

	/* revise */
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckOwnership(st->owner);
		if (ret.Failed()) return ret;
	}

	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (flags & DC_EXEC) {
		st->town->noise_reached -= CalculateNoiseLevel(st);
	}

	MASKED_TILE_AREA_LOOP(tile_cur, st->airport, st->airport.footprint) {
		assert(st->TileBelongsToAirport(tile_cur));

		cost.AddCost(_price[PR_CLEAR_STATION_AIRPORT]);

		if (flags & DC_EXEC) {
			DeleteAnimatedTile(tile_cur);
			DoClearSquare(tile_cur);
			DeleteNewGRFInspectWindow(GSF_AIRPORTTILES, tile_cur);
			Company::Get(st->owner)->infrastructure.airport--;
		}
	}

	if (flags & DC_EXEC) {
		/* Clear the persistent storage. */
		delete st->airport.psa;

		DeleteWindowById(WC_VEHICLE_DEPOT, st->airport.depot_id);
		OrderBackup::Reset(st->airport.depot_id, false);

		st->rect.AfterRemoveRect(st, st->airport);

		st->airport.Clear();
		st->facilities &= ~FACIL_AIRPORT;

		InvalidateWindowData(WC_STATION_VIEW, st->index, -1);

		if (_settings_game.economy.station_noise_level) {
			SetWindowDirty(WC_TOWN_VIEW, st->town->index);
		}

		st->AfterStationTileSetChange(false, st->airport, STATION_AIRPORT);

		DeleteNewGRFInspectWindow(GSF_AIRPORTS, st->index);
	}

	return cost;
}


/**
 * Computes the minimal distance from town's xy to any airport's tile.
 * @param it An iterator over all airport tiles.
 * @param town_tile town's tile (t->xy)
 * @return minimal manhattan distance from town_tile to any airport's tile
 */
static uint GetMinimalAirportDistanceToTile(TileIterator &it, TileIndex town_tile)
{
	uint mindist = UINT_MAX;

	for (TileIndex cur_tile = it; cur_tile != INVALID_TILE; cur_tile = ++it) {
		mindist = min(mindist, DistanceManhattan(town_tile, cur_tile));
	}

	return mindist;
}

/**
 * Get the distance
 * @param as airport information
 * @param it An iterator over all airport tiles.
 * @param town_tile TileIndex of town's center, the one who will receive the airport's candidature
 * @return the noise that will be generated, according to distance
 */
uint GetMinTownAirportDistance()
{
	return 3 + (_settings_game.difficulty.town_council_tolerance * 4);
}

/**
 * CircularTileSearch callback; finds the tile furthest from any
 * @return always false
 */
static bool FindClosestHouse(TileIndex tile, void *user_data)
{
	return IsTileType(tile, MP_HOUSE);
}

/**
 * Check if we are too close to houses
 */
bool CheckTooCloseToHouses(const TileArea ta)
{
	TileArea mod_ta(ta);
	TILE_AREA_LOOP(tile, mod_ta) {
		if (IsTileType(tile, MP_HOUSE)) return false;
	}

	if (CircularTileSearch(&mod_ta.tile, GetMinTownAirportDistance(), mod_ta.w, mod_ta.h, FindClosestHouse, NULL)) return true;

	return false;
}

/**
 * Finds the town nearest to given airport. Based on minimal manhattan distance to any airport tile.
 * If two towns have the same distance, town with lower index is returned.
 * @param as airport description
 * @param it An iterator over all airport tiles
 * @return nearest town to airport
 */
Town *AirportGetNearestTown(const AirportSpec *as, const TileIterator &it)
{
	Town *t, *nearest = NULL;
	uint add = as->size_x + as->size_y - 2; // GetMinimalAirportDistanceToTile can differ from DistanceManhattan by this much
	uint mindist = UINT_MAX - add; // prevent overflow
	FOR_ALL_TOWNS(t) {
		if (DistanceManhattan(t->xy, it) < mindist + add) { // avoid calling GetMinimalAirportDistanceToTile too often
			TileIterator *copy = it.Clone();
			uint dist = GetMinimalAirportDistanceToTile(*copy, t->xy);
			delete copy;
			if (dist < mindist) {
				nearest = t;
				mindist = dist;
			}
		}
	}

	return nearest;
}

/**
 * Open/close an airport to incoming aircraft.
 * @param tile Unused.
 * @param flags Operation to perform.
 * @param p1 Station ID of the airport.
 * @param p2 Unused.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdOpenCloseAirport(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (!Station::IsValidID(p1)) return CMD_ERROR;
	Station *st = Station::Get(p1);

	if (!(st->facilities & FACIL_AIRPORT) || st->owner == OWNER_NONE) return CMD_ERROR;

	CommandCost ret = CheckOwnership(st->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		st->airport.flags ^= AIRPORT_CLOSED_block;
		SetWindowWidgetDirty(WC_STATION_VIEW, st->index, WID_SV_CLOSE_AIRPORT);
	}
	return CommandCost();
}
