/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_aircraft.cpp Implementation of YAPF for aircraft. */

#include "../../stdafx.h"
#include "../../aircraft.h"
#include "../../pbs_air.h"
#include "../../pbs_air.h"

#include "track_func.h"
#include "yapf.hpp"
#include "yapf_node_air.hpp"

#include "../../safeguards.h"

/**
 * When looking for aircraft paths, crossing a non-diagonal track
 * may put the aircraft too close to another aircraft crossing the
 * equivalent track on a neighbour tile (or getting too close to a hangar).
 * Check whether the associated tile is available
 * and its corresponding track is not reserved.
 * @param tile Tile to check.
 * @param track Involved track on tile.
 * @return Whether the associated tile can be crossed and it is of the same station and is not reserved.
 * @pre IsAirportTile
 */
bool IsAssociatedAirportTileFree(TileIndex tile, Track track)
{
	assert(IsAirportTile(tile));
	assert(IsValidTrack(track));

	if (IsDiagonalTrack(track)) return true;

	static const Direction track_dir_table[TRACK_END] = {
			INVALID_DIR,
			INVALID_DIR,
			DIR_N,
			DIR_S,
			DIR_W,
			DIR_E,
	};

	TileIndex neighbour =  TileAddByDir(tile, track_dir_table[track]);

	return IsValidTile(neighbour)
			&& IsAirportTileOfStation(neighbour, GetStationIndex(tile))
			&& MayHaveAirTracks(neighbour)
			&& !IsHangar(neighbour)
			&& !(IsRunway(neighbour) && GetReservationAsRunway(neighbour))
			&& !HasAirportTrackReserved(neighbour, TrackToOppositeTrack(track));
}

/**
 * Check if a tile can be reserved and does not collide with another reserved path.
 * @param tile The tile.
 * @param trackdir The trackdir to check.
 * @return True if reserving \a trackdir on tile \a tile doesn't collide with other paths.
 */
bool IsAirportTileFree(TileIndex tile, Trackdir trackdir)
{
	assert(IsAirportTile(tile));
	assert(MayHaveAirTracks(tile));
	assert(IsValidTrackdir(trackdir));

	/* Reserved tiles are not free. */
	if (HasAirportTileAnyReservation(tile)) return false;

	/* With non-diagonal tracks, if there is a close reserved non-diagonal track,
	 * it is not a free tile. */
	Track track = TrackdirToTrack(trackdir);
	return IsAssociatedAirportTileFree(tile, track);
}

/** Node Follower module of YAPF for aircraft. */
template <class Types>
class CYapfFollowAircraftT
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node;        ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

public:

	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 * reachable trackdir on the new tile creates new node, initializes it
	 * and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node &old_node)
	{
		TrackFollower F(Yapf().GetVehicle());
		bool try_reverse = true; // Whether the vehicle should try to reverse at the end of this tile.
		bool reverse_reservable; // Whether if reservation while reversing on the edge of this tile is possible.

		/* Add nodes for rotation in middle of the tile if possible. */
		if (IsDiagonalTrackdir(old_node.m_key.m_td) &&
				(old_node.m_parent == nullptr || old_node.m_key.m_tile != old_node.m_parent->m_key.m_tile) &&
				(GetAirportTileTracks(old_node.m_key.m_tile) & TRACK_BIT_CROSS) == TRACK_BIT_CROSS) {
			assert(IsValidTrackdir(old_node.m_key.m_td));
			try_reverse = false;
			TrackBits tracks = TRACK_BIT_CROSS & ~TrackToTrackBits(TrackdirToTrack(old_node.m_key.m_td));
			for (TrackdirBits rtds = TrackBitsToTrackdirBits(tracks);
					rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
				Trackdir td = (Trackdir)FindFirstBit(rtds);
				Node &n = Yapf().CreateNewNode();
				// Here there is a choice.
				n.Set(&old_node, old_node.GetTile(), td, true, true);
				Yapf().AddNewNode(n, F);
			}
		}

		if (F.Follow(old_node.m_key.m_tile, old_node.m_key.m_td)) {
			for (TrackdirBits rtds = F.m_new_td_bits; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
				Trackdir td = (Trackdir)FindFirstBit(rtds);
				Node &n = Yapf().CreateNewNode();
				n.Set(&old_node, F.m_new_tile, td, false, true);
				Yapf().AddNewNode(n, F);
			}

			if (!try_reverse) return;

			/* If next tile can have air tracks and the old tile has no track bit cross,
			 * then allow rotation at the end of this tile. */
			reverse_reservable = (GetReservedAirportTracks(F.m_new_tile) & TrackdirBitsToTrackBits(F.m_new_td_bits)) == TRACK_BIT_NONE;

			/* Add nodes for rotation at the end of the tile if possible. */
			DiagDirection reentry_dir = ReverseDiagDir(TrackdirToExitdir(old_node.m_key.m_td));
			TrackdirBits rtds = DiagdirReachesTrackdirs(reentry_dir) &
					TrackBitsToTrackdirBits(GetAirportTileTracks(old_node.m_key.m_tile)) &
					~TrackdirToTrackdirBits(old_node.m_key.m_td);
			for ( ; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
				Trackdir td = (Trackdir)FindFirstBit(rtds);
				Node &n = Yapf().CreateNewNode();
				// reverse_reservable indicates whether there would be
				// a path collision with another path on the next tile.
				n.Set(&old_node, old_node.GetTile(), td, false, reverse_reservable);
				Yapf().AddNewNode(n, F);
			}
		}
	}

	/** return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 'a';
	}

	static Trackdir ChooseAircraftPath(const Aircraft *v, PBSTileInfo *best_dest, bool &path_found, AircraftState dest_state, AircraftPathChoice &path_cache)
	{
		if (!path_cache.empty()) path_cache.clear();

		/* Handle special case: when next tile is destination tile. */
		if (v->tile == v->GetNextTile()) {
			path_found = true;
			best_dest->okay = true;
			best_dest->tile = v->tile;
			best_dest->trackdir = v->trackdir;
			return v->trackdir;
		}

		assert(IsValidTrackdir(v->trackdir));
		Track track= TrackdirToTrack(v->trackdir);
		TrackdirBits trackdirs;
		if ((GetAirportTileTracks(v->tile) & TRACK_BIT_CROSS) == TRACK_BIT_CROSS && (v->x_pos & 0xF) == 8 && (v->y_pos & 0xF) == 8) {
			trackdirs = TrackBitsToTrackdirBits(TRACK_BIT_CROSS);
		} else {
			trackdirs = TrackBitsToTrackdirBits(TrackToTrackBits(track));
		}

		/* Create pathfinder instance. */
		Tpf pf;

		/* Set origin and destination nodes. */
		pf.SetOrigin(v->tile, trackdirs);
		pf.SetDestination(v->IsHelicopter(), dest_state);

		/* Find best path. */
		path_found = pf.FindPath(v);
		bool do_track_reservation = path_found;
		Node *pNode = pf.GetBestNode();

		Trackdir next_trackdir = INVALID_TRACKDIR;

		if (pNode != nullptr) {
			/* walk through the path back to the origin */
			while (pNode->m_parent != nullptr) {
				do_track_reservation &= pNode->m_reservable;
				pNode = pNode->m_parent;
			}
			assert(pNode->GetTile() == v->tile);
			/* return trackdir from the best next node (origin) */
			next_trackdir = pNode->GetTrackdir();
			/* Reserve path from origin till last safe waiting tile */
			if (do_track_reservation) {
				for (const Node *cur = pf.GetBestNode(); cur != nullptr; cur = cur->m_parent) {
					assert(IsValidTrackdir(cur->GetTrackdir()));
					SetAirportTrackReservation(cur->GetTile(), TrackdirToTrack(cur->GetTrackdir()));
					if (cur->m_parent != nullptr && cur->GetIsChoice()) {
						assert(cur->GetTile() == cur->m_parent->GetTile());
						path_cache.td.push_front(cur->GetTrackdir());
						path_cache.tile.push_front(cur->GetTile());
					}
				}
			}

			best_dest->tile = pf.GetBestNode()->GetTile();
			best_dest->trackdir = pf.GetBestNode()->GetTrackdir();
		}

		assert(!path_found || next_trackdir != INVALID_TRACKDIR);
		best_dest->okay = do_track_reservation;

		return next_trackdir;
	}
};

/** YAPF origin provider class for air. */
template <class Types>
class CYapfAirportOriginTileT
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::NodeList::Titem Node; ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables

protected:
	TileIndex    m_orgTile;                       ///< origin tile
	TrackdirBits m_orgTrackdirs;                  ///< origin trackdir mask

	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

public:
	/** Set origin tile / trackdir mask */
	void SetOrigin(TileIndex tile, TrackdirBits trackdirs)
	{
		m_orgTile = tile;
		m_orgTrackdirs = trackdirs;
	}

	/** Called when YAPF needs to place origin nodes into open list */
	void PfSetStartupNodes()
	{
		bool is_choice = (KillFirstBit(m_orgTrackdirs) != TRACKDIR_BIT_NONE);
		for (TrackdirBits tdb = m_orgTrackdirs; tdb != TRACKDIR_BIT_NONE; tdb = KillFirstBit(tdb)) {
			Trackdir td = (Trackdir)FindFirstBit(tdb);
			Node &n1 = Yapf().CreateNewNode();
			n1.Set(nullptr, m_orgTile, td, is_choice, true);
			DirDiff difference = NonOrientedDirDifference(TrackdirToDir(Yapf().GetVehicle()->trackdir), TrackdirToDir(td));
			assert(difference % 2 == 0);
			assert(difference <= 4);
			n1.m_cost = (difference / 2) * YAPF_TILE_LENGTH;
			Yapf().AddStartupNode(n1);
		}
	}
};

/** Destination module of YAPF for aircraft. */
template <class Types>
class CYapfDestinationAircraftBase {
protected:
	AircraftState dest_state;
	bool is_helicopter;

public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::NodeList::Titem Node; ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

	void SetDestination(bool is_helicopter, AircraftState dest_state)
	{
		this->dest_state = dest_state;
		this->is_helicopter = is_helicopter;
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(Node &n)
	{
		return PfDetectDestination(n.m_segment_last_tile, n.m_segment_last_td);
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(TileIndex tile, Trackdir td)
	{
		if (!IsDiagonalTrackdir(td)) return false;

		switch (this->dest_state) {
			case AS_HANGAR:
				/* Reserved extended hangars may be occupied for a long time,
				   so better try finding another hangar tile. */
				return IsHangar(tile) && (IsStandardHangar(tile) || !HasAirportTileAnyReservation(tile));
			case AS_HELIPAD:
				return IsHelipadTile(tile) || IsPlaneApronTile(tile);
			case AS_APRON:
				return IsPlaneApronTile(tile);
			case AS_ON_HOLD_APPROACHING:
			case AS_DESCENDING:
				return IsRunwayStart(tile) && IsLandingTypeTile(tile);
			case AS_START_TAKEOFF:
				return this->is_helicopter ? IsApron(tile) : IsRunwayStart(tile);
			case AS_IDLE:
				return false;
			default:
				NOT_REACHED();
		}
	}

	/**
	 * Called by YAPF to calculate cost estimate. Calculates distance to the destination
	 *  adds it to the actual cost from origin and stores the sum to the Node::m_estimate
	 */
	inline bool PfCalcEstimate(Node &n)
	{
		n.m_estimate = n.m_cost;
		return true;
	}

};

/** Cost Provider module of YAPF for aircraft. */
template <class Types>
class CYapfCostAircraftT
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node; ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables

protected:
	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

public:
	/**
	 * Called by YAPF to calculate the cost from the origin to the given node.
	 * Calculates only the cost of given node, adds it to the parent node cost
	 * and stores the result into Node::m_cost member
	 */
	inline bool PfCalcCost(Node &n, const TrackFollower *)
	{
		int c = 0;

		/* Is current trackdir a diagonal one? */
		c += IsDiagonalTrackdir(n.GetTrackdir()) ? YAPF_TILE_LENGTH : YAPF_TILE_CORNER_LENGTH;

		/* Reserved tiles. */
		if (!IsAirportTileFree(n.GetTile(), n.GetTrackdir())) {
			n.m_reservable = false;
			c *= 4;
			/* Prefer other possible paths where the destination is not occupied. */
			if (Yapf().PfDetectDestination(n.GetTile(), n.GetTrackdir())) {
				c *= 4;
			}
		}

		/* Add cost to avoid vehicles going over aprons and other special tiles as much as possible. */
		if (!IsSimpleTrack(n.GetTile())) c += 8 * YAPF_TILE_LENGTH;

		if (n.GetTile() == n.m_parent->GetTile()) {
			/* Penalty for rotating in a tile (middle or edge position). */
			c += YAPF_TILE_LENGTH;

			if (!IsDiagonalTrack(TrackdirToTrack(n.GetTrackdir())) ||
					!IsDiagonalTrack(TrackdirToTrack(n.m_parent->GetTrackdir()))) {
				/* Extra penalty for rotating at the edge of a tile. */
				c += YAPF_TILE_LENGTH;
			}
		}

		/* Penalty for curves. */
		if (n.GetTrackdir() != NextTrackdir(n.m_parent->GetTrackdir())) {
			/* New trackdir does not match the next one when going straight. */
			c += YAPF_TILE_LENGTH;
		}

		/* Apply it. */
		n.m_cost = n.m_parent->m_cost + c;
		return true;
	}
};

/**
 * Config struct of YAPF for aircraft.
 *  Defines all 6 base YAPF modules as classes providing services for CYapfBaseT.
 */
template <class Tpf_, class Ttrack_follower, template <class Types> class TCYapfCostAircraftT>
struct CYapfAircraft_TypesT
{
	/** Types - shortcut for this struct type */
	typedef CYapfAircraft_TypesT<Tpf_, Ttrack_follower, TCYapfCostAircraftT>  Types;

	/** Tpf - pathfinder type */
	typedef Tpf_                              Tpf;
	/** track follower helper class */
	typedef Ttrack_follower                   TrackFollower;
	/** node list type */
	typedef CAircraftNodeListTrackDir         NodeList;
	typedef Aircraft                          VehicleType;
	/** pathfinder components (modules) */
	typedef CYapfBaseT<Types>                 PfBase;        // base pathfinder class
	typedef CYapfFollowAircraftT<Types>       PfFollow;      // node follower
	typedef CYapfAirportOriginTileT<Types>    PfOrigin;      // origin provider
	typedef CYapfDestinationAircraftBase<Types>      PfDestination; // destination/distance provider
	typedef CYapfSegmentCostCacheNoneT<Types> PfCache;       // segment cost cache provider
	typedef TCYapfCostAircraftT<Types>        PfCost;        // cost provider
};

/* YAPF with reservation - uses TileIndex/Trackdir as Node key, allows 90-deg turns */
struct CYapfAircraft : CYapfT<CYapfAircraft_TypesT<CYapfAircraft, CFollowTrackAirport, CYapfCostAircraftT> > {};

/** Aircraft controller helper - path finder invoker */
Trackdir YapfAircraftFindPath(const Aircraft *v, PBSTileInfo &best_dest, bool &path_found, AircraftState dest_state, AircraftPathChoice &path_cache)
{
	return CYapfAircraft::ChooseAircraftPath(v, &best_dest, path_found, dest_state, path_cache);
}

