/*
 * Copright 2011 Range Networks, Inc.
 * All rights reserved.
 * Copyright (C) 2013-2014 Null Team Impex SRL
 * Copyright (C) 2014 Legba, Inc
*/

#include "NeighborTable.h"
#include <GSMConfig.h>

#include <Logger.h>
#include <Globals.h>

#include <stdio.h>

using namespace Peering;
using namespace std;


bool NeighborTable::setNeighbors(const char* list)
{
    // Syntax is: BAND ARFCN1:BSIC1:CELLID1 ARFCN2:BSIC2:CELLID2 ...
    int band,len;
    if (sscanf(list,"%d %n",&band,&len) != 1)
	return false;
    std::vector<Neighbor> cells;
    std::vector<unsigned> arfcns;
    int bccs = 0;
    int lastC0 = -1;
    for (list += len; *list; list += len) {
	int c0,bsic;
	char id[16];
	if (sscanf(list,"%d:%d:%15s %n",&c0,&bsic,id,&len) != 3 || c0 < 0 || bsic < 0 || bsic > 63)
	    return false;
	cells.push_back(Neighbor(c0,bsic,id));
	// Received list is already sorted by ARFCN
	if (c0 != lastC0) {
	    arfcns.push_back(c0);
	    lastC0 = c0;
	}
	bccs |= (1 << (bsic & 0x07));
    }
    // Radio band is ignored now, we should receive only neighbors for our own band
    LOG(NOTICE) << "Neighbor cells: " << cells.size() << ", ARFCNs: " << arfcns.size() << ", BCCs: 0x" << hex << bccs;
    mLock.lock();
    mNeighbors = cells;
    mARFCNList = arfcns;
    mBCCSet = bccs;
    mLock.unlock();
    gBTS.regenerateBeacon();
    return true;
}

std::string NeighborTable::getCellID(unsigned BCCH_FREQ_NCELL, unsigned BSIC)
{
	// There is a potential race condition here where mARFCNList could have changed
	// between the sending of SI5 and the receipt of the corresponding measurement report.
	// That will not be a serious problem as long as BSICs are unique,
	// which they should be.  The method will just return NULL.

	LOG(DEBUG) << "BCCH_FREQ_NCELL=" << BCCH_FREQ_NCELL << " BSIC=" << BSIC;
	ScopedLock lock(mLock);

	int c0 = getARFCN(BCCH_FREQ_NCELL);
	if (c0 < 0) return "";
	for (std::vector<Neighbor>::iterator it = mNeighbors.begin() ; it != mNeighbors.end(); ++it)
		if ((unsigned)c0 == it->C0 && BSIC == it->BSIC) return it->CELL_ID;
	return "";
}

/* Return the ARFCN given its position in the BCCH channel list.
 * The way I read GSM 04.08 10.5.2.20, they take the ARFCNs, sort them
 * in ascending order, move the first to the last if it's 0,
 * then BCCH-FREQ-NCELL is a position in that list.
 */
int NeighborTable::getARFCN(unsigned BCCH_FREQ_NCELL)
{
	LOG(DEBUG) << "BCCH_FREQ_NCELL=" << BCCH_FREQ_NCELL;
	ScopedLock lock(mLock);
	if (BCCH_FREQ_NCELL >= mARFCNList.size()) {
		LOG(ALERT) << "BCCH-FREQ-NCELL not in BCCH channel list";
		return -1;
	}
	return mARFCNList[BCCH_FREQ_NCELL];
}

