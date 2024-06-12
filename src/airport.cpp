/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport.cpp Functions related to airports. */

#include "stdafx.h"
#include "station_base.h"
#include "newgrf_airtype.h"
#include "depot_base.h"
#include "air_map.h"
#include "window_func.h"

#include "debug.h"

#include "table/airtypes.h"
#include "table/strings.h"
#include "table/airporttile_ids.h"
#include "table/airport_defaults.h"

#include "safeguards.h"


/** Helper type for lists/vectors of trains */
typedef std::vector<Aircraft *> AircraftList;

AirTypeInfo _airtypes[AIRTYPE_END];
std::vector<AirType> _sorted_airtypes;
AirTypes _airtypes_hidden_mask;


const AirportSpec AirportSpec::dummy = {{APC_BEGIN, 0}, {}, INVALID_AIRTYPE, 0, 0, 0, 0, 0, CalendarTime::MIN_YEAR, CalendarTime::MIN_YEAR, STR_NULL, ATP_TTDP_LARGE, 0, false, false, false, GRFFileProps(AT_INVALID)};
const AirportSpec AirportSpec::custom = {{APC_CUSTOM, 0}, {}, INVALID_AIRTYPE, 0, 0, 0, 0, 0, CalendarTime::MIN_YEAR, CalendarTime::MIN_YEAR, STR_AIRPORT_CUSTOM, ATP_TTDP_LARGE, 0, false, false, false, GRFFileProps(AT_INVALID)};


void ResolveAirTypeGUISprites(AirTypeInfo *ati)
{
	SpriteID cursors_base = GetCustomAirSprite(ati, INVALID_TILE, ATSG_CURSORS);
	if (cursors_base != 0) {
		ati->gui_sprites.add_airport_tiles        = cursors_base +   0;
		ati->gui_sprites.build_track_tile         = cursors_base +   1;
		ati->gui_sprites.change_airtype           = cursors_base +   2;
		ati->gui_sprites.build_catchment_infra    = cursors_base +   3;
		ati->gui_sprites.build_noncatchment_infra = cursors_base +   4;
		ati->gui_sprites.define_landing_runway    = cursors_base +   5;
		ati->gui_sprites.define_nonlanding_runway = cursors_base +   6;
		ati->gui_sprites.build_apron              = cursors_base +   7;
		ati->gui_sprites.build_helipad            = cursors_base +   8;
		ati->gui_sprites.build_heliport           = cursors_base +   9;
		ati->gui_sprites.build_hangar             = cursors_base +  10;

		ati->cursor.add_airport_tiles        = cursors_base +  11;
		ati->cursor.build_track_tile         = cursors_base +  12;
		ati->cursor.change_airtype           = cursors_base +  13;
		ati->cursor.build_catchment_infra    = cursors_base +  14;
		ati->cursor.build_noncatchment_infra = cursors_base +  15;
		ati->cursor.define_landing_runway    = cursors_base +  16;
		ati->cursor.define_nonlanding_runway = cursors_base +  17;
		ati->cursor.build_apron              = cursors_base +  18;
		ati->cursor.build_helipad            = cursors_base +  19;
		ati->cursor.build_heliport           = cursors_base +  20;
		ati->cursor.build_hangar             = cursors_base +  21;
	}
}


/**
 * Reset all air type information to its default values.
 */
void ResetAirTypes()
{
	static_assert(lengthof(_original_airtypes) <= lengthof(_airtypes));

	uint i = 0;
	for (; i < lengthof(_original_airtypes); i++) _airtypes[i] = _original_airtypes[i];

	static const AirTypeInfo empty_airtype = {
			{	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Ground sprite
				  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				},
				{
					{	// Airport buildings with infrastructure: non-snowed/snowed + 5 building + 4 rotations
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 }
					},
					{
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 },
						{ 0, 0, 0, 0 }
					}
				},
				{	// Airport animated flag    revise: maybe 4 sprites instead of 16 is enough
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 }
				},
				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Radar sprites
				{ // Infrastructure with no catchment (with 4 rotated sprites): transmitter, snowed transmitter, tower, snowed tower
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 }
				},
				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Runway sprites
				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  }, // Sprites for normal apron, helipad, 4 rotated sprites for heliports and 4 more for snowed heliports
				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // Hangar sprites
			},
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Icons
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Cursors
			{ 0, 0, 0, 0 }, // Strings
			0, AIRTYPES_NONE, AIRTYPES_NONE, 0, 0, 0, 0, AirTypeLabelList(), 0, 0,
			AIRTYPES_NONE, AIRTYPES_NONE, 0,
			{}, {}, 0, 0, 0, 0, 0, false, false };
	for (; i < lengthof(_airtypes); i++) _airtypes[i] = empty_airtype;
}

/**
 * Compare airtypes based on their sorting order.
 * @param first  The airtype to compare to.
 * @param second The airtype to compare.
 * @return True iff the first should be sorted before the second.
 */
static bool CompareAirTypes(const AirType &first, const AirType &second)
{
	return GetAirTypeInfo(first)->sorting_order < GetAirTypeInfo(second)->sorting_order;
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

	_sorted_airtypes.clear();
	for (AirType at = AIRTYPE_BEGIN; at != AIRTYPE_END; at++) {
		if (_airtypes[at].label != 0 && !HasBit(_airtypes_hidden_mask, at)) {
			_sorted_airtypes.push_back(at);
		}
	}
	std::sort(_sorted_airtypes.begin(), _sorted_airtypes.end(), CompareAirTypes);

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
			ati->alternate_labels.clear();

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

/**
 * Set the new airport layout, rotation and airtype of old airports
 * (even those built with OpenGRF+Airports).
 * @param st Station with the airport to update, if there is one.
 */
void ConvertOldAirportData(Station *st) {
	if (st->airport.tile == INVALID_TILE) return;

	st->airport.air_type = INVALID_AIRTYPE;

	if (st->airport.type < NEW_AIRPORT_OFFSET) return;

	if (st->airport.type == NEW_AIRPORT_OFFSET + 1) {
		/* Already existing water airports built on land cannot be converted properly.
		 * Handle them as standard small gravel airports. */
		st->airport.type = NEW_AIRPORT_OFFSET;
	}

	st->airport.layout   = 0;

	/* Old rotation dir values are 0, 2, 4, 6.
	 * Convert them to 0, 1, 2, 3 (DiagDirection values). */
	assert(st->airport.rotation % 2 == 0);
	assert(st->airport.rotation <= 6);
	st->airport.rotation = (Direction)(st->airport.rotation / 2);

	if (st->airport.type > 23) {
		/* OpenGRF+Airports includes airport types from NEW_AIRPORT_OFFSET to 23.
		 * Do not try to convert a completely unknown airport type. */
		Debug(misc, 0, "Unknown airport type: station {} airport type {}", st->index, st->airport.type);
		NOT_REACHED();
	}
}

/**
 * After loading an old savegame, update type and tracks of airport tiles.
 */
void AfterLoadSetAirportTileTypes()
{
	for (Station *st : Station::Iterate()) {
		ConvertOldAirportData(st);
		st->LoadAirportTilesFromSpec(st->airport, (DiagDirection)st->airport.rotation, st->airport.air_type);
	}
}

/** Clear data about infrastructure of airport */
void Station::ClearAirportDataInfrastructure() {
	this->airport.Clear();
	this->airport.air_type = INVALID_AIRTYPE;
	this->airport.aprons.clear();
	this->airport.helipads.clear();
	this->airport.runways.clear();
	if (this->airport.HasHangar()) {
		this->airport.hangar->depot_tiles.clear();
	}
}

void UpdateTracks(TileIndex tile)
{
	assert(IsAirportTile(tile));
	if (!MayHaveAirTracks(tile) || IsHangar(tile)) return;
	TrackBits tracks = GetAllowedTracks(tile) & GetAirportTileTracks(tile);
	if (tracks != GetAirportTileTracks(tile)) {
		Debug(misc, 0, "Removing invalid track on tile {}", tile);
	}

	SetAirportTileTracks(tile, tracks);
}

/**
 * Get the start or end of a runway.
 * @param tile Tile belonging a runway.
 * @param dir Direction to follow.
 * @return a runway extreme.
 */
TileIndex GetRunwayExtreme(TileIndex tile, DiagDirection dir)
{
	assert(IsAirportTile(tile) && IsRunway(tile));

	TileIndexDiff delta = TileOffsByDiagDir(dir);
	TileIndex t = tile;

	for (;;) {
		assert(IsAirportTile(t));
		assert(IsRunway(t));
		if (IsRunwayExtreme(t)) {
			DiagDirection last_dir = GetRunwayExtremeDirection(t);
			if (IsRunwayEnd(t)) {
				if (last_dir == dir) return t;
				assert(last_dir == ReverseDiagDir(dir));
			} else {
				assert(IsRunwayStart(t));
				if (last_dir == ReverseDiagDir(dir)) return t;
				assert(last_dir == dir);
			}
		}
		t += delta;
	}
}

/**
 * Check if a tile is a valid continuation of a runway.
 * The tile \a test_tile is a valid continuation to \a runway, if all of the following are true:
 * \li \a test_tile is an airport tile
 * \li \a test_tile and \a start_tile are in the same station
 * \li the tracks on \a test_tile and \a start_tile are in the same direction
 * @param test_tile Tile to test
 * @param start_tile Depot tile to compare with
 * @pre IsAirport && IsRunwayStart(start_tile)
 * @return true if the two tiles are compatible
 */
inline bool IsCompatibleRunwayTile(TileIndex test_tile, TileIndex start_tile)
{
	assert(IsAirportTile(start_tile) && IsRunwayStart(start_tile));
	return IsAirportTile(test_tile) &&
			GetStationIndex(test_tile) == GetStationIndex(start_tile) &&
			(GetRunwayTrackdirs(start_tile) & GetRunwayTrackdirs(test_tile)) != TRACKDIR_BIT_NONE;
}

/**
 * Get the runway length.
 * @param tile Runway start tile
 * @return the length of the runway.
 */
uint GetRunwayLength(TileIndex tile)
{
	assert(IsAirport(tile) && IsRunwayStart(tile));
	DiagDirection dir = GetRunwayExtremeDirection(tile);
	assert(IsValidDiagDirection(dir));

	uint length = 1;
	[[maybe_unused]] TileIndex start_tile = tile;

	do {
		length++;
		tile += TileOffsByDiagDir(dir);
		assert(IsCompatibleRunwayTile(tile, start_tile));
	} while (!IsRunwayEnd(tile));

	return length;
}

/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(start)
 * @param tile starting tile of the platform
 * @param b the state the reservation should be set to
 */
void SetRunwayReservation(TileIndex tile, bool b)
{
	assert(IsRunwayExtreme(tile));
	DiagDirection runway_dir = GetRunwayExtremeDirection(tile);
	if (IsRunwayEnd(tile)) runway_dir = ReverseDiagDir(runway_dir);
	TileIndexDiff diff = TileOffsByDiagDir(runway_dir);

	do {
		assert(IsAirportTile(tile));
		assert(!HasAirportTrackReserved(tile));
		SetReservationAsRunway(tile, b);
		if (_show_airport_tracks) MarkTileDirtyByTile(tile);
		tile = TileAdd(tile, diff);
	} while (!IsRunwayExtreme(tile));

	SetReservationAsRunway(tile, b);
	if (_show_airport_tracks) MarkTileDirtyByTile(tile);
}

/**
 * Get the position in the tile spec of a tile of a tilearea.
 * @param tile one tile of the area of the airport.
 * @param tile_area of the airport.
 * @param rotation the rotation of the airport.
 */
uint RotatedAirportSpecPosition(const TileIndex tile, const TileArea tile_area, const DiagDirection rotation)
{
	/* Get the tile difference between current tile and northern tile of the airport.
	 * @revise the function TileIndexToTileIndexDiffC as it seems to return the difference
	 * between the first tile minus the second one (one would expect to be the opposite).
	 */
	TileIndexDiffC tile_diff = TileIndexToTileIndexDiffC(tile, tile_area.tile);

	switch (rotation) {
		case 0:
			break;
		case 1:
			tile_diff = {(int16_t)(tile_area.h - 1 - tile_diff.y), (int16_t)tile_diff.x};
			break;
		case 2:
			tile_diff = {(int16_t)(tile_area.w - 1 - tile_diff.x), (int16_t)(tile_area.h - 1 - tile_diff.y)};
			break;
		case 3:
			tile_diff = {(int16_t)tile_diff.y, (int16_t)(tile_area.w - 1 - tile_diff.x)};
			break;
		default: NOT_REACHED();
	}

	return tile_diff.x + tile_diff.y * (rotation % 2 == 0 ? tile_area.w : tile_area.h);
}

/**
 * Rotate the trackbits as indicated by a direction
 * @param track_bits to rotate.
 * @param dir indicating how to rotate track_bits.
 *             (0 -> no rotation,
 *              1 -> 90 clockwise,
 *              2 -> 180 clockwise,
 *              3 -> 270 clockwise).
 * @return the rotated trackbits.
 */
TrackBits RotateTrackBits(TrackBits track_bits, DiagDirection dir)
{
	static const TrackBits rotation_table[DIAGDIR_END][TRACK_END] = {
		{ TRACK_BIT_X, TRACK_BIT_Y, TRACK_BIT_UPPER, TRACK_BIT_LOWER, TRACK_BIT_LEFT,  TRACK_BIT_RIGHT },
		{ TRACK_BIT_Y, TRACK_BIT_X, TRACK_BIT_RIGHT, TRACK_BIT_LEFT,  TRACK_BIT_UPPER, TRACK_BIT_LOWER },
		{ TRACK_BIT_X, TRACK_BIT_Y, TRACK_BIT_LOWER, TRACK_BIT_UPPER, TRACK_BIT_RIGHT, TRACK_BIT_LEFT  },
		{ TRACK_BIT_Y, TRACK_BIT_X, TRACK_BIT_LEFT,  TRACK_BIT_RIGHT, TRACK_BIT_LOWER, TRACK_BIT_UPPER }
	};

	TrackBits rotated = TRACK_BIT_NONE;
	for (Track track : SetTrackBitIterator(track_bits)) {
		rotated |= rotation_table[dir][track];
	}

	return rotated;
}

void Station::LoadAirportTilesFromSpec(TileArea ta, DiagDirection rotation, AirType airtype)
{
	if (this->airport.tile == INVALID_TILE) return;

	const AirportSpec *as = this->airport.GetSpec();
	if (airtype == INVALID_AIRTYPE) airtype = as->airtype;
	this->airport.air_type = airtype;

	for (Tile t : ta) {
		uint pos = RotatedAirportSpecPosition(t, ta, rotation);
		const AirportTileTable *airport_tile_desc = &as->layouts[this->airport.layout].tiles[pos];
		if (airport_tile_desc->type == ATT_INVALID) continue;
		assert(this->TileBelongsToAirport(t));

		t.m5() = 0;

		SetStationType(t, STATION_AIRPORT);
		SetAirType(t, airtype);
		SetAirportTileType(t, airport_tile_desc->type);

		bool airtype_gfx = airtype != as->airtype || airport_tile_desc->gfx[rotation] == INVALID_AIRPORTTILE;
		SetAirGfxType(t, airtype_gfx);
		SetTileAirportGfx(t, airtype_gfx ? airport_tile_desc->at_gfx : airport_tile_desc->gfx[rotation]);

		switch (GetAirportTileType(t)) {
			case ATT_INFRASTRUCTURE_WITH_CATCH:
			case ATT_INFRASTRUCTURE_NO_CATCH:
				SetAirportTileRotation(t, (DiagDirection)((rotation + airport_tile_desc->dir) % DIAGDIR_END));
				if (!airtype_gfx) SetAirportGfxForAirtype(t, airport_tile_desc->at_gfx);
				break;

			case ATT_SIMPLE_TRACK:
				break;
			case ATT_HANGAR_STANDARD:
			case ATT_HANGAR_EXTENDED:
				SetHangarDirection(t, RotateDiagDir(airport_tile_desc->dir, rotation));
				break;

			case ATT_APRON_NORMAL:
			case ATT_APRON_HELIPAD:
			case ATT_APRON_HELIPORT:
			case ATT_APRON_BUILTIN_HELIPORT:
				SetAirportTileRotation(t, (DiagDirection)((rotation + airport_tile_desc->dir) % DIAGDIR_END));
				break;

			case ATT_RUNWAY_MIDDLE:
				SB(t.m8(), 12, 2, RotateDirection(airport_tile_desc->runway_directions, rotation));
				break;

			case ATT_RUNWAY_START_NO_LANDING:
			case ATT_RUNWAY_START_ALLOW_LANDING:
			case ATT_RUNWAY_END:
				SB(t.m8(), 12, 2, RotateDiagDir(airport_tile_desc->dir, rotation));
				break;
			case ATT_WAITING_POINT:
				NOT_REACHED();
			default: NOT_REACHED();
		}

		if (!IsInfrastructure(t)) {
			SetAirportTileTracks(t, RotateTrackBits(airport_tile_desc->trackbits, rotation));
		}
	}

	this->UpdateAirportDataStructure();
}

/** Update cached variables after loading a game or modifying an airport */
void Station::UpdateAirportDataStructure()
{
	this->ClearAirportDataInfrastructure();

	/* Recover the airport area tile rescanning the rect of the station */
	TileArea ta(TileXY(this->rect.left, this->rect.top), TileXY(this->rect.right, this->rect.bottom));
	/* At the same time, detect any hangar. */
	TileIndex first_hangar = INVALID_TILE;

	TileArea airport_area;
	for (TileIndex t : ta) {
		if (!this->TileBelongsToAirport(t)) continue;
		airport_area.Add(t);

		if (first_hangar != INVALID_TILE) continue;

		if (this->airport.air_type == INVALID_AIRTYPE) this->airport.air_type = GetAirType(t);

		assert(this->airport.air_type == GetAirType(t));

		if (IsHangar(t)) first_hangar = t;
	}

	/* Set/Clear depot. */
	if (first_hangar != INVALID_TILE && this->airport.hangar == nullptr) {
		if (!Depot::CanAllocateItem()) NOT_REACHED();
		this->airport.hangar = new Depot(first_hangar, VEH_AIRCRAFT, GetTileOwner(first_hangar), this);
		this->airport.hangar->build_date = this->build_date;
		this->airport.hangar->town = this->town;
		SetBit(this->airport.flags, AFB_HANGAR);
	} else if (this->airport.hangar != nullptr) {
		if (first_hangar == INVALID_TILE) {
			ClrBit(this->airport.flags, AFB_HANGAR);
			if (this->airport.hangar->IsInUse()) {
				this->airport.hangar->Disuse();
			}
		} else {
			SetBit(this->airport.flags, AFB_HANGAR);
			if (!this->airport.hangar->IsInUse()) {
				/* Reuse current hangar. */
				this->airport.hangar->Reuse(first_hangar);
			}
		}
	}

	if (airport_area.tile == INVALID_TILE) return;

	if (this->airport.hangar != nullptr) this->airport.hangar->r_types.air_types = AIRTYPES_NONE;

	bool allow_landing = false;
	for (TileIndex t : airport_area) {
		if (!this->TileBelongsToAirport(t)) continue;
		this->airport.Add(t);

		assert(this->airport.air_type == GetAirType(t));

		if (!MayHaveAirTracks(t)) continue;

		UpdateTracks(t);

		switch (GetAirportTileType(t)) {
			case ATT_HANGAR_STANDARD:
			case ATT_HANGAR_EXTENDED:
				assert(this->airport.HasHangar());
				this->airport.hangar->depot_tiles.emplace_back(t);
				this->airport.hangar->xy = t;
				this->airport.hangar->r_types.air_types |= (AirTypes)(1 << this->airport.air_type);
				break;

			case ATT_APRON_NORMAL:
				this->airport.aprons.emplace_back(t);
				break;
			case ATT_APRON_HELIPAD:
				this->airport.helipads.emplace_back(t);
				break;
			case ATT_APRON_HELIPORT:
			case ATT_APRON_BUILTIN_HELIPORT:
				this->airport.heliports.emplace_back(t);
				break;

			case ATT_RUNWAY_START_ALLOW_LANDING:
				allow_landing = true;
				[[fallthrough]];
			case ATT_RUNWAY_START_NO_LANDING:
				this->airport.runways.emplace_back(t);
				break;

			default: break;
		}
	}

	if (this->airport.hangar != nullptr) InvalidateWindowData(WC_BUILD_VEHICLE, this->airport.hangar->index);

	if (this->airport.HasLandingRunway() != allow_landing) {
		ToggleBit(this->airport.flags, AFB_LANDING_RUNWAY);
	}
}

/**
 * Return the tracks a tile could have.
 * It returns the tracks the tile has plus the extra tracks that
 * could also exist on the tile.
 * @param tile
 * @return The tracks the tile could have.
 */
TrackBits GetAllowedTracks(TileIndex tile)
{
	assert(IsAirportTile(tile));
	switch (GetAirportTileType(tile)) {
		case ATT_INFRASTRUCTURE_NO_CATCH:
		case ATT_INFRASTRUCTURE_WITH_CATCH:
			return TRACK_BIT_NONE;

		case ATT_HANGAR_STANDARD:
		case ATT_HANGAR_EXTENDED:
			return HasBit(Tile(tile).m8(), 15) ? TRACK_BIT_Y: TRACK_BIT_X;

		case ATT_APRON_HELIPORT:
		case ATT_APRON_BUILTIN_HELIPORT:
			return TRACK_BIT_CROSS;

		case ATT_APRON_NORMAL:
		case ATT_APRON_HELIPAD:
		case ATT_SIMPLE_TRACK:
		case ATT_RUNWAY_MIDDLE:
		case ATT_RUNWAY_END:
		case ATT_RUNWAY_START_NO_LANDING:
		case ATT_RUNWAY_START_ALLOW_LANDING: {
			TrackBits tracks = TRACK_BIT_ALL;

			const TrackBits rem_tracks[] = {
				~TRACK_BIT_UPPER,
				~(TRACK_BIT_UPPER | TRACK_BIT_RIGHT),
				~TRACK_BIT_RIGHT,
				~(TRACK_BIT_LOWER | TRACK_BIT_RIGHT),
				~TRACK_BIT_LOWER,
				~(TRACK_BIT_LOWER | TRACK_BIT_LEFT),
				~TRACK_BIT_LEFT,
				~(TRACK_BIT_UPPER | TRACK_BIT_LEFT),
			};

			for (Direction dir = DIR_BEGIN; dir < DIR_END; dir++) {
				TileIndex t = TileAddByDir(tile, dir);
				if (!IsValidTile(t) || !IsAirportTile(t) ||
					GetStationIndex(t) != GetStationIndex(tile) || !MayHaveAirTracks(t)) {
					tracks &= rem_tracks[dir];
					} else if (IsHangar(t)) {
						tracks &= rem_tracks[dir];
					}
			}

			return tracks;
		}

		default: NOT_REACHED();
	}
}

/**
 * Get the sprite for an airport tile.
 * @param t Tile to get the sprite of.
 * @return AirportTile ID.
 */
StationGfx GetAirportGfx(TileIndex t)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	if (!HasAirtypeGfx(t)) return GetTranslatedAirportTileID(GetTileAirportGfx(t));

	switch (GetAirportTileType(t)) {
		case ATT_INFRASTRUCTURE_NO_CATCH:
		case ATT_INFRASTRUCTURE_WITH_CATCH:
			return GetTileAirportGfx(t);

		case ATT_SIMPLE_TRACK:
			return (StationGfx)0;

		case ATT_HANGAR_STANDARD:
		case ATT_HANGAR_EXTENDED:
			return (StationGfx)0;

		case ATT_APRON_NORMAL:
		case ATT_APRON_HELIPAD:
		case ATT_APRON_HELIPORT:
		case ATT_APRON_BUILTIN_HELIPORT:
			switch (GetApronType(t)) {
				case APRON_APRON:
				case APRON_HELIPAD:
				case APRON_HELIPORT:
					return (StationGfx)0;
				case APRON_BUILTIN_HELIPORT:
					return (StationGfx)0; // oil rig heliport
				default: NOT_REACHED();
			}

		case ATT_RUNWAY_MIDDLE:
		case ATT_RUNWAY_START_NO_LANDING:
		case ATT_RUNWAY_START_ALLOW_LANDING:
		case ATT_RUNWAY_END: {
			return (StationGfx)0;
			break;
		}

		case ATT_WAITING_POINT:
			NOT_REACHED();
		default:
			NOT_REACHED();
	}
}

