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
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node &old_node)
	{
		TrackFollower F(Yapf().GetVehicle());
		if (F.Follow(old_node.m_key.m_tile, old_node.m_key.m_td)) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	/**
	 * Find the node containing the point where to end a reservation.
	 * For aircraft, it depends on current orders.
	 * @param path Last node of the path.
	 * @param v Aircraft reserving the path.
	 * @return Last node to reserve.
	 */
	static const Node *FindSafePosition(const Node *path, const Aircraft *v)
	{
		const Node *last_free = NULL;
		assert(v != NULL);

		for (const Node *node = path; node != NULL; node = node->m_parent) {
			TileIndex tile = node->GetTile();

			/* Special cases on crossing the starting tile. */
			if (v->tile == node->GetTile()) {
				if (node->m_parent == NULL) return last_free;
				last_free = NULL;
			}

			if (!IsSafeWaitingPosition(v, tile, node->GetTrackdir())) {
				last_free = NULL;
			} else if (last_free == NULL) {
				last_free = node;
			}
		}

		NOT_REACHED();
	}

	/** return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 'a';
	}

	static Trackdir ChooseAircraftTrack(const Aircraft *v, TileIndex tile, DiagDirection enterdir, TrackBits tracks, bool &path_found)
	{
		/* handle special case - when next tile is destination tile */
		if (tile == v->dest_tile) {
			/* convert tracks to trackdirs */
			TrackdirBits trackdirs = (TrackdirBits)(tracks | ((int)tracks << 8));
			/* limit to trackdirs reachable from enterdir */
			trackdirs &= DiagdirReachesTrackdirs(enterdir);

			/* use vehicle's current direction if that's possible, otherwise use first usable one. */
			Trackdir veh_dir = v->GetVehicleTrackdir();
			return ((trackdirs & TrackdirToTrackdirBits(veh_dir)) != 0) ? veh_dir : (Trackdir)FindFirstBit2x64(trackdirs);
		}

		/* move back to the old tile/trackdir (where aircraft is coming from) */
		TileIndex src_tile = TILE_ADD(tile, TileOffsByDiagDir(ReverseDiagDir(enterdir)));
		Trackdir trackdir = v->GetVehicleTrackdir();
		assert(IsValidTrackdir(trackdir));

		/* convert origin trackdir to TrackdirBits */
		TrackdirBits trackdirs = TrackdirToTrackdirBits(trackdir);
		/* get available trackdirs on the destination tile */
		TrackdirBits dest_trackdirs = TrackStatusToTrackdirBits(GetTileTrackStatus(v->dest_tile, TRANSPORT_WATER, 0));

		/* create pathfinder instance */
		Tpf pf;
		/* set origin and destination nodes */
		pf.SetOrigin(src_tile, trackdirs);
		pf.SetDestination(v->dest_tile, dest_trackdirs);
		/* find best path */
		path_found = pf.FindPath(v);

		Trackdir next_trackdir = INVALID_TRACKDIR; // this would mean "path not found"

		Node *pNode = pf.GetBestNode();
		if (pNode != NULL) {
			/* walk through the path back to the origin */
			Node *pPrevNode = NULL;
			while (pNode->m_parent != NULL) {
				pPrevNode = pNode;
				pNode = pNode->m_parent;
			}
			/* return trackdir from the best next node (direct child of origin) */
			Node &best_next_node = *pPrevNode;
			assert(best_next_node.GetTile() == tile);
			next_trackdir = best_next_node.GetTrackdir();
			/* Reserve path from origin till last safe waiting tile */
			for (const Node *cur = FindSafePosition(pf.GetBestNode(), v); cur != NULL; cur = cur->m_parent) {
				if (v->tile == cur->GetTile()) break;
				SetWaterTrackReservation(cur->GetTile(), TrackdirToTrack(cur->GetTrackdir()), true);
				if (IsTileType(cur->GetTile(), MP_TUNNELBRIDGE)) cur = cur->m_parent;
			}
		}
		return next_trackdir;
	}

	/**
	 * Check whether a aircraft should reverse to reach its destination.
	 * Called when leaving depot.
	 * @param v Aircraft
	 * @param tile Current position
	 * @param td1 Forward direction
	 * @param td2 Reverse direction
	 * @return true if the reverse direction is better
	 */
	static bool CheckAircraftReverse(const Aircraft *v, TileIndex tile, Trackdir td1, Trackdir td2)
	{
		/* get available trackdirs on the destination tile */
		TrackdirBits dest_trackdirs = TrackStatusToTrackdirBits(GetTileTrackStatus(v->dest_tile, TRANSPORT_WATER, 0));

		/* create pathfinder instance */
		Tpf pf;
		/* set origin and destination nodes */
		pf.SetOrigin(tile, TrackdirToTrackdirBits(td1) | TrackdirToTrackdirBits(td2));
		pf.SetDestination(v->dest_tile, dest_trackdirs);
		/* find best path */
		if (!pf.FindPath(v)) return false;

		Node *pNode = pf.GetBestNode();
		if (pNode == NULL) return false;

		/* path was found
		 * walk through the path back to the origin */
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		Trackdir best_trackdir = pNode->GetTrackdir();
		assert(best_trackdir == td1 || best_trackdir == td2);
		return best_trackdir == td2;
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
		/* base tile cost depending on distance */
		int c = IsDiagonalTrackdir(n.GetTrackdir()) ? YAPF_TILE_LENGTH : YAPF_TILE_CORNER_LENGTH;
		/* additional penalty for curves */
		if (n.GetTrackdir() != NextTrackdir(n.m_parent->GetTrackdir())) {
			/* new trackdir does not match the next one when going straight */
			c += YAPF_TILE_LENGTH;
		}

		if (IsHangar(n.GetTile())) {
			c += YAPF_TILE_LENGTH;
			if (IsBigDepot(n.GetTile()) && GetReservationAsDepot(n.GetTile())) c += YAPF_INFINITE_PENALTY;
		}

		/* Skipped tile cost for aqueducts. */
		c += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

		/* apply it */
		n.m_cost = n.m_parent->m_cost + c;
		return true;
	}
};

/**
 * Config struct of YAPF for aircraft.
 *  Defines all 6 base YAPF modules as classes providing services for CYapfBaseT.
 */
template <class Tpf_, class Ttrack_follower, class Tnode_list>
struct CYapfAircraft_TypesT
{
	/** Types - shortcut for this struct type */
	typedef CYapfAircraft_TypesT<Tpf_, Ttrack_follower, Tnode_list>  Types;

	/** Tpf - pathfinder type */
	typedef Tpf_                              Tpf;
	/** track follower helper class */
	typedef Ttrack_follower                   TrackFollower;
	/** node list type */
	typedef Tnode_list                        NodeList;
	typedef Aircraft                              VehicleType;
	/** pathfinder components (modules) */
	typedef CYapfBaseT<Types>                 PfBase;        // base pathfinder class
	typedef CYapfFollowAircraftT<Types>           PfFollow;      // node follower
	typedef CYapfOriginTileT<Types>           PfOrigin;      // origin provider
	typedef CYapfDestinationTileT<Types>      PfDestination; // destination/distance provider
	typedef CYapfSegmentCostCacheNoneT<Types> PfCache;       // segment cost cache provider
	typedef CYapfCostAircraftT<Types>             PfCost;        // cost provider
};

/* YAPF type 1 - uses TileIndex/Trackdir as Node key, allows 90-deg turns */
struct CYapfAircraft1 : CYapfT<CYapfAircraft_TypesT<CYapfAircraft1, CFollowTrackAirport    , CAircraftNodeListTrackDir> > {};
/* YAPF type 2 - uses TileIndex/DiagDirection as Node key, allows 90-deg turns */
struct CYapfAircraft2 : CYapfT<CYapfAircraft_TypesT<CYapfAircraft2, CFollowTrackAirport    , CAircraftNodeListExitDir > > {};

/** Aircraft controller helper - path finder invoker */
Track YapfAircraftChooseTrack(const Aircraft *v, TileIndex tile, DiagDirection enterdir, TrackBits tracks, bool &path_found)
{
	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseAircraftTrack)(const Aircraft*, TileIndex, DiagDirection, TrackBits, bool &path_found);
	PfnChooseAircraftTrack pfnChooseAircraftTrack = CYapfAircraft2::ChooseAircraftTrack; // default: ExitDir, allow 90-deg

	pfnChooseAircraftTrack = &CYapfAircraft1::ChooseAircraftTrack; // Trackdir, allow 90-deg

	Trackdir td_ret = pfnChooseAircraftTrack(v, tile, enterdir, tracks, path_found);
	return (td_ret != INVALID_TRACKDIR) ? TrackdirToTrack(td_ret) : INVALID_TRACK;
}

bool YapfAircraftCheckReverse(const Aircraft *v)
{
	Trackdir td = v->GetVehicleTrackdir();
	Trackdir td_rev = ReverseTrackdir(td);
	TileIndex tile = v->tile;

	typedef bool (*PfnCheckReverseAircraft)(const Aircraft*, TileIndex, Trackdir, Trackdir);
	PfnCheckReverseAircraft pfnCheckReverseAircraft = CYapfAircraft2::CheckAircraftReverse; // default: ExitDir, allow 90-deg

	pfnCheckReverseAircraft = &CYapfAircraft1::CheckAircraftReverse; // Trackdir, allow 90-deg

	bool reverse = pfnCheckReverseAircraft(v, tile, td, td_rev);

	return reverse;
}
