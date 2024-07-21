/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_airport.h NewGRF handling of airports. */

#ifndef NEWGRF_AIRPORT_H
#define NEWGRF_AIRPORT_H

#include "airport.h"
#include "timer/timer_game_calendar.h"
#include "newgrf_class.h"
#include "newgrf_commons.h"
#include "newgrf_spritegroup.h"
#include "newgrf_town.h"
#include "tilearea_type.h"

#include "table/airporttile_ids.h"

/** Copy from station_map.h */
typedef uint8_t StationGfx;

/** List of default airport classes. */
enum AirportClassID {
	APC_BEGIN     = 0,  ///< Lowest valid airport class id
	APC_SMALL     = 0,  ///< id for small airports class
	APC_LARGE,          ///< id for large airports class
	APC_HUB,            ///< id for hub airports class
	APC_HELIPORT,       ///< id for heliports
	APC_CUSTOM,         ///< customized airport class
	APC_MAX       = 16, ///< maximum number of airport classes
};

/** Allow incrementing of AirportClassID variables */
DECLARE_POSTFIX_INCREMENT(AirportClassID)

/** TTDP airport types. Used to map our types to TTDPatch's */
enum TTDPAirportType {
	ATP_TTDP_SMALL,    ///< Same as AT_SMALL
	ATP_TTDP_LARGE,    ///< Same as AT_LARGE
	ATP_TTDP_HELIPORT, ///< Same as AT_HELIPORT
	ATP_TTDP_OILRIG,   ///< Same as AT_OILRIG
};

struct AirportTileTable {
	AirportTileType type;                       // Use this tile will have (apron, tracks,...).
	ApronType apron_type{APRON_INVALID};        // Subtype of apron.
	DiagDirection dir{INVALID_DIAGDIR};         // Direction of runway or exit direction of hangars.
	TrackBits trackbits{TRACK_BIT_NONE};        // Tracks for this tile.
	Direction runway_directions{INVALID_DIR};   // Directions of the runways present on this tile.
												// Maps a direction into the diagonal directions of the runways.
	AirportTiles at_gfx{ATTG_DEFAULT_GFX};      // Sprite for this tile as provided by an airtype.
	AirportTiles gfx[DIAGDIR_END] {             // Sprites for this tile.
		INVALID_AIRPORTTILE,
		INVALID_AIRPORTTILE,
		INVALID_AIRPORTTILE,
		INVALID_AIRPORTTILE
	};

	void SetGfx(AirportTiles gfx) {
		this->gfx[DIAGDIR_BEGIN] = gfx;
	}

	/* Description for simple track tiles. */
	AirportTileTable(AirportTileType att, TrackBits trackbits, AirportTiles gfx = INVALID_AIRPORTTILE) :
		type(att),
		trackbits(trackbits)
	{
		assert(att == ATT_SIMPLE_TRACK);
		SetGfx(gfx);
	}

	/* Description for aprons, helipads and heliports. */
	AirportTileTable(AirportTileType att, TrackBits trackbits, ApronType type,
			AirportTiles gfx = INVALID_AIRPORTTILE) :
		type(att),
		apron_type(type),
		trackbits(trackbits)
	{
		assert(att >= ATT_APRON_NORMAL && att <= ATT_APRON_BUILTIN_HELIPORT);
		SetGfx(gfx);
	}

	/* Description for hangars and runway end and start. */
	AirportTileTable(AirportTileType att, TrackBits trackbits, DiagDirection dir, AirportTiles gfx = INVALID_AIRPORTTILE) :
		type(att),
		dir(dir),
		trackbits(trackbits)
	{
		assert(att == ATT_HANGAR_STANDARD ||
				att == ATT_HANGAR_EXTENDED ||
				att == ATT_RUNWAY_END ||
				att == ATT_RUNWAY_START_ALLOW_LANDING ||
				att == ATT_RUNWAY_START_NO_LANDING);
		SetGfx(gfx);
	}

	/* Description for middle parts of runways. */
	AirportTileTable(AirportTileType att, TrackBits trackbits, Direction runway_directions, AirportTiles gfx = INVALID_AIRPORTTILE) :
		type(att),
		trackbits(trackbits),
		runway_directions(runway_directions)
	{
		assert(att == ATT_RUNWAY_MIDDLE);
		assert(IsValidDirection(runway_directions));
		SetGfx(gfx);
	}

	/* Description for infrastructure. */
	AirportTileTable(AirportTileType att, AirportTiles at_gfx, DiagDirection rotation = DIAGDIR_NE,
			AirportTiles gfx = INVALID_AIRPORTTILE) :
		type(att),
		dir(rotation),
		at_gfx(at_gfx)
	{
		assert(att == ATT_INFRASTRUCTURE_WITH_CATCH || att == ATT_INFRASTRUCTURE_NO_CATCH);
		SetGfx(gfx);
	}

	/* Description for a non-airport tile, for non-rectangular airports. */
	AirportTileTable() : type(ATT_INVALID) {}
};

struct AirportTileLayout {
	std::vector<AirportTileTable> tiles; ///< List of all tiles in this layout.
	uint8_t size_x;                        ///< size of airport in x direction
	uint8_t size_y;                        ///< size of airport in y direction
};

/**
 * Defines the data structure for an airport.
 */
struct AirportSpec : NewGRFSpecBase<AirportClassID> {
	std::vector<AirportTileLayout> layouts;///< list of the different layouts
	AirType airtype;                       ///< the airtype for this set of layouts
	uint8_t num_runways;                   ///< number of runways
	uint8_t num_aprons;                    ///< number of aprons
	uint8_t num_helipads;                  ///< number of helipads
	uint8_t num_heliports;                 ///< number of heliports
	uint8_t min_runway_length;             ///< length of the shortest runway
	TimerGameCalendar::Year min_year;      ///< first year the airport is available
	TimerGameCalendar::Year max_year;      ///< last year the airport is available
	StringID name;                         ///< name of this airport
	TTDPAirportType ttd_airport_type;      ///< ttdpatch airport type (Small/Large/Helipad/Oilrig)
	SpriteID preview_sprite;               ///< preview sprite for this airport
	bool enabled;                          ///< Entity still available (by default true). Newgrf can disable it, though.
	bool has_hangar;
	bool has_heliport;
	struct GRFFileProps grf_prop;          ///< Properties related to the grf file.

	static const AirportSpec *Get(uint8_t type);
	static AirportSpec *GetWithoutOverride(uint8_t type);

	bool IsAvailable(AirType air_type = INVALID_AIRTYPE) const;
	bool IsWithinMapBounds(uint8_t table, TileIndex index, uint8_t layout) const;

	static void ResetAirports();

	/** Get the index of this spec. */
	uint8_t GetIndex() const
	{
		assert(this >= std::begin(specs) && this < std::end(specs));
		return static_cast<uint8_t>(std::distance(std::cbegin(specs), this));
	}

	uint8_t GetAirportNoise(AirType airtype) const;

	static const AirportSpec custom; ///< The customized airports specs.
	static const AirportSpec dummy;  ///< The dummy airport.

private:
	static AirportSpec specs[NUM_AIRPORTS]; ///< Specs of the airports.
};

/** Information related to airport classes. */
using AirportClass = NewGRFClass<AirportSpec, AirportClassID, APC_MAX>;

void BindAirportSpecs();

/** Resolver for the airport scope. */
struct AirportScopeResolver : public ScopeResolver {
	struct Station *st; ///< Station of the airport for which the callback is run, or \c nullptr for build gui.
	const AirportSpec *spec; ///< AirportSpec for which the callback is run.
	uint8_t layout;        ///< Layout of the airport to build.
	TileIndex tile;     ///< Tile for the callback, only valid for airporttile callbacks.

	/**
	 * Constructor of the scope resolver for an airport.
	 * @param ro Surrounding resolver.
	 * @param tile %Tile for the callback, only valid for airporttile callbacks.
	 * @param st %Station of the airport for which the callback is run, or \c nullptr for build gui.
	 * @param spec AirportSpec for which the callback is run.
	 * @param layout Layout of the airport to build.
	 */
	AirportScopeResolver(ResolverObject &ro, TileIndex tile, Station *st, const AirportSpec *spec, uint8_t layout)
		: ScopeResolver(ro), st(st), spec(spec), layout(layout), tile(tile)
	{
	}

	uint32_t GetRandomBits() const override;
	uint32_t GetVariable(uint8_t variable, [[maybe_unused]] uint32_t parameter, bool &available) const override;
	void StorePSA(uint pos, int32_t value) override;
};


/** Resolver object for airports. */
struct AirportResolverObject : public ResolverObject {
	AirportScopeResolver airport_scope;
	std::optional<TownScopeResolver> town_scope = std::nullopt; ///< The town scope resolver (created on the first call).

	AirportResolverObject(TileIndex tile, Station *st, const AirportSpec *spec, uint8_t layout,
			CallbackID callback = CBID_NO_CALLBACK, uint32_t callback_param1 = 0, uint32_t callback_param2 = 0);

	TownScopeResolver *GetTown();

	ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, uint8_t relative = 0) override
	{
		switch (scope) {
			case VSG_SCOPE_SELF: return &this->airport_scope;
			case VSG_SCOPE_PARENT:
			{
				TownScopeResolver *tsr = this->GetTown();
				if (tsr != nullptr) return tsr;
				[[fallthrough]];
			}
			default: return ResolverObject::GetScope(scope, relative);
		}
	}

	GrfSpecFeature GetFeature() const override;
	uint32_t GetDebugID() const override;
};

StringID GetAirportTextCallback(const AirportSpec *as, uint8_t layout, uint16_t callback);

#endif /* NEWGRF_AIRPORT_H */
