#ifndef OSMOCORE_UTIL_H
#define OSMOCORE_UTIL_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include <stdint.h>

struct value_string {
	unsigned int value;
	const char *str;
};

const char *get_value_string(const struct value_string *vs, uint32_t val);
int get_string_value(const struct value_string *vs, const char *str);

char *osmo_ubit_dump(const uint8_t *bits, unsigned int len);
char *osmo_hexdump_nospc(const unsigned char *buf, int len);

int osmo_hexparse(const char *str, uint8_t *b, int max_len);


#endif
