/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_node_aircraft.hpp Node tailored for aircraft pathfinding. */

#ifndef YAPF_NODE_AIRCRAFT_HPP
#define YAPF_NODE_AIRCRAFT_HPP

/** Yapf Node for aircraft YAPF */
template <class Tkey_>
struct CYapfAircraftNodeT : CYapfNodeT<Tkey_, CYapfAircraftNodeT<Tkey_> > {
	typedef CYapfNodeT<Tkey_, CYapfAircraftNodeT<Tkey_> > base;

	TileIndex m_segment_last_tile;
	Trackdir  m_segment_last_td;
	bool m_reservable;

	void Set(CYapfAircraftNodeT *parent, TileIndex tile, Trackdir td, bool is_choice, bool reservable)
	{
		base::Set(parent, tile, td, is_choice);
		m_segment_last_tile = tile;
		m_segment_last_td = td;
		m_reservable = reservable;
	}
};

typedef CYapfAircraftNodeT<CYapfNodeKeyTrackDir> CYapfAircraftNodeTrackDir;
typedef CNodeList_HashTableT<CYapfAircraftNodeTrackDir, 10, 12> CAircraftNodeListTrackDir;

#endif /* YAPF_NODE_AIRCRAFT_HPP */
