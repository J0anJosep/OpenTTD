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
	const AirTypeInfo *ati; ///< The corresponding airtype info.

	/**
	* Constructor of the airtype scope resolvers.
	* @param ro Surrounding resolver.
	* @param ati AirTypeInfo
	* @param tile %Tile containing the track. For track on a bridge this is the southern bridgehead.
	* @param context Are we resolving sprites for the upper halftile, or on a bridge?
	*/
	AirTypeScopeResolver(ResolverObject &ro, const AirTypeInfo *ati, TileIndex tile, TileContext context) :
			ScopeResolver(ro), tile(tile), context(context), ati(ati)
	{
	}

	uint32_t GetRandomBits() const override;
	uint32_t GetVariable(uint8_t variable, uint32_t parameter, bool &available) const override;
};

/** Resolver object for air types. */
struct AirTypeResolverObject : public ResolverObject {
	AirTypeScopeResolver airtype_scope; ///< Resolver for the airtype scope.

	AirTypeResolverObject(const AirTypeInfo *ati, TileIndex tile, TileContext context, AirTypeSpriteGroup rtsg, uint32_t param1 = 0, uint32_t param2 = 0);

	ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, uint8_t relative = 0) override
	{
		if (scope == VSG_SCOPE_SELF) return &this->airtype_scope;

		return ResolverObject::GetScope(scope, relative);
	}

	GrfSpecFeature GetFeature() const override;
	uint32_t GetDebugID() const override;
};

SpriteID GetCustomAirSprite(const AirTypeInfo *ati, TileIndex tile, AirTypeSpriteGroup atsg, TileContext context = TCX_NORMAL, uint *num_results = nullptr);

AirType GetAirTypeTranslation(uint8_t airtype, const GRFFile *grffile);
uint8_t GetReverseAirTypeTranslation(AirType railtype, const GRFFile *grffile);

#endif /* NEWGRF_AIRTYPE_H */
