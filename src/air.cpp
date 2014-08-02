/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file air.cpp Implementation of air specific functions. */

#include "stdafx.h"
#include "station_map.h"
#include "date_func.h"
#include "company_func.h"
#include "company_base.h"
#include "engine_base.h"
#include "aircraft.h"
#include "air.h"

/**
 * Finds out if a company has a certain airtype available
 * @param company the company in question
 * @param airtype requested AirType
 * @return true if company has requested AirType available
 */
bool HasAirTypeAvail(const CompanyID company, const AirType airtype)
{
	return HasBit(Company::Get(company)->avail_airtypes, airtype);
}

/**
 * Validate functions for air building.
 * @param air the airtype to check.
 * @return true if the current company may build the air.
 */
bool ValParamAirType(const AirType air)
{
	return air < AIRTYPE_END && HasAirTypeAvail(_current_company, air);
}

/**
 * Returns the "best" airtype a company can build.
 * As the AI doesn't know what the BEST one is, we have our own priority list
 * here. When adding new airtypes, modify this function
 * @param company the company "in action"
 * @return The "best" airtype a company has available
 */
AirType GetBestAirType(const CompanyID company)
{
	if (HasAirTypeAvail(company, AIRTYPE_ASPHALT)) return AIRTYPE_ASPHALT;
	return AIRTYPE_GRAVEL;
}

/**
 * Add the air types that are to be introduced at the given date.
 * @param current The currently available airtypes.
 * @param date    The date for the introduction comparisions.
 * @return The air types that should be available when date
 *         introduced air types are taken into account as well.
 */
AirTypes AddDateIntroducedAirTypes(AirTypes current, Date date)
{
	AirTypes ats = current;

	for (AirType at = AIRTYPE_BEGIN; at != AIRTYPE_END; at++) {
		const AirTypeInfo *ati = GetAirTypeInfo(at);
		/* Unused air type. */
		if (ati->label == 0) continue;

		/* Not date introduced. */
		if (!IsInsideMM(ati->introduction_date, 0, MAX_DAY)) continue;

		/* Not yet introduced at this date. */
		if (ati->introduction_date > date) continue;

		/* Have we introduced all required airtypes? */
		AirTypes required = ati->introduction_required_airtypes;
		if ((ats & required) != required) continue;

		ats |= ati->introduces_airtypes;
	}

	/* When we added airtypes we need to run this method again; the added
	 * airtypes might enable more air types to become introduced. */
	return ats == current ? ats : AddDateIntroducedAirTypes(ats, date);
}

/**
 * Get the air types the given company can build.
 * @param c the company to get the air types for.
 * @return the air types.
 */
AirTypes GetCompanyAirTypes(CompanyID company)
{
	AirTypes ats = AIRTYPES_NONE;

	Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, VEH_AIRCRAFT) {
		const EngineInfo *ei = &e->info;

		if (HasBit(ei->climates, _settings_game.game_creation.landscape) &&
				(HasBit(e->company_avail, company) || _date >= e->intro_date + DAYS_IN_YEAR)) {
			const AircraftVehicleInfo *avi = &e->u.air;

			if (avi->subtype <= AIR_AIRCRAFT) {
				assert(avi->airtype < AIRTYPE_END);
				ats |= GetAirTypeInfo(avi->airtype)->introduces_airtypes;
			}
		}
	}

	return AddDateIntroducedAirTypes(ats, _date);
}

/**
 * Get the air type for a given label.
 * @param label the airtype label.
 * @param allow_alternate_labels Search in the alternate label lists as well.
 * @return the airtype.
 */
AirType GetAirTypeByLabel(AirTypeLabel label, bool allow_alternate_labels)
{
	/* Loop through each air type until the label is found */
	for (AirType a = AIRTYPE_BEGIN; a != AIRTYPE_END; a++) {
		const AirTypeInfo *ati = GetAirTypeInfo(a);
		if (ati->label == label) return a;
	}

	if (allow_alternate_labels) {
		/* Test if any air type defines the label as an alternate. */
		for (AirType a = AIRTYPE_BEGIN; a != AIRTYPE_END; a++) {
			const AirTypeInfo *ati = GetAirTypeInfo(a);
			if (ati->alternate_labels.Contains(label)) return a;
		}
	}

	/* No matching label was found, so it is invalid */
	return INVALID_AIRTYPE;
}
