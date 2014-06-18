/**@ The global table of neighboring BTS units. */
/*
 * Copright 2011 Range Networks, Inc.
 * All rights reserved.
*/


#ifndef NEIGHBORTABLE_H
#define NEIGHBORTABLE_H

#include <Threads.h>
#include <string>
#include <vector>


namespace Peering {

class NeighborTable {

	struct Neighbor {
		Neighbor(int c0, int bsic, const char* id) : C0(c0), BSIC(bsic), CELL_ID(id) { }
		unsigned C0;
		unsigned BSIC;
		std::string CELL_ID;
	};

private:
	std::vector<Neighbor> mNeighbors;
	std::vector<unsigned> mARFCNList;	///< current ARFCN list
	int mBCCSet;				///< set of current BCCs
	mutable Mutex mLock;

public:
	NeighborTable() : mBCCSet(0) {}

	/** Populate the list from a descriptor string */
	bool setNeighbors(const char* list);

	/** Returns the cell ID for a measurement. */
	std::string getCellID(unsigned BCCH_FREQ_NCELL, unsigned BSIC);

	/** Return the ARFCN given its position in the BCCH channel list (GSM 04.08 10.5.2.20). */
	int getARFCN(unsigned BCCH_FREQ_NCELL);

	/**
		Get the neighbor cell ARFCNs as a vector.
		mARFCNList is updated by every call to add().
		This returns a copy to be thread-safe.
	*/
	std::vector<unsigned> ARFCNList() const { ScopedLock lock(mLock); return mARFCNList; }

	/** Get the neighbor cell BCC set as a bitmask. */
	int BCCSet() const { return mBCCSet; }
};

} //namespace

extern Peering::NeighborTable gNeighborTable;

#endif

