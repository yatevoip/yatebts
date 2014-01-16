/*
 * a5.h
 *
 * Copyright (C) 2011  Sylvain Munaut <tnt@246tNt.com>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __OSMO_A5_H__
#define __OSMO_A5_H__

#include <stdint.h>
#include <stdbool.h>

#include "bits.h"

/*! \defgroup a5 GSM A5 ciphering algorithm
 *  @{
 */

/*! \file gsm/a5.h
 *  \brief Osmocom GSM A5 ciphering algorithm header
 */

/*! \brief Converts a frame number into the 22 bit number used in A5/x
 *  \param[in] fn The true framenumber
 *  \return 22 bit word
 */
static inline uint32_t
osmo_a5_fn_count(uint32_t fn)
{
	int t1 = fn / (26 * 51);
	int t2 = fn % 26;
	int t3 = fn % 51;
	return (t1 << 11) | (t3 << 5) | t2;
}

	/* Notes:
	 *  - key must be 8 bytes long (or NULL for A5/0)
	 *  - the dl and ul pointer must be either NULL or 114 bits long
	 *  - fn is the _real_ GSM frame number.
	 *    (converted internally to fn_count)
	 */
void osmo_a5(int n, const uint8_t *key, uint32_t fn, ubit_t *dl, ubit_t *ul);
void osmo_a5_1(const uint8_t *key, uint32_t fn, ubit_t *dl, ubit_t *ul);
void osmo_a5_2(const uint8_t *key, uint32_t fn, ubit_t *dl, ubit_t *ul);
void osmo_a5_3(const uint8_t *key, uint32_t fn, ubit_t *dl, ubit_t *ul);
void osmo_a5_4(const uint8_t *ck, uint32_t fn, ubit_t *dl, ubit_t *ul);

/*! @} */

#endif /* __OSMO_A5_H__ */
