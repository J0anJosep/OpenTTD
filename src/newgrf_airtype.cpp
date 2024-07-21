/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_airtype.cpp NewGRF handling of air types. */

#include "stdafx.h"
#include "core/container_func.hpp"
#include "debug.h"
#include "newgrf_airtype.h"
#include "station_base.h"
#include "depot_base.h"
#include "town.h"

#include "safeguards.h"

/* virtual */ uint32_t AirTypeScopeResolver::GetRandomBits() const
{
	uint tmp = CountBits(this->tile.base() + (TileX(this->tile) + TileY(this->tile)) * TILE_SIZE);
	return GB(tmp, 0, 2);
}

/* virtual */ uint32_t AirTypeScopeResolver::GetVariable(uint8_t variable, [[maybe_unused]] uint32_t parameter, bool &available) const
{
	if (this->tile == INVALID_TILE) {
		switch (variable) {
			case 0x40: return 0;
			case 0x41: return 0;
			case 0x42: return 0;
			case 0x43: return TimerGameCalendar::date.base();
			case 0x44: return HZB_TOWN_EDGE;
		}
	}

	switch (variable) {
		case 0x40: return GetTerrainType(this->tile, this->context);
		case 0x41: return 0;
		case 0x42: return 0;
		case 0x43:
			if (IsHangarTile(this->tile)) return Depot::GetByTile(this->tile)->build_date.base();
			return TimerGameCalendar::date.base();
		case 0x44: {
			const Town *t = nullptr;
			t = Station::GetByTile(this->tile)->town;
			return t != nullptr ? GetTownRadiusGroup(t, this->tile) : HZB_TOWN_EDGE;
		}
	}

	Debug(grf, 1, "Unhandled air type tile variable 0x{:X}", variable);

	available = false;
	return UINT_MAX;
}

GrfSpecFeature AirTypeResolverObject::GetFeature() const
{
	return GSF_AIRTYPES;
}

uint32_t AirTypeResolverObject::GetDebugID() const
{
	return this->airtype_scope.ati->label;
}

/**
 * Resolver object for air types.
 * @param ati AirType. nullptr in NewGRF Inspect window.
 * @param tile %Tile containing the track. For track on a bridge this is the southern bridgehead.
 * @param context Are we resolving sprites for the upper halftile, or on a bridge?
 * @param rtsg Airpart of interest
 * @param param1 Extra parameter (first parameter of the callback, except airtypes do not have callbacks).
 * @param param2 Extra parameter (second parameter of the callback, except airtypes do not have callbacks).
 */
AirTypeResolverObject::AirTypeResolverObject(const AirTypeInfo *ati, TileIndex tile, TileContext context, AirTypeSpriteGroup rtsg, uint32_t param1, uint32_t param2)
	: ResolverObject(ati != nullptr ? ati->grffile[rtsg] : nullptr, CBID_NO_CALLBACK, param1, param2), airtype_scope(*this, ati, tile, context)
{
	this->root_spritegroup = ati != nullptr ? ati->group[rtsg] : nullptr;
}

/**
 * Get the sprite to draw for the given tile.
 * @param ati The air type data (spec).
 * @param tile The tile to get the sprite for.
 * @param rtsg The type of sprite to draw.
 * @param content Where are we drawing the tile?
 * @param [out] num_results If not nullptr, return the number of sprites in the spriteset.
 * @return The sprite to draw.
 */
SpriteID GetCustomAirSprite(const AirTypeInfo *ati, TileIndex tile, AirTypeSpriteGroup atsg, TileContext context, uint *num_results)
{
	assert(atsg < ATSG_END);

	if (ati->group[atsg] == nullptr) return 0;

	AirTypeResolverObject object(ati, tile, context, atsg);
	const SpriteGroup *group = object.Resolve();
	if (group == nullptr || group->GetNumResults() == 0) return 0;

	if (num_results != nullptr) *num_results = group->GetNumResults();

	return group->GetResult();
}

/**
 * Translate an index to the GRF-local airtype-translation table into an AirType.
 * @param airtype  Index into GRF-local translation table.
 * @param grffile   Originating GRF file.
 * @return AirType or INVALID_AIRTYPE if the airtype is unknown.
 */
AirType GetAirTypeTranslation(uint8_t airtype, const GRFFile *grffile)
{
	if (grffile == nullptr || grffile->airtype_list.size() == 0) {
		/* No airtype table present. Return airtype as-is (if valid), so it works for original airtypes. */
		if (airtype >= AIRTYPE_END || GetAirTypeInfo(static_cast<AirType>(airtype))->label == 0) return INVALID_AIRTYPE;

		return static_cast<AirType>(airtype);
	} else {
		/* AirType table present, but invalid index, return invalid type. */
		if (airtype >= grffile->airtype_list.size()) return INVALID_AIRTYPE;

		/* Look up airtype including alternate labels. */
		return GetAirTypeByLabel(grffile->airtype_list[airtype]);
	}
}

/**
 * Perform a reverse airtype lookup to get the GRF internal ID.
 * @param airtype The global (OpenTTD) airtype.
 * @param grffile The GRF to do the lookup for.
 * @return the GRF internal ID.
 */
uint8_t GetReverseAirTypeTranslation(AirType airtype, const GRFFile *grffile)
{
	/* No air type table present, return air type as-is */
	if (grffile == nullptr || grffile->airtype_list.size() == 0) return airtype;

	/* Look for a matching air type label in the table */
	AirTypeLabel label = GetAirTypeInfo(airtype)->label;

	int idx = find_index(grffile->airtype_list, label);
	if (idx >= 0) return idx;

	/* If not found, return invalid. */
	return 0xFF;
}
