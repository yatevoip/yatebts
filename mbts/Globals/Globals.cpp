/**@file Global system parameters. */
/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
* Copyright 2010 Kestrel Signal Processing, Inc.
* Copyright 2011, 2012 Range Networks, Inc.
* Copyright (C) 2013-2014 Null Team Impex SRL
* Copyright (C) 2014 Legba, Inc
*
* This software is distributed under multiple licenses;
* see the COPYING file in the main directory for licensing
* information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#include <config.h>
#include <Globals.h>
#include <CLI.h>
//#include <TMSITable.h>
//#include <URLEncode.h>
#include <Logger.h>

#define PROD_CAT "P"

#define FEATURES "+GPRS "

const char *gVersionString = "release " PACKAGE_VERSION " built " __DATE__ " rev" PACKAGE_REVISION " ";

const char* gOpenBTSWelcome =
	//23456789123456789223456789323456789423456789523456789623456789723456789
	"Yate-BTS MBTS Component\n"
	"Copyright 2008, 2009, 2010 Free Software Foundation, Inc.\n"
	"Copyright 2010 Kestrel Signal Processing, Inc.\n"
	"Copyright 2011, 2012, 2013 Range Networks, Inc.\n"
	"Copyright 2013, 2014 Null Team Impex SRL\n"
	"Copyright 2014 Legba, Inc.\n"
	"\"OpenBTS\" is a registered trademark of Range Networks, Inc.\n"
	"\nContributors:\n"
	"  SC Null Team Impex SRL:\n"
	"    Paul Chitescu\n"
	"  Legba, Inc.\n"
	"    David Burgess\n"
	"  Range Networks, Inc.:\n"
	"    David Burgess, Harvind Samra, Donald Kirker, Doug Brown,\n"
	"    Pat Thompson, Kurtis Heimerl\n"
	"  Kestrel Signal Processing, Inc.:\n"
	"    David Burgess, Harvind Samra, Raffi Sevlian, Roshan Baliga\n"
	"  GNU Radio:\n"
	"    Johnathan Corgan\n"
	"  Others:\n"
	"    Anne Kwong, Jacob Appelbaum, Joshua Lackey, Alon Levy\n"
	"    Alexander Chemeris, Alberto Escudero-Pascual\n"
	"Incorporated L/GPL libraries and components:\n"
	"  libusb, LGPL 2.1, various copyright holders, www.libusb.org\n"
	"Incorporated BSD/MIT-style libraries and components:\n"
	"  A5/1 Pedagogical Implementation, Simplified BSD License,\n"
	"    Copyright 1998-1999 Marc Briceno, Ian Goldberg, and David Wagner\n"
	"Incorporated public domain libraries and components:\n"
	"  sqlite3, released to public domain 15 Sept 2001, www.sqlite.org\n"
	"\n"
	"\nThis program comes with ABSOLUTELY NO WARRANTY.\n"
	"\nUse of this software may be subject to other legal restrictions,\n"
	"including patent licensing and radio spectrum licensing.\n"
	"All users of this software are expected to comply with applicable\n"
	"regulations and laws.  See the LEGAL file in the source code for\n"
	"more information.\n"
	"\n"
	"Release " PACKAGE_VERSION " formal build date " __DATE__ " rev" PACKAGE_REVISION "\n"
;


CommandLine::Parser gParser;

