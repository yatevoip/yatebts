/*
 * KASUMI header
 *
 * See kasumi.c for details
 */

#ifndef __KASUMI_H__
#define __KASUMI_H__

#include <stdint.h>

/*
 * Single iteration of KASUMI cipher
*/
uint64_t _kasumi(uint64_t P, uint16_t *KLi1, uint16_t *KLi2, uint16_t *KOi1, uint16_t *KOi2, uint16_t *KOi3, uint16_t *KIi1, uint16_t *KIi2, uint16_t *KIi3);

/*
 * Implementation of the KGCORE algorithm (used by A5/3, A5/4, GEA3, GEA4 and ECSD)
 *
 * CA    : uint8_t
 * cb    : uint8_t
 * cc    : uint32_t
 * cd    : uint8_t
 * ck    : uint8_t [8]
 * co    : uint8_t [output, cl-dependent]
 * cl    : uint16_t
 */
void _kasumi_kgcore(uint8_t CA, uint8_t cb, uint32_t cc, uint8_t cd, const uint8_t *ck, uint8_t *co, uint16_t cl);

/*! \brief Expand key into set of subkeys
 *  \param[in] key (128 bits) as array of bytes
 *  \param[out] arrays of round-specific subkeys - see TS 135 202 for details
 */
void _kasumi_key_expand(const uint8_t *key, uint16_t *KLi1, uint16_t *KLi2, uint16_t *KOi1, uint16_t *KOi2, uint16_t *KOi3, uint16_t *KIi1, uint16_t *KIi2, uint16_t *KIi3);

#endif /* __KASUMI_H__ */
