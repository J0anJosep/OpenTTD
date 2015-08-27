/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file train_pos_backup.h Structs and functions dealing with position backups of trains. */

#ifndef TRAINPB_H
#define TRAINPB_H

#include "core/enum_type.hpp"
#include "core/smallvec_type.hpp"
#include "train.h"


/* The type of problems that can arise when trying placing a train. */
enum PlacementInfo {
	PI_BEGIN = 0,
	PI_FAILED_PLATFORM_TYPE = PI_BEGIN,     // No compatible platforms with train type.
	PI_FAILED_LENGTH,                       // There are compatible platforms but not long enough.
	PI_WONT_LEAVE,
	PI_FAILED_RESERVED = PI_WONT_LEAVE,     // There are compatible platforms but reserved right now.
						  //(check if the train is reserving its platform and others ahead)
	PI_FAILED_SIGNALS,                      // There are compatible platforms not reserved, but signals don't allow placing it now.
	PI_FAILED_END,
	PI_SUCCESS_OTHR_PLAT = PI_FAILED_END,   // There is a good platform, but not the origignal one.
						  // (or the original one was extended and we can put it in the same platform
						  // but in another starting tile)
	PI_SUCCESS_ORIG_PLAT,                   // The original platform is ok for placing the train.
	PI_END,
};


/* The tiles, positions and directions of the units of a train in a depot. */
struct UnitPos {
	TileIndex tile;
	int32 x_pos;
	int32 y_pos;
	int32 z_pos;
	DirectionByte direction;
};

typedef SmallVector<UnitPos, 8> UnitPositions;

/* Store position of a train and lift it when necessary. */
struct TrainPosBackup {
	bool placed;                   // True if train is placed in rails.
	TileIndex orig_tile;           // Original tile of the train.
	Direction orig_dir;            // Original direction of the train.
	TrackBits orig_track;          // Original track of the train.
	TileIndex best_tile;           // Best tile for the train.
	Direction best_dir;            // Best direction for the train.
	TrackBits best_track;          // Best track for the train.
	UnitPositions *unit_positions; // Original positions of elements of the train (or the new positions).

	TrainPosBackup() : placed(false),
			orig_tile(INVALID_TILE),
			orig_dir(INVALID_DIR),
			orig_track(INVALID_TRACK_BIT),
			best_tile(INVALID_TILE),
			best_dir(INVALID_DIR),
			best_track(INVALID_TRACK_BIT),
			unit_positions(NULL) {}

	~TrainPosBackup() { free(unit_positions); }

	/* NOTE ABOUT THE SPECIAL USE OF DC_AUTOREPLACE.
	 * If the DC_AUTOREPLACE is set,
	 * the dual-methods Lift-Try or Lift-Restore
	 * won't change anything in the original vehicle.
	 * Also, if the DC_AUTOREPLACE is set, it must be assumed that
	 * the vehicle is already lifted in the depot. */
	void LiftTrainInBigDepot(Train *train, DoCommandFlag flags);
	void RestoreBackup(Train *train, DoCommandFlag flags);

	bool CheckPlacement(const Train *train, TileIndex tile);
	void LookForPlaceInBigDepot(const Train *train, PlacementInfo *info);
	bool CanFindAppropriatePlatform(const Train *train);
	bool TryPlaceTrainInBigDepot(Train *train, DoCommandFlag flags);
};

static inline bool CheckIfTrainNeedsPlacement(const Train *train)
{
	return IsBigRailDepot(train->tile) && (train->track & ~TRACK_BIT_DEPOT) == 0;
}

static inline bool IsTrainAlreadyPlaced(const Train *train)
{
	return IsBigRailDepot(train->tile) && (train->track & ~TRACK_BIT_DEPOT) != 0 &&
			(train->track & TRACK_BIT_DEPOT) != 0;
}

#endif /* TPB_H */
