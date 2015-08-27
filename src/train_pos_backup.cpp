/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file train_pos_backup.cpp Handling of trains. */

#include "stdafx.h"
#include "error.h"
#include "news_func.h"
#include "company_func.h"
#include "strings_func.h"
#include "platform_func.h"
#include "depot_base.h"
#include "depot_map.h"
#include "train_pos_backup.h"

#include "table/strings.h"

#include "safeguards.h"


/**
 * Lift a train in a big depot: keep the positions of the elements of the chain if needed,
 * and keep also the original tile, direction and track.
 * @param train The train we want to lift.
 * @pre The train must be inside a big rail depot.
 *      (i.e. the track is 'valid track | TRACK_BIT_DEPOT' or just 'TRACK_BIT_DEPOT').
 */
void TrainPosBackup::LiftTrainInBigDepot(Train *train, DoCommandFlag flags)
{
	assert(train == NULL || train->IsInDepot());
	assert(train == NULL || IsRailDepotTile(train->tile));
	assert(this->unit_positions == NULL);
	assert(this->placed == false);

	/* Lift the train only if we have a train in a big depot. */
	if (train == NULL || !IsBigRailDepot(train->tile)) return;

	/* Save original data, so we can restore it if needed. */
	this->orig_tile = train->tile;
	this->orig_dir = train->direction;
	this->orig_track = train->track;

	/* If train is not placed... return, because train is already lifted.
	 * Remember that DC_AUTOREPLACE assumes train is already lifted. */
	if ((train->track & ~TRACK_BIT_DEPOT) == 0) {
		return;
	}

	assert((flags & DC_AUTOREPLACE) == 0);

	/* Train is placed in rails: lift it. */
	this->placed = true;
	this->unit_positions = new UnitPositions();

	for (Train *t = train; t != NULL; t = t->Next()) {
		// Backup.
		UnitPos *up = unit_positions->Append();

		up->tile = t->tile;
		up->x_pos = t->x_pos;
		up->y_pos = t->y_pos;
		up->z_pos = t->z_pos;
		up->direction = t->direction;
		// Lift.
		t->track = TRACK_BIT_DEPOT;
		t->tile = (*unit_positions)[0].tile;
		t->x_pos = (*unit_positions)[0].x_pos;
		t->y_pos = (*unit_positions)[0].y_pos;
		assert(t->z_pos == (*unit_positions)[0].z_pos);
		assert(t->direction == (*unit_positions)[0].direction);
	}

	SetPlatformReservation(train->tile, DirToDiagDir(train->direction), false);
	SetPlatformReservation(train->tile, DirToDiagDir(ReverseDir(train->direction)), false);

	UpdateSignalsOnSegment(train->tile, INVALID_DIAGDIR, train->owner);
}

/**
 * Restore the positions and direction of a train.
 * @param train The train we want to restore the positions of.
 */
void TrainPosBackup::RestoreBackup(Train *train,  DoCommandFlag flags)
{
	if (train == NULL || !IsBigRailDepotTile(train->tile)) return;

	if (!this->placed || (flags & DC_AUTOREPLACE) != 0) {
		assert(IsValidTile(this->orig_tile));
		train->track = this->orig_track;
		train->tile = this->orig_tile;
		train->direction = this->orig_dir;
		return;
	}

	uint i = 0;
	for (Train *t = train; t != NULL; t = t->Next(), i++) {
		// Backup.
		t->tile = (*unit_positions)[i].tile;
		t->track = this->orig_track;
		t->x_pos = (*unit_positions)[i].x_pos;
		t->y_pos = (*unit_positions)[i].y_pos;
		t->z_pos = (*unit_positions)[i].z_pos;
		t->direction = (*unit_positions)[i].direction;
	}

	SetPlatformReservation(train->tile, DirToDiagDir(train->direction), true);
	SetPlatformReservation(train->tile, DirToDiagDir(ReverseDir(train->direction)), true);

	UpdateSignalsOnSegment(train->tile, INVALID_DIAGDIR, train->owner);

	assert(unit_positions->Length() == i);
}

/**
 * Check if a train can be placed in a given tile.
 * @param train The train.
 * @param check_tile The tile where we want to check whether it is possible to place the train.
 * @return if it is possible or not to do it (also, best_* tile, track and direction is updated).
 */
bool TrainPosBackup::CheckPlacement(const Train *train, TileIndex check_tile)
{
	assert(train != NULL);
	assert(IsBigRailDepotTile(check_tile));

	if (!train->IsFrontEngine()) return true;

	SigSegState seg_state = UpdateSignalsOnSegment(check_tile, INVALID_DIAGDIR, train->owner);

	if (train->force_proceed == TFP_NONE && seg_state == SIGSEG_FULL) return false;

	// revise what happens with pbs
	DiagDirection diag_dir = DirToDiagDir(this->orig_dir);
	if (diag_dir != GetRailDepotDirection(check_tile) && diag_dir != ReverseDiagDir(GetRailDepotDirection(check_tile))) {
		diag_dir = GetRailDepotDirection(check_tile);
	}

	/* A direction for the train must be choosen: the one that allows the longest train in platform. */
	if (GetPlatformLength(check_tile, diag_dir) > GetPlatformLength(check_tile, ReverseDiagDir(diag_dir))) {
		diag_dir = ReverseDiagDir(diag_dir);
	}

	this->best_tile = check_tile;
	this->best_dir = DiagDirToDir(diag_dir);
	this->best_track = TrackToTrackBits(GetRailDepotTrack(check_tile));

	return true;
}


/**
 * Before placing a train in the rails of a big depot, a valid platform must
 * be found. This function finds a tile for placing the train (and also gets the direction and track).
 * If there is no valid tile, INVALID_TILE is kept as the "best tile".
 * @param t The train we want to place in rails.
 * @param info The type of error that makes placement impossible now or the type of placement if possible.
 * @pre The train must be inside the big rail depot as if in a small depot.
 *      (i.e. the track is TRACK_BIT_DEPOT, vehicles are hidden...).
 */
void TrainPosBackup::LookForPlaceInBigDepot(const Train *train, PlacementInfo *info)
{
	assert(train != NULL);
	assert(IsBigRailDepotTile(train->tile));

	/* Initialitzation. */
	this->best_tile = INVALID_TILE;
	this->best_dir = INVALID_DIR;
	this->best_track = INVALID_TRACK_BIT;
	*info = PI_FAILED_PLATFORM_TYPE; // Right now, we don't know whether there is any compatible platform.

	// First candidate is the original position of the train.
	TileIndex check_tile = this->orig_tile;
	DiagDirection dir = DirToDiagDir(this->orig_dir);

	/* If it was already placed, try maintaining the same platform. */
	if (this->placed) {
		RailTypes rail_types = ~RAILTYPES_NONE;
		for (const Train *t = train; t != NULL; t = t->Next()) {
			rail_types &= (t->compatible_railtypes | (RailTypes)(1 << t->railtype));
		}

		if (HasBit(rail_types, GetRailType(check_tile))) {
			/* Length has to be checked. First of all, try the same tile and direction.
			 * If that is not possible, check if depot has been extended (in theory, impossible...)
			 * and select that tile if long enough. */
			if (train->gcache.cached_total_length <= GetPlatformLength(check_tile, ReverseDiagDir(dir)) * TILE_SIZE) {
				*info = PI_FAILED_SIGNALS;
				if (this->CheckPlacement(train, check_tile)) {
					*info = PI_SUCCESS_ORIG_PLAT;
					return;
				}
			}

			if (train->gcache.cached_total_length <= GetPlatformLength(check_tile) * TILE_SIZE) {
				/* the train wasn't stopped were it should be:
				 * Longer platform after train in depot. */
				check_tile = GetStartPlatformTile(check_tile);
				*info = PI_FAILED_SIGNALS;
				if (this->CheckPlacement(train, check_tile)) {
					*info = PI_SUCCESS_OTHR_PLAT;
					return;
				}
			}
		}
	}

	/* Look for all platforms. */
	assert(IsBigRailDepotTile(check_tile));
	Depot *depot = Depot::GetByTile(check_tile);

	for (uint i = depot->depot_tiles.Length(); i--;) {
		check_tile = depot->depot_tiles[i];

		RailTypes rail_types = ~RAILTYPES_NONE;
		for (const Train *t = train; t != NULL; t = t->Next()) {
			rail_types &= (t->compatible_railtypes | (RailTypes)(1 << t->railtype));
		}

		// First, compatibility.
		if (!HasBit(rail_types, GetRailType(check_tile))) continue;
		if (*info < PI_FAILED_LENGTH) *info = PI_FAILED_LENGTH;

		// Then, length.
		if (train->gcache.cached_total_length > GetPlatformLength(check_tile) * TILE_SIZE) continue;
		if (*info < PI_FAILED_RESERVED) *info = PI_FAILED_RESERVED;

		// Reserved platform?
		if (HasDepotReservation(check_tile)) continue;
		if (*info < PI_FAILED_SIGNALS) *info = PI_FAILED_SIGNALS;

		// Signals?
		if (this->CheckPlacement(train, check_tile)) {
			*info = PI_SUCCESS_OTHR_PLAT;
			return;
		}
	}
}

/**
 * Check if a train can leave now or when other trains
 * move away. It returns if it can find a platform long
 * enough and with the appropriate rail type.
 * @param train The train.
 * @return true if there is a compatible platform long enough.
 */
bool TrainPosBackup::CanFindAppropriatePlatform(const Train *train)
{
	PlacementInfo info;
	this->LookForPlaceInBigDepot(train, &info);
	return info >= PI_WONT_LEAVE;
}

/**
 * When a train is lifted inside a big depot, before starting its way again,
 * must be placed in rails; this function does all necessary things to do so.
 * In general, it's the opposite of #LiftTrainInBigDepot
 * @param train The train we want to place in rails.
 * @pre The train must be inside the big rail depot as if in a small depot.
 *      (i.e. the track is TRACK_BIT_DEPOT, vehicles are hidden...).
 * @return true if train has been placed.
 */
bool TrainPosBackup::TryPlaceTrainInBigDepot(Train *train, DoCommandFlag flags)
{
	if (train == NULL || !IsBigRailDepotTile(train->tile)) return false;
	if ((flags & DC_AUTOREPLACE) != 0 || (flags & DC_EXEC) == 0) { // If not executing, revert what was done.
		this->RestoreBackup(train, flags); // returning true: what if train was not placed?
		return true;
	}

	if (!train->IsFrontEngine()) {
		/* Free chain. */
		for (Train *t = train; t != NULL; t = t->Next()) {
			t->vehstatus |= VS_HIDDEN;
			t->track = TRACK_BIT_DEPOT;
			t->UpdatePosition();
			t->UpdateViewport(true, true);
		}
		return false;
	}

	/* We are executing the command. So we need to do lots of things.
	 * As a precaution, free reserved path ahead and the platform itself.
	 * If there was no reservation, do nothing. */
	if (this->placed) {
		// revise: we must have backedup the train
		//if (train != NULL) FreeTrainTrackReservation(train);
		SetPlatformReservation(this->orig_tile, DirToDiagDir(this->orig_dir), false); // probably not needed
		UpdateSignalsOnSegment(this->orig_tile, INVALID_DIAGDIR, GetTileOwner(this->orig_tile));
	}

	if (train == NULL) return true;

	/* Look for a platform. See PlacementInfo for the error or placement information. */
	PlacementInfo info;
	this->LookForPlaceInBigDepot(train, &info);

	/* In case of error, we can finish. */
	if (info < PI_FAILED_END) {
		assert(this->best_tile == INVALID_TILE);
		/* If train cannot be placed, hide inside the depot. */
		for (Train *t = train; t != NULL; t = t->Next()) {
			t->tile = this->orig_tile;
			t->vehstatus |= VS_HIDDEN;
			t->track = TRACK_BIT_DEPOT;
			t->UpdatePosition();
			t->UpdateViewport(true, true);
		}

		/* Train cannot leave until changing the depot. Stop the train and send a message. */
		if (info < PI_WONT_LEAVE) {
			train->vehstatus |= VS_STOPPED;
			/* If vehicle is not stopped and user is the local company, send a message if needed. */
			if ((train->vehstatus & VS_STOPPED) == 0 && train->owner == _local_company && train->IsFrontEngine()) {
				SetDParam(0, train->index);
				AddVehicleAdviceNewsItem(STR_ADVICE_PLATFORM_TYPE + info - PI_BEGIN, train->index);
			}
		}

		return false;
	}

	assert(this->best_tile != INVALID_TILE);
	assert(this->best_dir != INVALID_DIR);
	assert(this->best_track != INVALID_TRACK_BIT);
	assert(IsBigRailDepotTile(this->best_tile));

	DiagDirection diag_dir = ReverseDiagDir(DirToDiagDir(this->best_dir));

	static const byte _plat_initial_x_fract[4] = {15, 8, 0,  8};
	static const byte _plat_initial_y_fract[4] = { 8, 0, 8, 15};

	/** 'Lookup table' for tile offsets given a DiagDirection */
	static const TileIndexDiffC _tileoffs_by_diagdir[] = { // revise this or the inverse
		{-1,  0}, ///< DIAGDIR_NE
		{ 0,  1}, ///< DIAGDIR_SE
		{ 1,  0}, ///< DIAGDIR_SW
		{ 0, -1}  ///< DIAGDIR_NW
	};

	int x = TileX(this->best_tile) * TILE_SIZE | _plat_initial_x_fract[diag_dir];
	int y = TileY(this->best_tile) * TILE_SIZE | _plat_initial_y_fract[diag_dir];

	/* Add the offset for the first vehicle. */
	x += _tileoffs_by_diagdir[diag_dir].x * (train->gcache.cached_veh_length + 1) / 2;
	y += _tileoffs_by_diagdir[diag_dir].y * (train->gcache.cached_veh_length + 1) / 2;

	/* Proceed placing the train in the given tile.
	 * At this point, the first vehicle contains the direction, tile and track.
	 * We must update positions of all the chain. */
	for (Train *t = train; t != NULL; t = t->Next()) {
		t->vehstatus &= ~VS_HIDDEN;
		t->track = this->best_track | TRACK_BIT_DEPOT;
		t->direction = this->best_dir;
		t->x_pos = x;
		t->y_pos = y;
		int advance = t->CalcNextVehicleOffset();
		x += _tileoffs_by_diagdir[diag_dir].x * advance;
		y += _tileoffs_by_diagdir[diag_dir].y * advance;
		t->z_pos = GetSlopePixelZ(t->x_pos, t->y_pos);
		t->tile = TileVirtXY(t->x_pos,t->y_pos);

		assert(t->z_pos == train->z_pos);
		assert(IsBigRailDepotTile(t->tile));
		t->UpdatePosition();
		t->UpdateViewport(true, true);
	}

	SetPlatformReservation(train->tile, DirToDiagDir(train->direction), true);
	SetPlatformReservation(train->tile, DirToDiagDir(ReverseDir(train->direction)), true);

	UpdateSignalsOnSegment(train->tile, INVALID_DIAGDIR, train->owner);

	return true;
}
