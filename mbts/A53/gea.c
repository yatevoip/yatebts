/*
 * gea.c
 *
 * Full reimplementation of GEA3
 *
 * Copyright (C) 2013 Max <Max.Suraev@fairwaves.ru>
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
#include <stdint.h>

#include "bits.h"
#include "gprs_cipher.h"
#include "kasumi.h"


int osmo_gea4(uint8_t *out, uint16_t len, uint8_t * kc, uint32_t iv, enum gprs_cipher_direction direction) {
    _kasumi_kgcore(0xFF, 0, iv, direction, kc, out, len * 8);

    return 0;
}

int osmo_gea3(uint8_t *out, uint16_t len, uint64_t kc, uint32_t iv, enum gprs_cipher_direction direction) {
    uint8_t ck[16];
    osmo_64pack2pbit(kc, ck);
    osmo_64pack2pbit(kc, ck + 8);

//    _kasumi_kgcore(0xFF, 0, iv, direction, ck, out, len * 8);

    return osmo_gea4(out, len, ck, iv, direction);
}
