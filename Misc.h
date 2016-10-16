/***************************************************************************
                          Misc.h  -  Misc routines with various functionalities,
                          e.g., STL vector manipulation
                             -------------------
    begin                : Fri Mar 22 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#ifndef MISC_H
#define MISC_H

#include "definitions.h"
#include <vector>

/**
 * Reduces the excess capacity of an STL vector.
 *
 * @param v	input vector
 * @param clearEverything	if this flag is set then the input vector is cleared from all its contents
 *				and its capacity is minimized
 * @see	Effective STL, Item 17
 */
template<class T> void trimSTLvectorsCapacity(vector<T>& v, bool clearEverything = false);

/**
 * This routine persistently tries to reserve memory for an STL vector.
 * It initially tries to reserve szRequest entries (i.e., change the vector's
 * capacity to szRequest). If this effort fails then it repeatedly tries to
 * reserve a percent of the immediate previously requested size, which is determined by the
 * reduction_factor parameter. If the requested size drops below "minSize" then
 * it throws an std::bad_alloc exception. It returns the number of entries reserved.
 */
template<class T> memSize_t persistentReserveForSTLVector(vector<T>& v, memSize_t szRequest, float reduction_factor, memSize_t minSize) throw(std::bad_alloc);

#endif // MISC_H

