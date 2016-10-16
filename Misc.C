/***************************************************************************
                          Misc.C  -  Definitions of the routines in Misc.h
                             -------------------
    begin                : Fri Mar 22 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include "Misc.h"

#include <new>

export template<class T> void trimSTLvectorsCapacity(vector<T>& v, bool clearEverything)
{
	if(clearEverything)
		vector<T>().swap(v);
	else
		vector<T>(v).swap(v);			
}//end of trimSTLvectorsCapacity()

export template<class T> memSize_t persistentReserveForSTLVector(
			vector<T>& v, memSize_t szRequest, float reduction_factor, memSize_t minSize) throw(std::bad_alloc)
{
	memSize_t currRequest = szRequest;
	//try to reserve as much as you have been requested	
	try{
		v.reserve(currRequest);	
	}
	catch(std::bad_alloc&){ //on failure
		//derive new size request
		currRequest = static_cast<memSize_t>(reduction_factor * currRequest);
		
		//loop: while the size request is not less than the minimum size request
		while(currRequest > minSize) {
			//try again to reserve
                 	try{
                 		v.reserve(currRequest);	
                 	}
                 	catch(std::bad_alloc&){ //on failure
				//derive new size request
				currRequest = static_cast<memSize_t>(reduction_factor * currRequest);
				//continue to loop
				continue;
			}//catch
			//on success return the reserved amount		
			return currRequest;
		} //end while loop
		//no reservation made - throw a bad_alloc
		throw;
		//throw std::bad_alloc;			
	}//catch
	
	//Great! The whole request was fulfilled!
	return szRequest;		
}//persistentReserveForSTLVector()
