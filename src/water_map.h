/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file water_map.h Map accessors for water tiles. */

#ifndef WATER_MAP_H
#define WATER_MAP_H

#include "depot_type.h"
#include "tile_map.h"
#include "track_type.h"

/**
 * Bit field layout of m5 for water tiles.
 */
enum WaterTileTypeBitLayout {
	WBL_TYPE_BEGIN        = 6,   ///< Start of the 'type' bitfield.
	WBL_TYPE_COUNT        = 2,   ///< Length of the 'type' bitfield.

	WBL_TYPE_NORMAL       = 0x0, ///< Clear water or coast ('type' bitfield).
	WBL_TYPE_LOCK         = 0x1, ///< Lock ('type' bitfield).
	WBL_TYPE_DEPOT        = 0x2, ///< Depot ('type' bitfield).
	WBL_TYPE_BIG_DEPOT    = 0x3, ///< Big depot ('type' bitfield).

	WBL_COAST_FLAG        = 0,   ///< Flag for coast.

	WBL_LOCK_ORIENT_BEGIN = 0,   ///< Start of lock orientiation bitfield.
	WBL_LOCK_ORIENT_COUNT = 2,   ///< Length of lock orientiation bitfield.
	WBL_LOCK_PART_BEGIN   = 2,   ///< Start of lock part bitfield.
	WBL_LOCK_PART_COUNT   = 2,   ///< Length of lock part bitfield.

	WBL_DEPOT_PART        = 0,   ///< Depot part flag.
	WBL_DEPOT_AXIS        = 1,   ///< Depot axis flag.
};

/** Available water tile types. */
enum WaterTileType {
	WATER_TILE_CLEAR, ///< Plain water.
	WATER_TILE_COAST, ///< Coast.
	WATER_TILE_LOCK,  ///< Water lock.
	WATER_TILE_DEPOT, ///< Water Depot.
};

/** classes of water (for #WATER_TILE_CLEAR water tile type). */
enum WaterClass {
	WATER_CLASS_SEA,     ///< Sea.
	WATER_CLASS_CANAL,   ///< Canal.
	WATER_CLASS_RIVER,   ///< River.
	WATER_CLASS_INVALID, ///< Used for industry tiles on land (also for oilrig if newgrf says so).
};
/** Helper information for extract tool. */
template <> struct EnumPropsT<WaterClass> : MakeEnumPropsT<WaterClass, byte, WATER_CLASS_SEA, WATER_CLASS_INVALID, WATER_CLASS_INVALID, 2> {};

/** Sections of the water depot. */
enum DepotPart {
	DEPOT_PART_NORTH = 0, ///< Northern part of a depot.
	DEPOT_PART_SOUTH = 1, ///< Southern part of a depot.
	DEPOT_PART_END
};

/** Sections of the water lock. */
enum LockPart {
	LOCK_PART_MIDDLE = 0, ///< Middle part of a lock.
	LOCK_PART_LOWER  = 1, ///< Lower part of a lock.
	LOCK_PART_UPPER  = 2, ///< Upper part of a lock.
};

static const TileIndexDiffC _lock_tomiddle_offs[][DIAGDIR_END] = {
	/*   NE       SE        SW      NW       */
	{ { 0,  0}, {0,  0}, { 0, 0}, {0,  0} }, // LOCK_PART_MIDDLE
	{ {-1,  0}, {0,  1}, { 1, 0}, {0, -1} }, // LOCK_PART_LOWER
	{ { 1,  0}, {0, -1}, {-1, 0}, {0,  1} }, // LOCK_PART_UPPER
};

/**
 * Get the water tile type at a tile.
 * @param t Water tile to query.
 * @return Water tile type at the tile.
 */
static inline WaterTileType GetWaterTileType(TileIndex t)
{
	assert(IsTileType(t, MP_WATER));

	switch (GB(_m[t].m5, WBL_TYPE_BEGIN, WBL_TYPE_COUNT)) {
		case WBL_TYPE_NORMAL: return HasBit(_m[t].m5, WBL_COAST_FLAG) ? WATER_TILE_COAST : WATER_TILE_CLEAR;
		case WBL_TYPE_LOCK:   return WATER_TILE_LOCK;
		case WBL_TYPE_DEPOT:
		case WBL_TYPE_BIG_DEPOT:
			return WATER_TILE_DEPOT;
		default: NOT_REACHED();
	}
}

/**
 * Checks whether the tile has an waterclass associated.
 * You can then subsequently call GetWaterClass().
 * @param t Tile to query.
 * @return True if the tiletype has a waterclass.
 */
static inline bool HasTileWaterClass(TileIndex t)
{
	return IsTileType(t, MP_WATER) || IsTileType(t, MP_STATION) || IsTileType(t, MP_INDUSTRY) || IsTileType(t, MP_OBJECT);
}

/**
 * Get the water class at a tile.
 * @param t Water tile to query.
 * @pre IsTileType(t, MP_WATER) || IsTileType(t, MP_STATION) || IsTileType(t, MP_INDUSTRY) || IsTileType(t, MP_OBJECT)
 * @return Water class at the tile.
 */
static inline WaterClass GetWaterClass(TileIndex t)
{
	assert(HasTileWaterClass(t));
	return (WaterClass)GB(_m[t].m1, 5, 2);
}

/**
 * Set the water class at a tile.
 * @param t  Water tile to change.
 * @param wc New water class.
 * @pre IsTileType(t, MP_WATER) || IsTileType(t, MP_STATION) || IsTileType(t, MP_INDUSTRY) || IsTileType(t, MP_OBJECT)
 */
static inline void SetWaterClass(TileIndex t, WaterClass wc)
{
	assert(HasTileWaterClass(t));
	SB(_m[t].m1, 5, 2, wc);
}

/**
 * Tests if the tile was built on water.
 * @param t the tile to check
 * @pre IsTileType(t, MP_WATER) || IsTileType(t, MP_STATION) || IsTileType(t, MP_INDUSTRY) || IsTileType(t, MP_OBJECT)
 * @return true iff on water
 */
static inline bool IsTileOnWater(TileIndex t)
{
	return (GetWaterClass(t) != WATER_CLASS_INVALID);
}

/**
 * Is it a plain water tile?
 * @param t Water tile to query.
 * @return \c true if any type of clear water like ocean, river, or canal.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsWater(TileIndex t)
{
	return GetWaterTileType(t) == WATER_TILE_CLEAR;
}

/**
 * Is it a sea water tile?
 * @param t Water tile to query.
 * @return \c true if it is a sea water tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsSea(TileIndex t)
{
	return IsWater(t) && GetWaterClass(t) == WATER_CLASS_SEA;
}

/**
 * Is it a canal tile?
 * @param t Water tile to query.
 * @return \c true if it is a canal tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsCanal(TileIndex t)
{
	return IsWater(t) && GetWaterClass(t) == WATER_CLASS_CANAL;
}

/**
 * Is it a river water tile?
 * @param t Water tile to query.
 * @return \c true if it is a river water tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsRiver(TileIndex t)
{
	return IsWater(t) && GetWaterClass(t) == WATER_CLASS_RIVER;
}

/**
 * Is it a water tile with plain water?
 * @param t Tile to query.
 * @return \c true if it is a plain water tile.
 */
static inline bool IsWaterTile(TileIndex t)
{
	return IsTileType(t, MP_WATER) && IsWater(t);
}

/**
 * Is it a coast tile?
 * @param t Water tile to query.
 * @return \c true if it is a sea water tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsCoast(TileIndex t)
{
	return GetWaterTileType(t) == WATER_TILE_COAST;
}

/**
 * Is it a coast tile
 * @param t Tile to query.
 * @return \c true if it is a coast.
 */
static inline bool IsCoastTile(TileIndex t)
{
	return IsTileType(t, MP_WATER) && IsCoast(t);
}

/**
 * Is it a water tile with a ship depot on it?
 * @param t Water tile to query.
 * @return \c true if it is a ship depot tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsShipDepot(TileIndex t)
{
	return GetWaterTileType(t) == WATER_TILE_DEPOT;
}

/**
 * Is it a ship depot tile?
 * @param t Tile to query.
 * @return \c true if it is a ship depot tile.
 */
static inline bool IsShipDepotTile(TileIndex t)
{
	return IsTileType(t, MP_WATER) && IsShipDepot(t);
}

/**
 * Get the axis of the ship depot.
 * @param t Water tile to query.
 * @return Axis of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline Axis GetShipDepotAxis(TileIndex t)
{
	assert(IsShipDepotTile(t));
	return (Axis)GB(_m[t].m5, WBL_DEPOT_AXIS, 1);
}

/**
 * Get the part of a ship depot.
 * @param t Water tile to query.
 * @return Part of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline DepotPart GetShipDepotPart(TileIndex t)
{
	assert(IsShipDepotTile(t));
	return (DepotPart)GB(_m[t].m5, WBL_DEPOT_PART, 1);
}

/**
 * Get the direction of the ship depot.
 * @param t Water tile to query.
 * @return Direction of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline DiagDirection GetShipDepotDirection(TileIndex t)
{
	return XYNSToDiagDir(GetShipDepotAxis(t), GetShipDepotPart(t));
}

/**
 * Get the other tile of the ship depot.
 * @param t Tile to query, containing one section of a ship depot.
 * @return Tile containing the other section of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline TileIndex GetOtherShipDepotTile(TileIndex t)
{
	return t + (GetShipDepotPart(t) != DEPOT_PART_NORTH ? -1 : 1) * (GetShipDepotAxis(t) != AXIS_X ? TileDiffXY(0, 1) : TileDiffXY(1, 0));
}

/**
 * Get the most northern tile of a ship depot.
 * @param t One of the tiles of the ship depot.
 * @return The northern tile of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline TileIndex GetShipDepotNorthTile(TileIndex t)
{
	assert(IsShipDepot(t));
	TileIndex tile2 = GetOtherShipDepotTile(t);

	return t < tile2 ? t : tile2;
}

/**
 * Is there a lock on a given water tile?
 * @param t Water tile to query.
 * @return \c true if it is a water lock tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline bool IsLock(TileIndex t)
{
	return GetWaterTileType(t) == WATER_TILE_LOCK;
}

/**
 * Is there a lock on a given tile?
 * @param t Tile to query.
 * @return \c true if it is a water lock tile.
 */
static inline bool IsLockTile(TileIndex t)
{
	return IsTileType(t, MP_WATER) && GetWaterTileType(t) == WATER_TILE_LOCK;
}

/**
 * Get the direction of the water lock.
 * @param t Water tile to query.
 * @return Direction of the lock.
 * @pre IsTileType(t, MP_WATER) && IsLock(t)
 */
static inline DiagDirection GetLockDirection(TileIndex t)
{
	assert(IsLock(t));
	return (DiagDirection)GB(_m[t].m5, WBL_LOCK_ORIENT_BEGIN, WBL_LOCK_ORIENT_COUNT);
}

/**
 * Get the part of a lock.
 * @param t Water tile to query.
 * @return The part.
 * @pre IsTileType(t, MP_WATER) && IsLock(t)
 */
static inline byte GetLockPart(TileIndex t)
{
	assert(IsLock(t));
	return GB(_m[t].m5, WBL_LOCK_PART_BEGIN, WBL_LOCK_PART_COUNT);
}

/**
 * Get middle tile of a lock.
 * @param t Water tile to query.
 * @return The middle part tile.
 * @pre IsTileType(t, MP_WATER) && IsLock(t)
 */
static inline TileIndex GetLockMiddleTile(TileIndex t)
{
	assert(IsLock(t));
	return t + ToTileIndexDiff(_lock_tomiddle_offs[GetLockPart(t)][GetLockDirection(t)]);
}

/**
 * Check if two tiles belong to the same lock.
 * @param t1 First tile.
 * @param t2 Second tile.
 * @return true if the two tiles belong to the same dock.
 */
static inline bool CheckSameLock(TileIndex t1, TileIndex t2)
{
	if (!IsLockTile(t1) || !IsLockTile(t2)) return false;
	return GetLockMiddleTile(t1) == GetLockMiddleTile(t2);
}

/**
 * Get water level of a lock tile.
 * @param t Water tile to query.
 * @return The middle part tile.
 * @pre IsTileType(t, MP_WATER) && IsLock(t)
 */
static inline uint8 GetLockWaterLevel(TileIndex t)
{
	assert(IsLock(t));
	return GB(_m[t].m4, 0, 4);
}

/**
 * Set water level of a lock tile.
 * @param t Water tile to query.
 * @pre IsTileType(t, MP_WATER) && IsLock(t)
 */
static void SetLockWaterLevel(TileIndex t, uint8 level)
{
	assert(IsLock(t));
	SB(_m[t].m4, 0, 4, level);
}

/**
 * Get the random bits of the water tile.
 * @param t Water tile to query.
 * @return Random bits of the tile.
 * @pre IsTileType(t, MP_WATER)
 */
static inline byte GetWaterTileRandomBits(TileIndex t)
{
	assert(IsTileType(t, MP_WATER));
	return GB(_m[t].m4, 0, 4);
}

/**
 * Checks whether the tile has water at the ground.
 * That is, it is either some plain water tile, or a object/industry/station/... with water under it.
 * @return true iff the tile has water at the ground.
 * @note Coast tiles are not considered waterish, even if there is water on a halftile.
 */
static inline bool HasTileWaterGround(TileIndex t)
{
	return HasTileWaterClass(t) && IsTileOnWater(t) && !IsCoastTile(t);
}

/* Routines related to available tracks for water transport. */
TrackBits GetWaterTracks(TileIndex t);
TrackBits GetWaterTracksForBuoy(TileIndex t);

void UpdateWaterTiles();
void UpdateWaterTiles(TileIndex tile, uint rad);

/**
 * Helper function to make a coast tile.
 * @param t The tile to change into water
 */
static inline void MakeShore(TileIndex t)
{
	SetTileType(t, MP_WATER);
	SetTileOwner(t, OWNER_WATER);
	SetWaterClass(t, WATER_CLASS_SEA);
	_m[t].m2 = 0;
	_m[t].m3 = 0;
	_m[t].m4 = 0;
	_m[t].m5 = WBL_TYPE_NORMAL << WBL_TYPE_BEGIN | 1 << WBL_COAST_FLAG;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = 0;
}

/**
 * Helper function for making a watery tile.
 * @param t The tile to change into water
 * @param o The owner of the water
 * @param wc The class of water the tile has to be
 * @param random_bits Eventual random bits to be set for this tile (only 4 bits will be used)
 */
static inline void MakeWater(TileIndex t, Owner o, WaterClass wc, uint8 random_bits)
{
	SetTileType(t, MP_WATER);
	SetTileOwner(t, o);
	SetWaterClass(t, wc);
	_m[t].m2 = 0;
	_m[t].m3 = 0;
	SB(_m[t].m4, 0, 4, random_bits);
	_m[t].m5 = WBL_TYPE_NORMAL << WBL_TYPE_BEGIN;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = 0;
}

/**
 * Make a sea tile.
 * @param t The tile to change into sea
 */
static inline void MakeSea(TileIndex t)
{
	MakeWater(t, OWNER_WATER, WATER_CLASS_SEA, 0);
}

/**
 * Make a river tile
 * @param t The tile to change into river
 * @param random_bits Random bits to be set for this tile
 */
static inline void MakeRiver(TileIndex t, uint8 random_bits)
{
	MakeWater(t, OWNER_WATER, WATER_CLASS_RIVER, random_bits);
}

/**
 * Make a canal tile
 * @param t The tile to change into canal
 * @param o The owner of the canal
 * @param random_bits Random bits to be set for this tile
 */
static inline void MakeCanal(TileIndex t, Owner o, uint8 random_bits)
{
	assert(o != OWNER_WATER);
	MakeWater(t, o, WATER_CLASS_CANAL, random_bits);
}

/**
 * Make a ship depot section.
 * @param t    Tile to place the ship depot section.
 * @param o    Owner of the depot.
 * @param did  Depot ID.
 * @param part Depot part (either #DEPOT_PART_NORTH or #DEPOT_PART_SOUTH).
 * @param a    Axis of the depot.
 * @param original_water_class Original water class.
 */
static inline void MakeShipDepot(TileIndex t, Owner o, DepotID did, WaterTileTypeBitLayout depot_type, DepotPart part, Axis a, WaterClass original_water_class)
{
	SetTileType(t, MP_WATER);
	SetTileOwner(t, o);
	SetWaterClass(t, original_water_class);
	_m[t].m2 = did;
	_m[t].m3 = 0;
	_m[t].m4 = 0;
	_m[t].m5 = depot_type << WBL_TYPE_BEGIN | part << WBL_DEPOT_PART | a << WBL_DEPOT_AXIS;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = 0;
}

/**
 * Does this depot have a ship strictly inside?
 * @param tile tile of the depot.
 * @return true iff a ship is inside the depot.
 * @pre is a big ship depot tile
 */
static inline bool GetReservationAsDepot(TileIndex t)
{
	assert(IsShipDepotTile(t));
	assert(GB(_m[t].m5, WBL_TYPE_BEGIN, WBL_TYPE_COUNT) == WBL_TYPE_BIG_DEPOT);
	return HasBit(_m[t].m5, 2);
}

/**
 * Mark whether this depot has a ship inside.
 * @param tile of the depot.
 * @param value true if it has a ship inside, false if the ship has left.
 * @pre is a big ship depot tile.
 */
static inline void SetReservationAsDepot(TileIndex t, bool value)
{
	assert(IsShipDepotTile(t));
	assert(GB(_m[t].m5, WBL_TYPE_BEGIN, WBL_TYPE_COUNT) == WBL_TYPE_BIG_DEPOT);
	assert(GetReservationAsDepot(t) == GetReservationAsDepot(GetOtherShipDepotTile(t)));
	SB(_m[t].m5, 2, 1, value);
	SB(_m[GetOtherShipDepotTile(t)].m5, 2, 1, value);
}

/**
 * Make a lock section.
 * @param t Tile to place the water lock section.
 * @param o Owner of the lock.
 * @param part Part to place.
 * @param dir Lock orientation
 * @param original_water_class Original water class.
 * @see MakeLock
 */
static inline void MakeLockTile(TileIndex t, Owner o, LockPart part, DiagDirection dir, WaterClass original_water_class)
{
	SetTileType(t, MP_WATER);
	SetTileOwner(t, o);
	SetWaterClass(t, original_water_class);
	_m[t].m2 = 0;
	_m[t].m3 = 0;
	_m[t].m4 = 0;
	_m[t].m5 = WBL_TYPE_LOCK << WBL_TYPE_BEGIN | part << WBL_LOCK_PART_BEGIN | dir << WBL_LOCK_ORIENT_BEGIN;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = 0;
	if (part == LOCK_PART_UPPER) SetLockWaterLevel(t, TILE_HEIGHT);
}

/**
 * Make a water lock.
 * @param t Tile to place the water lock section.
 * @param o Owner of the lock.
 * @param d Direction of the water lock.
 * @param wc_lower Original water class of the lower part.
 * @param wc_upper Original water class of the upper part.
 * @param wc_middle Original water class of the middle part.
 */
static inline void MakeLock(TileIndex t, Owner o, DiagDirection d, WaterClass wc_lower, WaterClass wc_upper, WaterClass wc_middle)
{
	TileIndexDiff delta = TileOffsByDiagDir(d);

	/* Keep the current waterclass and owner for the tiles.
	 * It allows to restore them after the lock is deleted */
	MakeLockTile(t, o, LOCK_PART_MIDDLE, d, wc_middle);
	MakeLockTile(t - delta, IsWaterTile(t - delta) ? GetTileOwner(t - delta) : o, LOCK_PART_LOWER, d, wc_lower);
	MakeLockTile(t + delta, IsWaterTile(t + delta) ? GetTileOwner(t + delta) : o, LOCK_PART_UPPER, d, wc_upper);
}

#endif /* WATER_MAP_H */
