/* $Id: pbs_air.cpp $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs_air.cpp Path based system routines for air vehicles. */

#include "stdafx.h"
#include "viewport_func.h"
#include "vehicle_func.h"
#include "pathfinder/follow_track.hpp"
#include "pbs_air.h"
#include "depot_base.h"
#include "aircraft.h"
#include "air_map.h"

#include "table/airport_translation.h"

/**
 * After loading an old savegame, update type and tracks of airport tiles
 * @todo rotated airports
 */
void AfterLoadSetAirportTileTypes()
{
	Station *st;
	FOR_ALL_STATIONS(st) st->TranslateAirport();
}

/** Clear data about infrastructure of airport */
void Station::ClearAirportDataInfrastructure() {
	delete [] this->airport.footprint;
	this->airport.footprint = NULL;
	this->airport.tile = INVALID_TILE;
	this->airport.air_type = INVALID_AIRTYPE;
	this->airport.terminals.Clear();
	this->airport.helipads.Clear();
	this->airport.runways.Clear();
	if (this->airport.depot_id != INVALID_DEPOT) {
		assert(Depot::IsValidID(this->airport.depot_id));
		Depot::GetIfValid(this->airport.depot_id)->depot_tiles.Clear();
	}
}

void UpdateTracks(TileIndex tile)
{
	assert(IsAirportTile(tile));
	if (!MayHaveAirTracks(tile) || IsHangar(tile)) return;
	TrackBits tracks = GetAllowedTracks(tile) & GetAirportTileTracks(tile);
	if (tracks != GetAirportTileTracks(tile)) {
		DEBUG(misc, 0, "Removing invalid track on tile %d", tile);
	}

	SetAirportTileTracks(tile, tracks);
}

void Station::TranslateAirport()
{
	if (this->airport.tile == INVALID_TILE) return;

	this->airport.air_type = _translation_airport_specs[this->airport.type][0].ground;

	uint iter = 0;
	TILE_AREA_LOOP(t, this->airport) { // Default airports are rectangular.
		const TileTranslation *translation = &_translation_airport_specs[this->airport.type][iter];
		assert(translation->ground == this->airport.air_type);

		SetAirportType(t, translation->ground);
		SetAirportTileType(t, translation->type);

		_m[t].m4 = 0;
		_m[t].m5 = 0;

		if (!IsInfrastructure(t)) {
			if (IsHangar(t)) {
				SetHangarDirection(t, translation->dir);
			}

			SetAirportTileTracks(t, translation->trackbits);
		}

		switch (GetAirportTileType(t)) {
			case ATT_INFRASTRUCTURE:
				_m[t].m5 = translation->gfx_id;
				SetCatchmentAirportType(t, translation->catchment);
				break;

			case ATT_SIMPLE_TRACK:
			case ATT_HANGAR:
				break;

			case ATT_TERMINAL:
				SetTerminalType(t, translation->terminal_type);
				break;

			case ATT_RUNWAY:
				SB(_m[t].m4, 4, 2, translation->runway_tracks);
				break;

			case ATT_RUNWAY_START:
			case ATT_RUNWAY_END:
				SB(_m[t].m4, 4, 2, translation->dir);
				SB(_m[t].m4, 7, 1, translation->landing);
				break;
			default: NOT_REACHED();
		}

		iter++;
	}

	this->UpdateAirportDataStructure();
}

/** Update cached variables after loading a game or modifiying an airport */
void Station::UpdateAirportDataStructure()
{
	this->ClearAirportDataInfrastructure();

	/* Recover the airport area tile rescaning the rect of the station */
	TileArea ta(TileXY(this->rect.left, this->rect.top), TileXY(this->rect.right, this->rect.bottom));
	/* At the same time, detect any hangar. */
	TileIndex first_hangar = INVALID_TILE;

	TILE_AREA_LOOP(t, ta) {
		if (!this->TileBelongsToAirport(t)) continue;
		this->airport.Add(t);

		if (first_hangar != INVALID_TILE) continue;

		if (this->airport.air_type == INVALID_AIRTYPE) this->airport.air_type = GetAirportType(t);

		assert(this->airport.air_type == GetAirportType(t));

		if (IsHangar(t)) first_hangar = t;
	}

	/* Set/Clear depot. */
	if (first_hangar != INVALID_TILE && this->airport.depot_id == INVALID_DEPOT) {
		if (!Depot::CanAllocateItem()) NOT_REACHED();
		Depot *dep = new Depot(first_hangar);
		this->airport.depot_id = dep->index;
		dep->build_date = this->build_date;
		dep->company = GetTileOwner(dep->xy);
		dep->veh_type = VEH_AIRCRAFT;
		dep->town = this->town;
	} else if (first_hangar == INVALID_TILE && this->airport.depot_id != INVALID_DEPOT) {
		assert(Depot::IsValidID(this->airport.depot_id));
		DeleteWindowById(WC_VEHICLE_DEPOT, this->airport.depot_id);
		delete Depot::Get(this->airport.depot_id);
		this->airport.depot_id = INVALID_DEPOT;
	}

	uint size = this->airport.w * this->airport.h;
	if (size == 0) return;

	this->airport.footprint = NewBitMap(size);
	Depot *dep = Depot::GetIfValid(this->airport.depot_id);

	int iter = -1;
	TILE_AREA_LOOP(t, this->airport) {
		iter++;

		SetBitMapBit(this->airport.footprint, iter, this->TileBelongsToAirport(t));

		if (!this->TileBelongsToAirport(t)) continue;

		assert(this->airport.air_type == GetAirportType(t));

		if (!MayHaveAirTracks(t)) continue;

		UpdateTracks(t);

		switch (GetAirportTileType(t)) {
			case ATT_HANGAR:
				assert(dep != NULL);
				*dep->depot_tiles.Append() = t;
				break;

			case ATT_TERMINAL:
				switch (GetTerminalType(t)) {
					case HTT_TERMINAL:
						*this->airport.terminals.Append() = t;
						break;
					default:
						*this->airport.helipads.Append() = t;
						break;
				}
				break;

			case ATT_RUNWAY_START:
				*this->airport.runways.Append() = t;
				break;

			default: break;
		}
	}
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
		case ATT_INFRASTRUCTURE:
			return TRACK_BIT_NONE;

		case ATT_HANGAR:
			return HasBit(_m[tile].m4, 4) ? TRACK_BIT_Y: TRACK_BIT_X;

		case ATT_TERMINAL:
			if (GetTerminalType(tile) == HTT_HELIPORT) {
				return TRACK_BIT_NONE;
			}
			/* Fall through... */
		case ATT_SIMPLE_TRACK:
		case ATT_RUNWAY:
		case ATT_RUNWAY_END:
		case ATT_RUNWAY_START: {
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
				TileIndex t = TILE_ADD(tile, TileOffsByDir(dir));
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
 * When looking for aircraft paths, crossing a not-diagonal track
 * may put the aircraft too close to another aircraft crossing the
 * equivalent track on a neighbour tile (or getting too close to a hangar).
 * The function checks the tile that can cause problems and whether the associated track
 * on the neighbour tile is already reserved.
 * @param tile Tile to check.
 * @param track Involved track on tile.
 * @return The associated tile can be crossed, is of the same station and is not reserved.
 * @pre IsAirportTile
 * @todo revise move to somewhere else. do not inline
 */
bool CheckFreeAssociatedAirportTile(TileIndex tile, Track track)
{
	assert(IsAirportTile(tile));

	if (IsDiagonalTrack(track)) return true;

	Direction dir;
	switch (track) {
		case TRACK_UPPER:
			dir = DIR_N;
			track = TRACK_LOWER;
			break;
		case TRACK_LOWER:
			dir = DIR_S;
			track = TRACK_UPPER;
			break;
		case TRACK_LEFT:
			dir = DIR_W;
			track = TRACK_RIGHT;
			break;
		case TRACK_RIGHT:
			dir = DIR_E;
			track = TRACK_LEFT;
			break;
		default: NOT_REACHED();
	}

	TileIndex neighbour =  TILE_ADD(tile, TileOffsByDir(dir));

	if (!IsValidTile(neighbour)) return false;
	if (!IsAirportTileOfStation(neighbour, GetStationIndex(tile))) return false;
	if (!MayHaveAirTracks(neighbour)) return false;
	if (IsHangar(neighbour)) return false;
	if (IsRunway(neighbour) && GetReservationAsRunway(neighbour)) return false;

	return HasAirportTrackReserved(neighbour, track);
}

/**
 * Reserve a track of a tile.
 * @param t Tile where to reserve the (airport) track.
 * @param b Track to reserve.
 * @return True if the track has been reserved.
 */
bool TryAirportTrackReservation(TileIndex t, Track track)
{
	assert(IsAirportTile(t));
	assert(MayHaveAirTracks(t));
	assert(track != INVALID_TRACK);

	TrackBits trackbits = GetReservedAirportTracks(t);
	if (trackbits & TrackToTrackBits(track)) return false;

	trackbits |= TrackToTrackBits(track);

	SetAirportTracksReservation(t, trackbits);
	MarkTileDirtyByTile(t);
	return true;
}

/**
 * Lift an airport track reservation.
 * @param tile Tile with reserved track.
 * @param track Track to remove.
 * @return True if track reservation has been removed, false otherwise.
 * @pre IsAirportTile
 */
bool RemoveAirportTrackReservation(TileIndex t, Track track)
{
	assert(IsAirportTile(t));
	assert(MayHaveAirTracks(t));

	TrackBits trackbits = GetReservedAirportTracks(t);
	TrackBits removing_track = TrackToTrackBits(track);

	if (!(trackbits & removing_track)) return false;

	trackbits &= ~removing_track;
	SetAirportTracksReservation(t, trackbits);
	MarkTileDirtyByTile(t);

	return true;
}

/**
 * Is any track already reserved?
 * @param tile Tile to check.
 * @return True if there is a track reserved on \a tile, false if none is reserved.
 * @pre IsAirportTile
 */
bool HasAirportTrackReservation(TileIndex t)
{
	assert(IsAirportTile(t));
	assert(MayHaveAirTracks(t));
	return GetReservedAirportTracks(t) != TRACK_BIT_NONE;
}

/**
 * Are some of this tracks reserved?
 * @param t Tile to check.
 * @param tracks Tracks to check.
 * @return True if any of the given tracks \a tracks is reserved on tile \a t.
 */
bool HasAirportTracksReserved(TileIndex t, TrackBits tracks)
{
	assert(IsAirportTile(t));
	assert(MayHaveAirTracks(t));
	return (GetReservedAirportTracks(t) & tracks) != TRACK_BIT_NONE;
}

/**
 * Check if a track collides with the air tracks reserved on a tile.
 * @param t Tile to check.
 * @param track Track to check.
 * @return True if the track can be reserved on tile \a t without colliding other reserved paths.
 * @note It doesn't check next tile.
 */
bool AirTrackCollidesTrackReservation(TileIndex t, Track track)
{
	assert(MayHaveAirTracks(t));
	return TrackOverlapsTracks(GetReservedAirportTracks(t), track);
}

/**
 * Check if a tile can be reserved and does not collide with another path on next tile.
 * @param tile The tile.
 * @param trackdir The trackdir to check.
 * @return True if reserving track direction \a trackdir on tile \a tile
 *         doesn't collide with other paths.
 */
bool IsAirportPositionFree(TileIndex tile, Trackdir trackdir)
{
	CFollowTrackAirport ft(INVALID_COMPANY);

	assert(IsAirportTile(tile));
	assert(MayHaveAirTracks(tile));

	Track track = TrackdirToTrack(trackdir);

	/* Tile reserved? Can never be a free waiting position. */
	if (AirTrackCollidesTrackReservation(tile, track)) return false;

	if (!ft.Follow(tile, trackdir)) {
		if (!IsDiagonalTrack(track)) {
			return false;
		}
		return true;
	}

	/* Check for reachable tracks.
	 * Don't discard 90deg turns as we don't want two paths to collide
	 * even if they cannot really collide because of a 90deg turn */
	ft.m_new_td_bits &= DiagdirReachesTrackdirs(ft.m_exitdir);

	if (HasAirportTracksReserved(ft.m_new_tile, TrackdirBitsToTrackBits(ft.m_new_td_bits))) return false;

	return CheckFreeAssociatedAirportTile(ft.m_new_tile, track);
}

/**
 * Follow a reservation starting from a specific tile to the end.
 * @param o Owner (unused)
 * @param ats Compatible air types.
 * @param tile Starting tile.
 * @param trackdir Trackdir on \a tile .
 */
static PBSTileInfo FollowReservation(Owner o, RailTypes ats, TileIndex tile, Trackdir trackdir, bool ignore_oneway = false)
{
	assert(HasAirportTracksReserved(tile, TrackToTrackBits(TrackdirToTrack(trackdir))));

	CFollowTrackAirport ft(o, ats);
	while (ft.Follow(tile, trackdir)) {
		TrackdirBits reserved = ft.m_new_td_bits & TrackBitsToTrackdirBits(GetReservedAirportTracks(ft.m_new_tile));

		tile = ft.m_new_tile;
		trackdir =  FindFirstTrackdir(reserved);
	}

	return PBSTileInfo(tile, trackdir, false);
}

/**
 * Helper struct for finding the best matching vehicle on a specific track.
 */
struct FindAircraftOnTrackInfo {
	PBSTileInfo res; ///< Information about the track.
	Aircraft *best;     ///< The currently "best" vehicle we have found.

	/** Init the best location to NULL always! */
	FindAircraftOnTrackInfo() : best(NULL) {}
};

/** Callback for Has/FindVehicleOnPos to find an aircraft on a specific track. */
static Vehicle *FindAircraftOnTrackEnum(Vehicle *v, void *data)
{
	FindAircraftOnTrackInfo *info = (FindAircraftOnTrackInfo *)data;

	if (v->type != VEH_AIRCRAFT || (v->vehstatus & VS_CRASHED)) return NULL;

	Aircraft *a = Aircraft::From(v);
	if (!a->IsNormalAircraft()) return NULL;

	if (TrackdirToTrack(info->res.trackdir) == TrackdirToTrack(v->GetVehicleTrackdir())) {
		/* ALWAYS return the lowest ID (anti-desync!) */
		/* revise: can be omitted? */
		if (info->best == NULL || a->index < info->best->index) info->best = a;
		return a;
	}

	return NULL;
}

/**
 * Follow an aircraft reservation to the last tile.
 * @param v the vehicle
 * @param aircraft_on_res Is set to an aircraft we might encounter
 * @returns The last tile of the reservation or the current aircraft tile if no reservation present.
 */
PBSTileInfo FollowAircraftReservation(const Aircraft *v, Vehicle **aircraft_on_res)
{
	assert(v->type == VEH_AIRCRAFT);
	assert( Aircraft::From(v)->IsNormalAircraft());

	TileIndex tile = v->tile;
	Trackdir  trackdir = v->GetVehicleTrackdir();

	FindAircraftOnTrackInfo ftoti;
	//ftoti.res = FollowReservation(v->owner, GetRailTypeInfo(v->railtype)->compatible_railtypes, tile, trackdir);
	ftoti.res = FollowReservation(v->owner, (RailTypes)INVALID_AIRTYPES, tile, trackdir);
	ftoti.res.okay = IsSafeWaitingPosition(v, ftoti.res.tile, ftoti.res.trackdir);
	if (aircraft_on_res != NULL) {
		FindVehicleOnPos(ftoti.res.tile, &ftoti, FindAircraftOnTrackEnum);
		if (ftoti.best != NULL) *aircraft_on_res = ftoti.best->First();
	}
	return ftoti.res;
}

/**
 * Find the aircraft which has reserved a specific path.
 * @param tile A tile on the path.
 * @param track A reserved track on the tile.
 * @return The vehicle holding the reservation.
 * @note It will crash if stray path.
 */
Aircraft *GetAircraftForReservation(TileIndex tile, Track track)
{
	assert(HasAirportTracksReserved(tile, TrackToTrackBits(track)));
	Trackdir  trackdir = TrackToTrackdir(track);

	AirTypes ats = INVALID_AIRTYPES;

	/* Follow the path from tile to both ends.
	 * One of the end tiles should have an aircraft on it. */
	for (int i = 0; i < 2; ++i, trackdir = ReverseTrackdir(trackdir)) {
		FindAircraftOnTrackInfo ftoti;
		ftoti.res = FollowReservation(GetTileOwner(tile), (RailTypes)ats, tile, trackdir, true);

		FindVehicleOnPos(ftoti.res.tile, &ftoti, FindAircraftOnTrackEnum);
		if (ftoti.best != NULL) return ftoti.best;
	}

	NOT_REACHED();
}

/**
 * Determine whether a certain track on a tile is a safe position to end a path.
 * @param v The vehicle to test for.
 * @param tile The tile.
 * @param trackdir The trackdir to test (unused).
 * @return True if it is a safe position.
 */
bool IsSafeWaitingPosition(const Aircraft *v, TileIndex tile, Trackdir trackdir)
{
	assert(IsAirportTile(tile));
	assert(v != NULL);

	if (!IsDiagonalTrackdir(trackdir)) return false;

	switch (v->next_state) {
		case AM_HANGAR:
			return IsHangar(tile);
		case AM_TERMINAL:
			return IsTerminal(tile);
		case AM_HELIPAD:
			return IsHelipad(tile) || IsHeliport(tile);
		case AM_TAKEOFF:
			if (v->IsHelicopter()) return IsHelipad(tile) || IsHeliport(tile);
			return IsRunwayStart(tile); // Aircraft can take off in all runways.
		default: break;
	}

	return false;
}
