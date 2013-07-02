/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file filter_lists.h Functions related to the lists of filters. */

#ifndef FILTER_LISTS_H
#define FILTER_LISTS_H

#include "../string_func.h"
#include "../strings_func.h"

#include "../vehicle_func.h"
#include "../group.h"
#include "../town.h"
#include "../cargotype.h"
#include "../station_base.h"
#include "../sortlist_type.h"
#include "../aircraft.h"
#include "filter_active.h"
#include "../company_base.h"

#include "table/strings.h"


struct FilterLists : FilterBase {
	public:
	FilterActive *active;

	/* Default destructor. */
	~FilterLists() { free(this->active); }

	/**
	 * Build a list (towns, stations, etc).
	 * @param type of the list type to build.
	 */
	void Build(ListType type)
	{
		GUIListForFilter &list = this->lists[type];
		if (list.NeedRebuild()) {
			list.Clear();

			switch (type) {
				case LT_TOWNS:
					for (uint i = Town::GetPoolSize(); i--;) *list.Append() = FilterElement(i);
					break;

				case LT_COMPANIES: {
					const Company *c;
					FOR_ALL_COMPANIES(c) {
						*list.Append() = FilterElement(c->index);
					}
					break;
				}

				case LT_CARGO:
				case LT_CARGO_ACCEPTANCE: {
					const CargoSpec *cg;
					FOR_ALL_SORTED_CARGOSPECS(cg) {
						*list.Append() = FilterElement(cg->Index());
					}
					break;
				}

				case LT_STATION_FACILITIES: {
					for (uint sf = SF_BEGIN; sf < SF_END; sf++) {
						*list.Append() = FilterElement(sf);
					}
					break;
				}

				case LT_STATIONS: {
					const Station *st;
					FOR_ALL_STATIONS(st) {
						if (this->list_owner == INVALID_COMPANY || st->owner == list_owner) {
							*list.Append() = FilterElement(st->index);
						}
					}
					break;
				}

				case LT_ORDERLIST:
					for (uint ol = OLT_BEGIN; ol < OLT_END; ol++) *list.Append() = FilterElement(ol);
					break;

				case LT_VEHICLE_GROUP_PROPERTIES:
					for (uint prop = VGP_BEGIN; prop < VGP_END; prop++) *list.Append() = FilterElement(prop);
					break;

				case LT_CATCHMENT_AREA_PROPERTIES:
					for (uint prop = ZW_BEGIN; prop < ZW_END; prop++) *list.Append() = FilterElement(prop);
					break;

				case LT_TOWN_PROPERTIES:
					for (uint prop = TP_BEGIN; prop < TP_END; prop++) *list.Append() = FilterElement(prop);
					break;

				default: NOT_REACHED();
			}

			list.Compact();
			list.RebuildDone();
		}

		Sort(type);
	}

	/**
	 * Build towns, stations and cargo lists.
	 * @param mask of the lists to build.
	*/
	void Build(uint mask)
	{
		for (uint i = LT_END; i--;) {
			if (!HasBit(mask, i)) continue;
			lists[i].ForceRebuild();
			this->Build((ListType)i);
		}
	}

	/**
	 * Reset all the elements of a list of a filter.
	 * @param i List to build.
	 * @note Active lists are not changed.
	 */
	void Reset(uint i)
	{
		for (uint j = this->lists[i].Length(); j--;) {
			this->lists[i][j].Reset();
		}
	}

	/**
	 * Set all the elements (of all lists) unactive.
	 */
	void Reset()
	{
		this->active->Reset();
		for (uint i = LT_END; i--;) {
			this->Reset(i);
		}
	}

	/**
	 * Handle a click on an element of the filter.
	 * @param type List type of the element (needed to decide whether to toggle or rotate element).
	 * @param index Index of the element on the list.
	 */
	void OnClick(ListType type, FilterElement &element)
	{
		if (element.IsActive()) {
			if (type == LT_CATCHMENT_AREA_PROPERTIES) element.ToggleActive();
			else element.Rotate();
			this->active->Update(type, element);
		} else {
			if (type == LT_CATCHMENT_AREA_PROPERTIES) element.ToggleActive();
			else element.Rotate();
			*this->active->lists[type].Append() = FilterElement(element);
		}
	}

	/**
	 * Copy the activity of elements to the GUI elements (active -> lists).
	 * @param type of the list to copy.
	 */
	void GetFromActive(ListType type)
	{
		/* Copy the state of active elements. */
		for (FilterElement *active_el = this->active->lists[type].Begin(); active_el != this->active->lists[type].End(); active_el++) {
			FilterElement *object = this->lists[type].Find(*active_el);
			object->Copy(*active_el);
		}
	}

	/**
	 * GUI lists copy the state of the active/inactive elements.
	 */
	void GetFromActive()
	{
		for (uint i = LT_END; i--;) {
			/* Reset the elements */
			this->Reset((ListType)i);
			this->GetFromActive((ListType)i);
		}
	}
};

#endif /* FILTER_LISTS_H */