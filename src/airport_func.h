/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_func.h Functions related to airports. */

#ifndef AIRPORT_FUNC_H
#define AIRPORT_FUNC_H

#include "command_type.h"
#include "company_base.h"

struct station;

uint8 CalculateNoiseLevel(const Station *st);


#endif /* AIRPORT_FUNC_H */
