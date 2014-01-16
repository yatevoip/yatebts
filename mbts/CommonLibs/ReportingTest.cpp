/*
* Copyright 2012 Range Networks, Inc.
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "stdlib.h"
#include <syslog.h>
#include <time.h>

#include "Reporting.h"


int main(int argc, char *argv[])
{

	srandom(time(NULL));

	ReportingTable rpt("./test.db",LOG_LOCAL7);

	rpt.create("count1");
	rpt.create("count2");
	rpt.create("max1");
	rpt.create("max2");

	rpt.incr("count1");
	rpt.incr("count2");
	rpt.incr("count1");

	rpt.clear("max1");
	rpt.max("max1",random());
	rpt.max("max2",random());

	rpt.create("indexed",5,10);
	for (int i=0; i<20; i++) {
		rpt.incr("indexed",random()%6 + 5);
	}

	rpt.create("indexedMax",5,10);
	for (int i=5; i<=10; i++) {
		rpt.max("indexedMax", i, random());
	}

}
