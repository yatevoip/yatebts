/*
 * GEA3 header
 *
 * See gea.c for details
 */

#ifndef __GEA_H__
#define __GEA_H__

#include <stdint.h>

#include "gprs_cipher.h"

/*
 * Performs the GEA3 algorithm (used in GPRS)
 * out   : uint8_t []
 * len   : uint16_t
 * kc    : uint64_t
 * iv    : uint32_t
 * direct: 0 or 1
 */

int osmo_gea3(uint8_t *out, uint16_t len, uint64_t kc, uint32_t iv, enum gprs_cipher_direction direct);

int osmo_gea4(uint8_t *out, uint16_t len, uint8_t * kc, uint32_t iv, enum gprs_cipher_direction direct);

#endif /* __GEA_H__ */

