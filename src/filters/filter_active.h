/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filter_active.h Functions related to the active elements of a filter. */

#ifndef FILTER_ACTIVE_H
#define FILTER_ACTIVE_H

#include "../string_func.h"
#include "../strings_func.h"

#include "../vehicle_func.h"
#include "../group.h"
#include "../town.h"
#include "../cargotype.h"
#include "../station_base.h"
#include "../aircraft.h"
#include "../industry.h"
#include "../sortlist_type.h"
#include "../window_gui.h"

#include "filter_element.h"

typedef GUIList<FilterElement> GUIListForFilter;

struct FilterBase {
	public:
	Owner list_owner;                     ///< Owner the list applies to.
	GUIListForFilter lists[LT_END];       ///< The lists of elements (towns, companies,...).

	void Sort(ListType type);
};

struct FilterActive : FilterBase {

	/**
	 * Count how many active elements this filter has.
	 */
	uint Count() const
	{
		uint lines = 0;
		for (uint j = LT_END; j--;) lines += this->lists[j].Length();
		return lines;
	}

	/**
	 * Set all the elements (of all lists) inactive.
	 */
	void Reset()
	{
		for (uint i = LT_END; i--;) this->lists[i].Clear();
	}

	/**
	 * After a filter element changes by extern action, update the object.
	 * @param type of object that has to be copied/changed.
	 * @param object to copy/update.
	 */
	void Update(ListType type, const FilterElement &object)
 	{
		FilterElement *to_update = this->lists[type].Find(object);

		if (object.IsActive()) {
			if (to_update == this->lists[type].End()) {
				to_update = new FilterElement(object);
			} else {
				to_update->Copy(object);
			}
		} else if (to_update != this->lists[type].End()) {
			this->lists[type].Erase(to_update);
		}
	}

	void SortAll() {
		for (uint i = LT_END; i--;) this->Sort((ListType)i);
	}

	void CTRLClickOnActive(uint number);

	bool FilterTest(const Vehicle *v) const;
	bool FilterTest(const Group *g) const;
	bool FilterTest(const CargoSpec *cs) const;
	bool FilterTest(const Industry *i) const;
	bool FilterTest(const Town *town) const;
	bool FilterTest(const Station *st) const;
	bool FilterTestCA(const Station *st) const;
};

#endif /* FILTER_ACTIVE_H */