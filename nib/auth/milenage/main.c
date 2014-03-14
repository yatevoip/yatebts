/**
 * main.c
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * MILENAGE authentication helper
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2014 Null Team Impex SRL
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "milenage.h"
#include "rijndael.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int hextoint(char x)
{
	x = toupper(x);
	if (x >= 'A' && x <= 'F')
		return x-'A'+10;
	else if (x >= '0' && x <= '9')
		return x-'0';
	fprintf(stderr, "bad input.\n");
	exit(1);
}

int main(int argc, char **argv)
{
	u8 rand[16], key[16], op[16], sqn[6], amf[2];
	u8 xres[8], ck[16], ik[16], autn[16], auts[14];
	u8 mac_a[8], ak[6];
	int i;
	int ms = 0;
	int sync = 0;

	if (argc != 5 || strlen(argv[1]) != 34 || strlen(argv[2]) != 34
			|| ((strlen(argv[3]) != 14) &&
			    ((sync = 1)) && (strlen(argv[3]) != 30) &&
			    ((ms = 1)) && (strlen(argv[3]) != 34))
			|| strlen(argv[4]) != 34
			|| strncmp(argv[1], "0x", 2) != 0 || strncmp(argv[2], "0x", 2) != 0
			|| strncmp(argv[3], "0x", 2) != 0 || strncmp(argv[4], "0x", 2) != 0) {
		fprintf(stderr, "Usage: %s 0x<key> 0x<op> 0x<sqn/auts/autn> 0x<rand>\n", argv[0]);
		exit(1);
	}

	for (i=0; i<16; i++)
		key[i] = (hextoint(argv[1][2*i+2])<<4)
			| hextoint(argv[1][2*i+3]);
	for (i=0; i<16; i++)
		op[i] = (hextoint(argv[2][2*i+2])<<4)
			|hextoint(argv[2][2*i+3]);
	if (ms) {
		for (i=0; i<16; i++)
			autn[i] = (hextoint(argv[3][2*i+2])<<4)
				| hextoint(argv[3][2*i+3]);
	} else if (sync) {
		for (i=0; i<14; i++)
			auts[i] = (hextoint(argv[3][2*i+2])<<4)
				| hextoint(argv[3][2*i+3]);
	} else {
		for (i=0; i<6; i++)
			sqn[i] = (hextoint(argv[3][2*i+2])<<4)
				| hextoint(argv[3][2*i+3]);
	}
	for (i=0; i<16; i++)
		rand[i] = (hextoint(argv[4][2*i+2])<<4)
			 | hextoint(argv[4][2*i+3]);
	amf[1] = amf[0] = 0;

	if (ms) {
		/* compute xres, ck, ik, ak */
		f2345(key, rand, xres, ck, ik, ak, op);
		/* unobscure sqn of the AuC */
		for (i=0; i<6; i++)
			sqn[i] = ak[i] ^ autn[i];
		/* get amf */
		amf[0] = autn[6];
		amf[1] = autn[7];
		/* compute mac_a */
		f1(key, rand, sqn, amf, mac_a, op);
		for (i=0; i<8; i++)
			if (mac_a[i] != autn[i+8])
				return 0;
		/* xres, ck, ik */
		for (i=0; i<8; i++)
			printf("%02X", xres[i]);
		printf(" ");
		for (i=0; i<16; i++)
			printf("%02X", ck[i]);
		printf(" ");
		for (i=0; i<16; i++)
			printf("%02X", ik[i]);
		printf("\n");
		return 0;
	}

	if (sync) {
		/* compute ak */
		f5star(key, rand, ak, op);
		/* unobscure sqn of the MS */
		for (i=0; i<6; i++)
			sqn[i] = ak[i] ^ auts[i];
		/* compute mac_s using the recovered sqn */
		f1star(key, rand, sqn, amf, mac_a, op);
		for (i=0; i<8; i++)
			if (mac_a[i] != auts[i+6])
				return 0;
		/* mac_s matched, return deduced sqn */
		for (i=0; i<6; i++)
			printf("%02X", sqn[i]);
		printf("\n");
		return 0;
	}

	/* compute mac_a */
	f1(key, rand, sqn, amf, mac_a, op);
	/* compute xres, ck, ik, ak */
	f2345(key, rand, xres, ck, ik, ak, op);
	for (i=0; i<6; i++)
		autn[i] = sqn[i] ^ ak[i];
	autn[6] = amf[0];
	autn[7] = amf[1];
	for (i=0; i<8; i++)
		autn[i+8] = mac_a[i];
	/* xres, ck, ik, autn */
	for (i=0; i<8; i++)
		printf("%02X", xres[i]);
	printf(" ");
	for (i=0; i<16; i++)
		printf("%02X", ck[i]);
	printf(" ");
	for (i=0; i<16; i++)
		printf("%02X", ik[i]);
	printf(" ");
	for (i=0; i<16; i++)
		printf("%02X", autn[i]);
	printf("\n");
	return 0;
}

/* vi: set ts=8 sw=8 sts=8 noet: */
