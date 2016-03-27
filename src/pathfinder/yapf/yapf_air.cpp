/* $Id$ */

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

#include "yapf.hpp"
#include "yapf_node_air.hpp"

#include "../../safeguards.h"

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
	/** add multiple nodes - direct children of the given node */
	inline void AddMultipleNodesForAir(Node *parent, const TrackFollower &tf)
	{
		for (TrackdirBits rtds = tf.m_new_td_bits; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			Trackdir td = (Trackdir)FindFirstBit2x64(rtds);
			if (!IsAirportPositionFree(tf.m_new_tile, td)) continue;
			Node &n = Yapf().CreateNewNode();
			// is_choice will be set to false, as it isn't used anyway.
			n.Set(parent, tf.m_new_tile, td, false);
			Yapf().AddNewNode(n, tf);
		}
	}

	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node &old_node)
	{
		TrackFollower F(Yapf().GetVehicle());

		if (IsDiagonalTrackdir(old_node.m_key.m_td) &&
				(old_node.m_parent == NULL || old_node.m_key.m_tile != old_node.m_parent->m_key.m_tile) &&
				(GetAirportTileTracks(old_node.m_key.m_tile) & TRACK_BIT_CROSS) == TRACK_BIT_CROSS) {
			TrackBits tracks = TRACK_BIT_CROSS & ~TrackToTrackBits(TrackdirToTrack(old_node.m_key.m_td));
			for (TrackdirBits rtds = TrackBitsToTrackdirBits(tracks);
					rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
				Trackdir td = (Trackdir)FindFirstBit2x64(rtds);
				Node &n = Yapf().CreateNewNode();
				// is_choice will be set to false, as it isn't used anyway.
				n.Set(&old_node, old_node.GetTile(), td, false);
				Yapf().AddNewNode(n, F);
			}
		}

		if (F.Follow(old_node.m_key.m_tile, old_node.m_key.m_td)) {
			Yapf().AddMultipleNodesForAir(&old_node, F);
		}
	}

	/** return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 'a';
	}

	static Trackdir ChooseAircraftPath(const Aircraft *v, PBSTileInfo *best_dest, bool skip_reservation)
	{
		/* Handle special case: when next tile is destination tile. */
		if (v->tile == v->dest_tile) {
			return v->trackdir;
		}

		TrackdirBits trackdirs = TrackBitsToTrackdirBits(GetAirportTileTracks(v->tile) & TRACK_BIT_CROSS);

		/* Create pathfinder instance. */
		Tpf pf;
		/* set origin and destination nodes */
		pf.SetOrigin(v->tile, trackdirs);
		pf.SetDestination(v);
		/* Find best path. */
		if (!pf.FindPath(v)) return INVALID_TRACKDIR;

		Trackdir next_trackdir = INVALID_TRACKDIR; // this would mean "path not found"

		Node *pNode = pf.GetBestNode();
		if (pNode == NULL || pNode->m_cost > YAPF_INFINITE_PENALTY) return INVALID_TRACKDIR;

		if (pNode != NULL) {
			/* walk through the path back to the origin */
			while (pNode->m_parent != NULL) {
				pNode = pNode->m_parent;
			}
			assert(pNode->GetTile() == v->tile);
			/* return trackdir from the best next node (origin) */
			next_trackdir = pNode->GetTrackdir();
			/* Reserve path from origin till last safe waiting tile */
			if (!skip_reservation) {
				for (const Node *cur = pf.GetBestNode(); cur != NULL; cur = cur->m_parent) {
					SetAirportTrackReservation(cur->GetTile(), TrackdirToTrack(cur->GetTrackdir()));
				}
			}
		}
		assert(next_trackdir != INVALID_TRACKDIR);
		assert(IsDiagonalTrackdir(next_trackdir));

		best_dest->okay = true;
		best_dest->tile = pf.GetBestNode()->GetTile();
		best_dest->trackdir = pf.GetBestNode()->GetTrackdir();

		return next_trackdir;
	}

	static bool CheckPathExists(TileIndex tile_1, TileIndex tile_2)
	{
		if (tile_1 == INVALID_TILE) return true;
		assert(tile_2 != INVALID_TILE);
		assert(tile_1 != tile_2);

		/* Create pathfinder instance. */
		Tpf pf;

		TrackdirBits trackdirs_1 = TrackBitsToTrackdirBits(GetAirportTileTracks(tile_1) & TRACK_BIT_CROSS);
		pf.SetOrigin(tile_1, trackdirs_1);
		TrackdirBits trackdirs_2 = TrackBitsToTrackdirBits(GetAirportTileTracks(tile_2) & TRACK_BIT_CROSS);
		pf.SetDestination(tile_2, trackdirs_2);

		return pf.FindPath(NULL);
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
		assert(IsDiagonalTrackdir(Yapf().GetVehicle()->trackdir));
		DiagDirection veh_diag_dir = TrackdirToExitdir(Yapf().GetVehicle()->trackdir);
		for (TrackdirBits tdb = m_orgTrackdirs; tdb != TRACKDIR_BIT_NONE; tdb = KillFirstBit(tdb)) {
			Trackdir td = (Trackdir)FindFirstBit2x64(tdb);
			assert(IsDiagonalTrackdir(td));
			Node &n1 = Yapf().CreateNewNode();
			n1.Set(NULL, m_orgTile, td, is_choice);
			n1.m_cost = NonOriented90DegreeDifference(veh_diag_dir, TrackdirToExitdir(td)) * YAPF_TILE_LENGTH;
			Yapf().AddStartupNode(n1);
		}
	}
};

/** Destination module of YAPF for aircraft. */
template <class Types>
class CYapfDestinationAircraftBase {
protected:
	StationID station_id;
	AircraftMovement air_dest;

public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::NodeList::Titem Node; ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf *>(this);
	}

	void SetDestination(const Aircraft *v)
	{
		station_id = GetStationIndex(v->tile);
		air_dest = v->next_state;
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
		if (this->station_id != INVALID_STATION &&
				this->station_id != GetStationIndex(tile)) return false;

		switch (this->air_dest) {
			case AM_HANGAR:
				return IsHangar(tile);
			case AM_HELIPAD:
			case AM_HELICOPTER_TAKEOFF:
				if (IsHeliportTile(tile) || IsHelipadTile(tile)) return true;
				FALLTHROUGH;
			case AM_TERMINAL:
				return IsPlaneTerminalTile(tile);
			case AM_TAKEOFF:
				return IsRunwayStart(tile);
			case AM_IDLE:
				return true;
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
	 *  Calculates only the cost of given node, adds it to the parent node cost
	 *  and stores the result into Node::m_cost member
	 */
	inline bool PfCalcCost(Node &n, const TrackFollower *tf)
	{
		int c = 0;
		if (HasAirportTileAnyReservation(n.GetTile())) {
			//DEBUG(misc, 0, "Found reserved tile");
			c += YAPF_INFINITE_PENALTY;
		}

		/* Is current trackdir a diagonal one? */
		if (IsDiagonalTrackdir(n.GetTrackdir())) {
			c += YAPF_TILE_LENGTH;
		} else {
			c += YAPF_TILE_CORNER_LENGTH;
			if (!CheckFreeAssociatedAirportTile(n.GetTile(), TrackdirToTrack(n.GetTrackdir()))) {
				//DEBUG(misc, 0, "Non-free associated");
				c += YAPF_INFINITE_PENALTY;
			}
		}

		if (!IsAirportPositionFree(n.GetTile(), n.GetTrackdir())) {
			//DEBUG(misc, 0, "Adding a no airport position free");
			c += YAPF_INFINITE_PENALTY;
		}

		/* Add an additional cost for preventing vehicles going over terminals and special tiles
		 * when they are heading towards another destination tile. */
		if (!IsSimpleTrack(n.GetTile())) {
			c += 8 * YAPF_TILE_LENGTH;
		}

		/* Additional penalty for rotating in the middle of a tile. */
		if (n.GetTile() == n.m_parent->GetTile()) {
			//DEBUG(misc, 0, "aDDing cost");
			c += YAPF_TILE_LENGTH;
			if (!IsAirportPositionFree(n.GetTile(), ReverseTrackdir(n.GetTrackdir()))) {
				c += YAPF_INFINITE_PENALTY;
			}
		}

		/* Additional penalty for curves. */
		if (n.GetTrackdir() != NextTrackdir(n.m_parent->GetTrackdir())) {
			/* new trackdir does not match the next one when going straight */
			c += YAPF_TILE_LENGTH;
		}

		/* Apply it. */
		n.m_cost = n.m_parent->m_cost + c;
		return true;
	}
};

/** Cost Provider module of YAPF for aircraft. */
template <class Types>
class CYapfCostAircraftSkipReservationT
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
	 *  Calculates only the cost of given node, adds it to the parent node cost
	 *  and stores the result into Node::m_cost member
	 */
	inline bool PfCalcCost(Node &n, const TrackFollower *tf)
	{
		/* Is current trackdir a diagonal one? */
		bool is_diagonal_trackdir = IsDiagonalTrackdir(n.GetTrackdir());
		/* base tile cost depending on distance */
		int c = is_diagonal_trackdir ? YAPF_TILE_LENGTH : YAPF_TILE_CORNER_LENGTH;
		/* additional penalty for curves */
		if (n.GetTrackdir() != NextTrackdir(n.m_parent->GetTrackdir())) {
			/* new trackdir does not match the next one when going straight */
			c += YAPF_TILE_LENGTH;
		}

		if (IsHangar(n.GetTile())) c += YAPF_TILE_LENGTH;

		/* apply it */
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
/* YAPF no reservation checks - uses TileIndex/Trackdir as Node key, allows 90-deg turns */
struct CYapfAircraftSkipReservation : CYapfT<CYapfAircraft_TypesT<CYapfAircraftSkipReservation, CFollowTrackAirport, CYapfCostAircraftSkipReservationT> > {};

/** Aircraft controller helper - path finder invoker */
Trackdir YapfAircraftFindPath(const Aircraft *v, PBSTileInfo *best_dest, bool skip_reservation)
{
	/* Default is YAPF allowing 90-deg with reservation. */
	typedef Trackdir (*PfnChooseAircraftPath)(const Aircraft *, PBSTileInfo *, bool);
	PfnChooseAircraftPath pfnChooseAircraftPath = &CYapfAircraft::ChooseAircraftPath; // Trackdir, allow 90-deg allow 90-deg, check for reservations

	if (skip_reservation) pfnChooseAircraftPath = &CYapfAircraftSkipReservation::ChooseAircraftPath; // Trackdir, allow 90-deg, do not care for reservations

	return pfnChooseAircraftPath(v, best_dest, skip_reservation);
}

/**
 * Config struct of YAPF for aircraft paths between tiles.
 */
template <class Tpf_, class Ttrack_follower, template <class Types> class TCYapfCostAircraftT>
struct CYapfPath_TypesT
{
	/** Types - shortcut for this struct type */
	typedef CYapfPath_TypesT<Tpf_, Ttrack_follower, TCYapfCostAircraftT>  Types;

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
	typedef CYapfOriginTileT<Types>           PfOrigin;      // origin provider
	typedef CYapfDestinationTileT<Types>      PfDestination; // destination/distance provider
	typedef CYapfSegmentCostCacheNoneT<Types> PfCache;       // segment cost cache provider
	typedef TCYapfCostAircraftT<Types>        PfCost;        // cost provider
};

/* YAPF for checking the existence of a path between two tiles */
struct CYapfPath : CYapfT<CYapfPath_TypesT<CYapfPath, CFollowTrackAirport, CYapfCostAircraftSkipReservationT> > {};

/** Aircraft controller helper - path finder invoker. */
bool YapfDoesPathExist(TileIndex tile_1, TileIndex tile_2)
{
	return CYapfPath::CheckPathExists(tile_1, tile_2);
}
