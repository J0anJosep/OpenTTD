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
#include "table/airport_movement.h"
#include "table/airporttile_ids.h"
#include "table/airport_defaults.h"

#include "safeguards.h"


/** Helper type for lists/vectors of trains */
typedef std::vector<Aircraft *> AircraftList;

AirtypeInfo _airtypes[AIRTYPE_END];
std::vector<AirType> _sorted_airtypes;
AirTypes _airtypes_hidden_mask;


void ResolveAirTypeGUISprites(AirtypeInfo *ati)
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

	static const AirtypeInfo empty_airtype = {
			{	0, // Ground sprite
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
 * Compare railtypes based on their sorting order.
 * @param first  The railtype to compare to.
 * @param second The railtype to compare.
 * @return True iff the first should be sorted before the second.
 */
static bool CompareAirTypes(const AirType &first, const AirType &second)
{
	return GetAirtypeInfo(first)->sorting_order < GetAirtypeInfo(second)->sorting_order;
}

/**
 * Resolve sprites of custom air types
 */
void InitAirTypes()
{
	for (AirType at = AIRTYPE_BEGIN; at != AIRTYPE_END; at++) {
		AirtypeInfo *ati = &_airtypes[at];
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
		AirtypeInfo *ati = &_airtypes[at];

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
 * After loading an old savegame, update type and tracks of airport tiles
 * @todo rotated airports
 */
void AfterLoadSetAirportTileTypes()
{
	for (Station *st : Station::Iterate()) st->LoadAirportTilesFromSpec(st->airport);
}

/** Clear data about infrastructure of airport */
void Station::ClearAirportDataInfrastructure() {
	this->airport.Clear();
	this->airport.air_type = INVALID_AIRTYPE;
	this->airport.aprons.clear();
	this->airport.helipads.clear();
	this->airport.runways.clear();
	if (this->airport.depot_id != INVALID_DEPOT) {
		assert(Depot::IsValidID(this->airport.depot_id));
		Depot::GetIfValid(this->airport.depot_id)->depot_tiles.clear();
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

void Station::LoadAirportTilesFromSpec(TileArea ta, DiagDirection rotation, AirType airtype)
{
	if (this->airport.tile == INVALID_TILE) return;

	const AirportSpec *as = this->airport.GetSpec();
	this->airport.air_type = airtype;

	uint iter = 0;
	for (TileIndex t : this->airport) {
		if (!this->TileBelongsToAirport(t)) continue;
		const TileDescription *airport_tile_desc = &_description_airport_specs[this->airport.type][iter];
		assert(airport_tile_desc->ground == this->airport.air_type);

		_m[t].m4 = 0;
		_m[t].m5 = 0;

		SetStationType(t, STATION_AIRPORT);
		SetAirtype(t, airport_tile_desc->ground);
		SetAirportTileType(t, airport_tile_desc->type);

		AirportTileType att = GetAirportTileType(t);
		switch (att) {
			case ATT_INFRASTRUCTURE_WITH_CATCH:
			case ATT_INFRASTRUCTURE_NO_CATCH:
				_me[t].m8 = airport_tile_desc->at;
				SetAirportTileRotation(t, (DiagDirection)((rotation + airport_tile_desc->dir) % DIAGDIR_END));
				break;

			case ATT_SIMPLE_TRACK:
				break;
			case ATT_HANGAR_STANDARD:
			case ATT_HANGAR_EXTENDED:
				SetHangarDirection(t, airport_tile_desc->dir);
				break;

			case ATT_APRON_NORMAL:
			case ATT_APRON_HELIPAD:
			case ATT_APRON_HELIPORT:
			case ATT_APRON_BUILTIN_HELIPORT:
				SetAirportTileRotation(t, (DiagDirection)((rotation + airport_tile_desc->dir) % DIAGDIR_END));
				break;

			case ATT_RUNWAY_MIDDLE:
				SB(_me[t].m8, 12, 2, airport_tile_desc->runway_directions);
				break;

			case ATT_RUNWAY_START_NO_LANDING:
			case ATT_RUNWAY_START_ALLOW_LANDING:
			case ATT_RUNWAY_END:
				SB(_me[t].m8, 12, 2, airport_tile_desc->dir);
				break;
			default: NOT_REACHED();
		}

		if (!IsInfrastructure(t)) {
			SetAirportTileTracks(t, airport_tile_desc->trackbits);
		}
		assert(GetAirportTileType(t) != ATT_WAITING_POINT);

		iter++;
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

		if (this->airport.air_type == INVALID_AIRTYPE) this->airport.air_type = GetAirtype(t);

		assert(this->airport.air_type == GetAirtype(t));

		if (IsHangar(t)) first_hangar = t;
	}

	/* Set/Clear depot. */
	if (first_hangar != INVALID_TILE && this->airport.depot_id == INVALID_DEPOT) {
		if (!Depot::CanAllocateItem()) NOT_REACHED();
		Depot *dep = new Depot(first_hangar, VEH_AIRCRAFT, GetTileOwner(first_hangar));
		this->airport.depot_id = dep->index;
		dep->build_date = this->build_date;
		dep->town = this->town;
	} else if (first_hangar == INVALID_TILE && this->airport.depot_id != INVALID_DEPOT) {
		assert(Depot::IsValidID(this->airport.depot_id));
		Depot *dep = Depot::Get(this->airport.depot_id);
		dep->Disuse();
		delete dep;
		this->airport.depot_id = INVALID_DEPOT;
	}

	if (airport_area.tile == INVALID_TILE) return;

	Depot *dep = Depot::GetIfValid(this->airport.depot_id);

	for (TileIndex t : airport_area) {
		if (!this->TileBelongsToAirport(t)) continue;
		this->airport.Add(t);

		assert(this->airport.air_type == GetAirtype(t));

		if (!MayHaveAirTracks(t)) continue;

		UpdateTracks(t);

		switch (GetAirportTileType(t)) {
			case ATT_HANGAR_STANDARD:
			case ATT_HANGAR_EXTENDED:
				assert(dep != nullptr);
				dep->depot_tiles.emplace_back(t);
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
			case ATT_RUNWAY_START_NO_LANDING:
				this->airport.runways.emplace_back(t);
				break;

			default: break;
		}
	}

	if (this->airport.depot_id != INVALID_DEPOT) InvalidateWindowData(WC_BUILD_VEHICLE, this->airport.depot_id);
}

/**
 * Return the tracks a tile could have.
 * It returns the tracks the tile has plus the extra tracks that
 * could also exist on the tile.
 * @param tile
 * @return The tracks the tile could have.
 * @todo revise move to somewhere else. do not inline
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
			return HasBit(_me[tile].m8, 15) ? TRACK_BIT_Y: TRACK_BIT_X;

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
AirportTiles GetAirportGfx(TileIndex t)
{
	assert(IsTileType(t, MP_STATION));
	assert(IsAirport(t));

	AirportTileType att = GetAirportTileType(t);
	assert(GetAirportTileType(t) != ATT_WAITING_POINT);

	switch (att) {
		case ATT_INFRASTRUCTURE_NO_CATCH:
		case ATT_INFRASTRUCTURE_WITH_CATCH:
			return GetAirportGfxFromTile(t);

		case ATT_SIMPLE_TRACK:
			return (AirportTiles)0;

		case ATT_HANGAR_STANDARD:
		case ATT_HANGAR_EXTENDED:
			return (AirportTiles)0;

		case ATT_APRON_NORMAL:
		case ATT_APRON_HELIPAD:
		case ATT_APRON_HELIPORT:
		case ATT_APRON_BUILTIN_HELIPORT:
			switch (GetApronType(t)) {
				case APRON_APRON:
				case APRON_HELIPAD:
				case APRON_HELIPORT:
					return (AirportTiles)0;
				case APRON_BUILTIN_HELIPORT:
					return (AirportTiles)0; // oil rig heliport
				default: NOT_REACHED();
			}

		case ATT_RUNWAY_MIDDLE:
		case ATT_RUNWAY_START_NO_LANDING:
		case ATT_RUNWAY_START_ALLOW_LANDING:
		case ATT_RUNWAY_END: {
			return (AirportTiles)0;
			break;
		}

		case ATT_WAITING_POINT:
			NOT_REACHED();
		default:
			NOT_REACHED();
	}
}

static uint16 AirportGetNofElements(const AirportFTAbuildup *apFA);
static AirportFTA *AirportBuildAutomata(uint nofelements, const AirportFTAbuildup *apFA);

const AirportSpec AirportSpec::dummy = {&_airportfta_dummy, nullptr, _default_airports_rotation, 0, nullptr, 0, 0, 0, 0, 0, MIN_YEAR, MIN_YEAR, STR_NULL, ATP_TTDP_LARGE, APC_BEGIN, 0, 0, false, GRFFileProps(AT_INVALID)};

/**
 * Rotate the airport moving data to another rotation.
 * @param orig Pointer to the moving data to rotate.
 * @param rotation How to rotate the moving data.
 * @param num_tiles_x Number of tiles in x direction.
 * @param num_tiles_y Number of tiles in y direction.
 * @return The rotated moving data.
 */
AirportMovingData RotateAirportMovingData(const AirportMovingData *orig, Direction rotation, uint num_tiles_x, uint num_tiles_y)
{
	AirportMovingData amd;
	amd.flag = orig->flag;
	amd.direction = ChangeDir(orig->direction, (DirDiff)rotation);
	switch (rotation) {
		case DIR_N:
			amd.x = orig->x;
			amd.y = orig->y;
			break;

		case DIR_E:
			amd.x = orig->y;
			amd.y = num_tiles_y * TILE_SIZE - orig->x - 1;
			break;

		case DIR_S:
			amd.x = num_tiles_x * TILE_SIZE - orig->x - 1;
			amd.y = num_tiles_y * TILE_SIZE - orig->y - 1;
			break;

		case DIR_W:
			amd.x = num_tiles_x * TILE_SIZE - orig->y - 1;
			amd.y = orig->x;
			break;

		default: NOT_REACHED();
	}
	return amd;
}

AirportFTAClass::AirportFTAClass(
	const AirportMovingData *moving_data_,
	const byte *terminals_,
	const byte num_helipads_,
	const byte *entry_points_,
	Flags flags_,
	const AirportFTAbuildup *apFA,
	byte delta_z_
) :
	moving_data(moving_data_),
	terminals(terminals_),
	num_helipads(num_helipads_),
	flags(flags_),
	nofelements(AirportGetNofElements(apFA)),
	entry_points(entry_points_),
	delta_z(delta_z_)
{
	/* Build the state machine itself */
	this->layout = AirportBuildAutomata(this->nofelements, apFA);
}

AirportFTAClass::~AirportFTAClass()
{
	for (uint i = 0; i < nofelements; i++) {
		AirportFTA *current = layout[i].next;
		while (current != nullptr) {
			AirportFTA *next = current->next;
			free(current);
			current = next;
		}
	}
	free(layout);
}

/**
 * Get the number of elements of a source Airport state automata
 * Since it is actually just a big array of AirportFTA types, we only
 * know one element from the other by differing 'position' identifiers
 */
static uint16 AirportGetNofElements(const AirportFTAbuildup *apFA)
{
	uint16 nofelements = 0;
	int temp = apFA[0].position;

	for (uint i = 0; i < MAX_ELEMENTS; i++) {
		if (temp != apFA[i].position) {
			nofelements++;
			temp = apFA[i].position;
		}
		if (apFA[i].position == MAX_ELEMENTS) break;
	}
	return nofelements;
}

/**
 * Construct the FTA given a description.
 * @param nofelements The number of elements in the FTA.
 * @param apFA The description of the FTA.
 * @return The FTA describing the airport.
 */
static AirportFTA *AirportBuildAutomata(uint nofelements, const AirportFTAbuildup *apFA)
{
	AirportFTA *FAutomata = MallocT<AirportFTA>(nofelements);
	uint16 internalcounter = 0;

	for (uint i = 0; i < nofelements; i++) {
		AirportFTA *current = &FAutomata[i];
		current->position      = apFA[internalcounter].position;
		current->heading       = apFA[internalcounter].heading;
		current->block         = apFA[internalcounter].block;
		current->next_position = apFA[internalcounter].next;

		/* outgoing nodes from the same position, create linked list */
		while (current->position == apFA[internalcounter + 1].position) {
			AirportFTA *newNode = MallocT<AirportFTA>(1);

			newNode->position      = apFA[internalcounter + 1].position;
			newNode->heading       = apFA[internalcounter + 1].heading;
			newNode->block         = apFA[internalcounter + 1].block;
			newNode->next_position = apFA[internalcounter + 1].next;
			/* create link */
			current->next = newNode;
			current = current->next;
			internalcounter++;
		}
		current->next = nullptr;
		internalcounter++;
	}
	return FAutomata;
}

/**
 * Get the finite state machine of an airport type.
 * @param airport_type %Airport type to query FTA from. @see AirportTypes
 * @return Finite state machine of the airport.
 */
const AirportFTAClass *GetAirport(const byte airport_type)
{
	if (airport_type == AT_DUMMY) return &_airportfta_dummy;
	return AirportSpec::Get(airport_type)->fsm;
}

/**
 * Get the vehicle position when an aircraft is build at the given tile
 * @param hangar_tile The tile on which the vehicle is build
 * @return The position (index in airport node array) where the aircraft ends up
 */
byte GetVehiclePosOnBuild(TileIndex hangar_tile)
{
	const Station *st = Station::GetByTile(hangar_tile);
	const AirportFTAClass *apc = st->airport.GetFTA();
	/* When we click on hangar we know the tile it is on. By that we know
	 * its position in the array of depots the airport has.....we can search
	 * layout for #th position of depot. Since layout must start with a listing
	 * of all depots, it is simple */
	for (uint i = 0;; i++) {
		return apc->layout[i].position;
	}
	NOT_REACHED();
}
