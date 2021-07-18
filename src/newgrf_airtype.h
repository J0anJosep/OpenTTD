/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_airtype.h NewGRF handling of air types. */

#ifndef NEWGRF_AIRTYPE_H
#define NEWGRF_AIRTYPE_H

#include "air.h"
#include "newgrf_commons.h"
#include "newgrf_spritegroup.h"

/** Resolver for the railtype scope. */
struct AirTypeScopeResolver : public ScopeResolver {
	TileIndex tile;         ///< Tracktile. For track on a bridge this is the southern bridgehead.
	TileContext context;    ///< Are we resolving sprites for the upper halftile, or on a bridge?
	const AirtypeInfo *ati; ///< The corresponding airtype info.

	/**
	* Constructor of the airtype scope resolvers.
	* @param ro Surrounding resolver.
	* @param ati AirtypeInfo
	* @param tile %Tile containing the track. For track on a bridge this is the southern bridgehead.
	* @param context Are we resolving sprites for the upper halftile, or on a bridge?
	*/
	AirTypeScopeResolver(ResolverObject &ro, const AirtypeInfo *ati, TileIndex tile, TileContext context) :
			ScopeResolver(ro), tile(tile), context(context), ati(ati)
	{
	}

	/* virtual */ uint32 GetRandomBits() const;
	/* virtual */ uint32 GetVariable(byte variable, uint32 parameter, bool *available) const;
};

/** Resolver object for air types. */
struct AirTypeResolverObject : public ResolverObject {
	AirTypeScopeResolver airtype_scope; ///< Resolver for the airtype scope.

	AirTypeResolverObject(const AirtypeInfo *ati, TileIndex tile, TileContext context, AirTypeSpriteGroup rtsg, uint32 param1 = 0, uint32 param2 = 0);

	/* virtual */ ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, byte relative = 0)
	{
		switch (scope) {
			case VSG_SCOPE_SELF: return &this->airtype_scope;
			default:             return ResolverObject::GetScope(scope, relative);
		}
	}

	GrfSpecFeature GetFeature() const;
	uint32 GetDebugID() const;
};

SpriteID GetCustomAirSprite(const AirtypeInfo *ati, TileIndex tile, AirTypeSpriteGroup atsg, TileContext context = TCX_NORMAL, uint *num_results = nullptr);

AirType GetAirTypeTranslation(uint8 airtype, const GRFFile *grffile);
uint8 GetReverseAirTypeTranslation(AirType railtype, const GRFFile *grffile);

#endif /* NEWGRF_AIRTYPE_H */
