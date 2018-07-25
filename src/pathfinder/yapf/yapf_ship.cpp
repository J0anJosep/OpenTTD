/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_ship.cpp Implementation of YAPF for ships. */

#include "../../stdafx.h"
#include "../../ship.h"

#include "yapf.hpp"
#include "yapf_node_ship.hpp"

#include "../../safeguards.h"

/** Node Follower module of YAPF for ships */
template <class Types>
class CYapfFollowShipT
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
	 * For ships, the first time we collide with another path or last node.
	 * @param path Last node of the path.
	 * @param v Ship reserving the path.
	 * @return Last node to reserve.
	 */
	static const Node *FindSafePosition(const Node *path, TileIndex src_tile)
	{
		const Node *last_free = NULL;
		assert(IsValidTile(src_tile));

		for (const Node *node = path; node != NULL; node = node->m_parent) {
			TileIndex tile = node->GetTile();

			if (src_tile == tile && node->m_parent != NULL) {
				last_free = NULL;
				continue;
			}

			if (!IsWaterPositionFree(tile, node->GetTrackdir())) {
				last_free = NULL;
				/* Skip tiles of the same lock. */
				while (CheckSameLock(tile, node->m_parent->GetTile())) node = node->m_parent;
			} else if (last_free == NULL) {
				last_free = node;
			}
		}

		return last_free;
	}

	/** return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 'w';
	}

	static Trackdir ChooseShipTrack(const Ship *v, TileIndex tile, DiagDirection enterdir, TrackBits tracks, bool &path_found)
	{
		/* Get available trackdirs on origin. */
		TrackdirBits trackdirs = TrackBitsToTrackdirBits(tracks) & DiagdirReachesTrackdirs(enterdir);

		/* handle special case - when next tile is destination tile */
		if (tile == v->dest_tile) {
			/* Use vehicle's current direction if that's possible, otherwise use first usable one. */
			Trackdir veh_dir = v->GetVehicleTrackdir();
			return (HasTrackdir(trackdirs, veh_dir)) ? veh_dir : (Trackdir)FindFirstBit2x64(trackdirs);
		}

		/* Get available trackdirs on destination tile. */
		assert(trackdirs != TRACKDIR_BIT_NONE);
		TrackdirBits dest_trackdirs = TrackStatusToTrackdirBits(GetTileTrackStatus(v->dest_tile, TRANSPORT_WATER, 0));

		/* create pathfinder instance */
		Tpf pf;
		/* set origin and destination nodes */
		pf.SetOrigin(tile, trackdirs);
		pf.SetDestination(v->dest_tile, dest_trackdirs);
		/* find best path */
		path_found = pf.FindPath(v);

		Trackdir next_trackdir = INVALID_TRACKDIR; // this would mean "path not found"

		const Node *pNode = pf.GetBestNode();
		if (pNode != NULL) {
			/* Walk through the path back to the origin. */
			while (pNode->m_parent != NULL) pNode = pNode->m_parent;

			assert(pNode->GetTile() == tile);
			next_trackdir = pNode->GetTrackdir();
			assert(HasTrackdir(trackdirs, next_trackdir));

			/* Find the last track of the path that can be safely reserved (safe position). */
			pNode = FindSafePosition(pf.GetBestNode(), tile);

			/* No safe position. */
			if (pNode == NULL) return INVALID_TRACKDIR;

			/* Reserve path from origin till last safe waiting tile. */
			for (; pNode != NULL; pNode = pNode->m_parent) {
				SetWaterTrackReservation(pNode->GetTile(), TrackdirToTrack(pNode->GetTrackdir()), true);
				if (IsTileType(pNode->GetTile(), MP_TUNNELBRIDGE)) pNode = pNode->m_parent;
			}
		}
		return next_trackdir;
	}

	/**
	 * Check whether a ship should reverse to reach its destination.
	 * Called when leaving depot.
	 * @param v Ship
	 * @param tile Current position
	 * @param td1 Forward direction
	 * @param td2 Reverse direction
	 * @return true if the reverse direction is better
	 */
	static bool CheckShipReverse(const Ship *v, TileIndex tile, Trackdir td1, Trackdir td2)
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

/** Cost Provider module of YAPF for ships */
template <class Types>
class CYapfCostShipT
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

		if (_settings_game.pf.ship_path_reservation) {
			/* Apply a penalty for using reserved trackdirs on a tile. */
			if (HasWaterTrackReservation(n.GetTile()) &&
					TracksOverlap(TrackToTrackBits(TrackdirToTrack(n.GetTrackdir())) | GetReservedWaterTracks(n.GetTile()))) {
				c += 3 * (IsDiagonalTrackdir(n.GetTrackdir()) ? YAPF_TILE_LENGTH : YAPF_TILE_CORNER_LENGTH);
			}
		}

		/* additional penalty for curves */
		if (n.GetTrackdir() != NextTrackdir(n.m_parent->GetTrackdir())) {
			/* new trackdir does not match the next one when going straight */
			c += YAPF_TILE_LENGTH;
		}

		if (IsShipDepotTile(n.GetTile()) || IsDockTile(n.GetTile())) c += YAPF_TILE_LENGTH;

		/* Skipped tile cost for aqueducts. */
		c += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

		/* Ocean/canal speed penalty. */
		const ShipVehicleInfo *svi = ShipVehInfo(Yapf().GetVehicle()->engine_type);
		byte speed_frac = (GetEffectiveWaterClass(n.GetTile()) == WATER_CLASS_SEA) ? svi->ocean_speed_frac : svi->canal_speed_frac;
		if (speed_frac > 0) c += YAPF_TILE_LENGTH * (1 + tf->m_tiles_skipped) * speed_frac / (256 - speed_frac);

		/* apply it */
		n.m_cost = n.m_parent->m_cost + c;
		return true;
	}
};

/**
 * Config struct of YAPF for ships.
 *  Defines all 6 base YAPF modules as classes providing services for CYapfBaseT.
 */
template <class Tpf_, class Ttrack_follower, class Tnode_list>
struct CYapfShip_TypesT
{
	/** Types - shortcut for this struct type */
	typedef CYapfShip_TypesT<Tpf_, Ttrack_follower, Tnode_list>  Types;

	/** Tpf - pathfinder type */
	typedef Tpf_                              Tpf;
	/** track follower helper class */
	typedef Ttrack_follower                   TrackFollower;
	/** node list type */
	typedef Tnode_list                        NodeList;
	typedef Ship                              VehicleType;
	/** pathfinder components (modules) */
	typedef CYapfBaseT<Types>                 PfBase;        // base pathfinder class
	typedef CYapfFollowShipT<Types>           PfFollow;      // node follower
	typedef CYapfOriginTileT<Types>           PfOrigin;      // origin provider
	typedef CYapfDestinationTileT<Types>      PfDestination; // destination/distance provider
	typedef CYapfSegmentCostCacheNoneT<Types> PfCache;       // segment cost cache provider
	typedef CYapfCostShipT<Types>             PfCost;        // cost provider
};

/* YAPF type 1 - uses TileIndex/Trackdir as Node key, allows 90-deg turns */
struct CYapfShip1 : CYapfT<CYapfShip_TypesT<CYapfShip1, CFollowTrackWater    , CShipNodeListTrackDir> > {};
/* YAPF type 2 - uses TileIndex/DiagDirection as Node key, allows 90-deg turns */
struct CYapfShip2 : CYapfT<CYapfShip_TypesT<CYapfShip2, CFollowTrackWater    , CShipNodeListExitDir > > {};
/* YAPF type 3 - uses TileIndex/Trackdir as Node key, forbids 90-deg turns */
struct CYapfShip3 : CYapfT<CYapfShip_TypesT<CYapfShip3, CFollowTrackWaterNo90, CShipNodeListTrackDir> > {};

/** Ship controller helper - path finder invoker */
Track YapfShipChooseTrack(const Ship *v, TileIndex tile, DiagDirection enterdir, TrackBits tracks, bool &path_found)
{
	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseShipTrack)(const Ship*, TileIndex, DiagDirection, TrackBits, bool &path_found);
	PfnChooseShipTrack pfnChooseShipTrack = CYapfShip2::ChooseShipTrack; // default: ExitDir, allow 90-deg

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnChooseShipTrack = &CYapfShip3::ChooseShipTrack; // Trackdir, forbid 90-deg
	} else if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnChooseShipTrack = &CYapfShip1::ChooseShipTrack; // Trackdir, allow 90-deg
	}

	Trackdir td_ret = pfnChooseShipTrack(v, tile, enterdir, tracks, path_found);
	return (td_ret != INVALID_TRACKDIR) ? TrackdirToTrack(td_ret) : INVALID_TRACK;
}

bool YapfShipCheckReverse(const Ship *v)
{
	Trackdir td = v->GetVehicleTrackdir();
	Trackdir td_rev = ReverseTrackdir(td);
	TileIndex tile = v->tile;

	typedef bool (*PfnCheckReverseShip)(const Ship*, TileIndex, Trackdir, Trackdir);
	PfnCheckReverseShip pfnCheckReverseShip = CYapfShip2::CheckShipReverse; // default: ExitDir, allow 90-deg

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnCheckReverseShip = &CYapfShip3::CheckShipReverse; // Trackdir, forbid 90-deg
	} else if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnCheckReverseShip = &CYapfShip1::CheckShipReverse; // Trackdir, allow 90-deg
	}

	bool reverse = pfnCheckReverseShip(v, tile, td, td_rev);

	return reverse;
}
