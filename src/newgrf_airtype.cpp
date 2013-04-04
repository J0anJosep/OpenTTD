/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_airtype.cpp NewGRF handling of air types. */

#include "stdafx.h"
#include "debug.h"
#include "newgrf_airtype.h"
#include "date_func.h"
#include "station_base.h"
#include "town.h"

/* virtual */ uint32 AirTypeScopeResolver::GetRandomBits() const
{
	return GB(_m[this->tile].m4, 4, 4); //revise
}

/* virtual */ uint32 AirTypeScopeResolver::GetVariable(byte variable, uint32 parameter, bool *available) const
{
	if (this->tile == INVALID_TILE) {
		switch (variable) {
			case 0x40: return 0;
			case 0x41: return 0;
			case 0x42: return 0;
			case 0x43: return _date;
			case 0x44: return HZB_TOWN_EDGE;
		}
	}

	switch (variable) {
		case 0x40: return GetTerrainType(this->tile, this->context);
		case 0x41: return 0;
		case 0x42: return 0; // IsLevelCrossingTile(this->tile) && IsCrossingBarred(this->tile);
		case 0x43: return Station::GetByTile(this->tile)->build_date;
		case 0x44: {
			const Town *t = NULL;
			t = Station::GetByTile(this->tile)->town;
			return t != NULL ? GetTownRadiusGroup(t, this->tile) : HZB_TOWN_EDGE;
		}
	}

	DEBUG(grf, 1, "Unhandled air type tile variable 0x%X", variable);

	*available = false;
	return UINT_MAX;
}

/* virtual */ const SpriteGroup *AirTypeResolverObject::ResolveReal(const RealSpriteGroup *group) const
{
	if (group->num_loading > 0) return group->loading[0];
	if (group->num_loaded  > 0) return group->loaded[0];
	return NULL;
}

/**
 * Constructor of the airtype scope resolvers.
 * @param ro Surrounding resolver.
 * @param tile %Tile containing the track. For track on a bridge this is the southern bridgehead.
 * @param context Are we resolving sprites for the upper halftile, or on a bridge?
 */
AirTypeScopeResolver::AirTypeScopeResolver(ResolverObject &ro, TileIndex tile, TileContext context) : ScopeResolver(ro)
{
	this->tile = tile;
	this->context = context;
}

/**
 * Resolver object for air types.
 * @param tile %Tile containing the track. For track on a bridge this is the southern bridgehead.
 * @param context Are we resolving sprites for the upper halftile, or on a bridge?
 * @param grffile The GRF to do the lookup for.
 * @param param1 Extra parameter (first parameter of the callback, except airtypes do not have callbacks).
 * @param param2 Extra parameter (second parameter of the callback, except airtypes do not have callbacks).
 */
AirTypeResolverObject::AirTypeResolverObject(TileIndex tile, TileContext context, const GRFFile *grffile, uint32 param1, uint32 param2)
	: ResolverObject(grffile, CBID_NO_CALLBACK, param1, param2), airtype_scope(*this, tile, context)
{
}

/**
 * Get the sprite to draw for the given tile.
 * @param ati The air type data (spec).
 * @param tile The tile to get the sprite for.
 * @param rtsg The type of sprite to draw.
 * @param content Where are we drawing the tile?
 * @return The sprite to draw.
 */
SpriteID GetCustomAirSprite(const AirTypeInfo *ati, TileIndex tile, AirTypeSpriteGroup atsg, TileContext context)
{
	assert(atsg < ATSG_END);

	if (ati->group[atsg] == NULL) return 0;

	AirTypeResolverObject object(tile, context, ati->grffile[atsg]);
	const SpriteGroup *group = SpriteGroup::Resolve(ati->group[atsg], object);
	if (group == NULL || group->GetNumResults() == 0) return 0;

	return group->GetResult();
}

/**
 * Perform a reverse airtype lookup to get the GRF internal ID.
 * @param airtype The global (OpenTTD) airtype.
 * @param grffile The GRF to do the lookup for.
 * @return the GRF internal ID.
 */
uint8 GetReverseAirTypeTranslation(AirType airtype, const GRFFile *grffile)
{
	/* No air type table present, return air type as-is */
	if (grffile == NULL || grffile->airtype_list.Length() == 0) return airtype;

	/* Look for a matching air type label in the table */
	AirTypeLabel label = GetAirTypeInfo(airtype)->label;
	int index = grffile->airtype_list.FindIndex(label);
	if (index >= 0) return index;

	/* If not found, return as invalid */
	return 0xFF;
}
