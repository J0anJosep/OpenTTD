/* $Id$ */

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
	TileIndex tile;      ///< Tracktile. For track on a bridge this is the southern bridgehead.
	TileContext context; ///< Are we resolving sprites for the upper halftile, or on a bridge?

	AirTypeScopeResolver(ResolverObject &ro, TileIndex tile, TileContext context);

	/* virtual */ uint32 GetRandomBits() const;
	/* virtual */ uint32 GetVariable(byte variable, uint32 parameter, bool *available) const;
};

/** Resolver object for air types. */
struct AirTypeResolverObject : public ResolverObject {
	AirTypeScopeResolver airtype_scope; ///< Resolver for the airtype scope.

	AirTypeResolverObject(TileIndex tile, TileContext context, const GRFFile *grffile, uint32 param1 = 0, uint32 param2 = 0);

	/* virtual */ ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, byte relative = 0)
	{
		switch (scope) {
			case VSG_SCOPE_SELF: return &this->airtype_scope;
			default:             return ResolverObject::GetScope(scope, relative);
		}
	}

	/* virtual */ const SpriteGroup *ResolveReal(const RealSpriteGroup *group) const;
};

SpriteID GetCustomAirSprite(const AirTypeInfo *ati, TileIndex tile, AirTypeSpriteGroup atsg, TileContext context = TCX_NORMAL);

uint8 GetReverseAirTypeTranslation(AirType railtype, const GRFFile *grffile);

#endif /* NEWGRF_AIRTYPE_H */
