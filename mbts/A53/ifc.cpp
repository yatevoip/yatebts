#include "a53.h"
extern "C" {
#include "a5.h"
};
#include <stdio.h>

void A53_GSM( u8 *key, int klen, int count, u8 *block1, u8 *block2 )
{
	ubit_t dl[120];
	ubit_t ul[120];
	static bool first = true;
	if (first) {
		printf("public A5/3\n");
		first = false;
	}
	osmo_a5_3(key, (uint32_t)count, dl, ul);
	for (int i = 0; i < 15; i++) {
		unsigned char acc1 = 0;
		unsigned char acc2 = 0;
		for (int j = 0; j < 8; j++) {
			acc1 |= (dl[i*8+j] & 1) << (7-j);
			acc2 |= (ul[i*8+j] & 1) << (7-j);
		}
		block1[i] = acc1;
		block2[i] = acc2;
	}
	block1[14] &= 0xC0;
	block2[14] &= 0xC0;
}
