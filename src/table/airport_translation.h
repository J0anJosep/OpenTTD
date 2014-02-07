/* $Id: airport_translation.h $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_translation.h Tables with translation values for airports. */

#ifndef AIRPORT_TRANSLATION_H
#define AIRPORT_TRANSLATION_H

static const bool _translation_airport_hangars[] = {
	1,
	1,
	0,
	1,
	1,
	1,
	1,
	1,
	1,
	0,
};

assert_compile(NEW_AIRPORT_OFFSET == lengthof(_translation_airport_hangars));

#endif /* AIRPORT_TRANSLATION_H */
