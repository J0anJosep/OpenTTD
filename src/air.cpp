/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file air.cpp Implementation of air specific functions. */

#include "stdafx.h"
#include "station_map.h"
#include "company_func.h"
#include "company_base.h"
#include "engine_base.h"
#include "aircraft.h"

#include "table/airtypes.h"

/**
 * Finds out if a company has a certain buildable airtype available.
 * @param company the company in question
 * @param airtype requested AirType
 * @return true if company has requested AirType available
 */
bool HasAirTypeAvail(const CompanyID company, const AirType airtype)
{
	return !HasBit(_airtypes_hidden_mask, airtype) && HasBit(Company::Get(company)->avail_airtypes, airtype);
}

/**
 * Test if any buildable airtype is available for a company.
 * @param company the company in question
 * @return true if company has any AirTypes available
 */
bool HasAnyAirTypesAvail(const CompanyID company)
{
	return (Company::Get(company)->avail_airtypes & ~_airtypes_hidden_mask) != 0;
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
 * Add the air types that are to be introduced at the given date.
 * @param current The currently available airtypes.
 * @param date    The date for the introduction comparisons.
 * @return The air types that should be available when date
 *         introduced air types are taken into account as well.
 */
AirTypes AddDateIntroducedAirTypes(AirTypes current, TimerGameCalendar::Date date)
{
	AirTypes rts = current;

	for (AirType rt = AIRTYPE_BEGIN; rt != AIRTYPE_END; rt++) {
		const AirTypeInfo *rti = GetAirTypeInfo(rt);
		/* Unused air type. */
		if (rti->label == 0) continue;

		/* Not date introduced. */
		if (!IsInsideMM(rti->introduction_date, 0, CalendarTime::MAX_DATE.base())) continue;

		/* Not yet introduced at this date. */
		if (rti->introduction_date > date) continue;

		/* Have we introduced all required airtypes? */
		AirTypes required = rti->introduction_required_airtypes;
		if ((rts & required) != required) continue;

		rts |= rti->introduces_airtypes;
	}

	/* When we added airtypes we need to run this method again; the added
	 * airtypes might enable more air types to become introduced. */
	return rts == current ? rts : AddDateIntroducedAirTypes(rts, date);
}

/**
 * Get the air types the given company can build.
 * @param company the company to get the air types for.
 * @param introduces If true, include air types introduced by other air types
 * @return the air types.
 */
AirTypes GetCompanyAirTypes(CompanyID company, bool introduces)
{
	AirTypes rts = AIRTYPES_NONE;

	for (const Engine *e : Engine::IterateType(VEH_AIRCRAFT)) {
		const EngineInfo *ei = &e->info;

		if (HasBit(ei->climates, _settings_game.game_creation.landscape) &&
			(HasBit(e->company_avail, company) || TimerGameCalendar::date >= e->intro_date + CalendarTime::DAYS_IN_YEAR)) {
			const AircraftVehicleInfo *rvi = &e->u.air;

			assert(rvi->airtype < AIRTYPE_END);
			if (introduces) {
				rts |= GetAirTypeInfo(rvi->airtype)->introduces_airtypes;
			} else {
				SetBit(rts, rvi->airtype);
			}
		}
	}

	if (introduces) return AddDateIntroducedAirTypes(rts, TimerGameCalendar::date);
	return rts;
}

/**
 * Get list of air types, regardless of company availability.
 * @param introduces If true, include air types introduced by other air types
 * @return the air types.
 */
AirTypes GetAirTypes(bool introduces)
{
	AirTypes rts = AIRTYPES_NONE;

	for (const Engine *e : Engine::IterateType(VEH_AIRCRAFT)) {
		const EngineInfo *ei = &e->info;
		if (!HasBit(ei->climates, _settings_game.game_creation.landscape)) continue;

		const AircraftVehicleInfo *rvi = &e->u.air;
		assert(rvi->airtype < AIRTYPE_END);
		if (introduces) {
			rts |= GetAirTypeInfo(rvi->airtype)->introduces_airtypes;
		} else {
			SetBit(rts, rvi->airtype);
		}
	}

	if (introduces) return AddDateIntroducedAirTypes(rts, CalendarTime::MAX_DATE);
	return rts;
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
	for (AirType r = AIRTYPE_BEGIN; r != AIRTYPE_END; r++) {
		const AirTypeInfo *rti = GetAirTypeInfo(r);
		if (rti->label == label) return r;
	}

	if (allow_alternate_labels) {
		/* Test if any air type defines the label as an alternate. */
		for (AirType r = AIRTYPE_BEGIN; r != AIRTYPE_END; r++) {
			const AirTypeInfo *rti = GetAirTypeInfo(r);
			if (std::find(rti->alternate_labels.begin(), rti->alternate_labels.end(), label) != rti->alternate_labels.end()) return r;
		}
	}

	/* No matching label was found, so it is invalid */
	return INVALID_AIRTYPE;
}

