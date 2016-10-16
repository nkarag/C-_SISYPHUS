#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <numeric>
#include <cmath>
//#include <strstream>
//#include <algorithm>

#include "Exceptions.h"

class LevelMember{
public:
        static const int PSEUDO_CODE = -1;
};

class Chunk {
public:
	/**
	 * this is the depth at the root chunk level
	 */
	static const unsigned short MIN_DEPTH = 0;
	
	/**
	 * Definition of NULL local depth.
	 */
	static const short NULL_DEPTH = -1;	
	
	/**
	 * Definition of minimum order code (origin) for a level..
	 */
	static const short MIN_ORDER_CODE = -1;	
};

struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const int NULL_RANGE = -1;
	string dimName;
	string lvlName;
	int leftEnd;
	int rightEnd;

	LevelRange(): dimName(""), lvlName(""), leftEnd(NULL_RANGE), rightEnd(NULL_RANGE){}
	LevelRange(const string& dn, const string& ln, int le, int re):
		dimName(dn),lvlName(ln),leftEnd(le),rightEnd(re){}
	/**
	 * assignment operator, because the compiler complains that it cant use
	 * the default operator=, due to the const member
	 */
	//LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	// LevelRange::LevelRange(const LevelRange& l);
};//end struct LevelRange

/**
 * This function class represents a method for resolving the storage of a large chunk.
 */
class EquiGrid_EquiChildren {
      public:
      	/**
      	 * Constructor
      	 *
      	 * @param	e	Maximum number of entries that fit in a bucket
      	 * @param 	d 	Number of dimensions of the cube, on which  the method will be applied
      	 * @param	rangeVect A vector containing the order-code ranges for each dimension of the
      	 *				input large data chunk (the one that the method will be applied on)
      	 */
      	/*EquiGrid_EquiChildren(int e, int d, vector<LevelRange>& rangeVect): maxDirEntries(e), noDims(d) {
       		
      	}//end EquiGrid_EquiChildren()*/
      	
        EquiGrid_EquiChildren(int nd): noDims(nd) {
                noMembersNewLevel = 7;
                maxNoChildrenPerDim.push_back(3);
                maxNoChildrenPerDim.push_back(0);                                       		
      	}//end EquiGrid_EquiChildren()      	
       		
      	/**
      	 * Destructor
      	 */
      	~EquiGrid_EquiChildren() {}

      	/**
      	 * Create the new hierarchies per dimension due to the artificial chunking
      	 * and store them in a vector of maps (one map per dimension)
      	 *
      	 * @param	oldRangeVect 	A vector containing the order-code ranges for each dimension of the
      	 *					input large data chunk (the one that the method will be applied on) - input parameter
         * @param	newHierarchyVect	output parameter storing the hierarchies for all dimensions
        */       		
	void createNewHierarchies(
				const vector<LevelRange>& oldRangeVect, //input
               			vector<map<int, LevelRange> >& newHierarchyVect //output
       	        		 );
      	       		
      private:
       /**
        * Maximum number of entries that fit in a bucket
        */
       //unsigned int maxDirEntries;
		
        /**
         * Number of dimensions of the cube, on which  the method will be applied
         */
       unsigned int noDims;	
		  	
       /**
        * Number of artificial partitions (i.e., number of members of new level) along each dimension.This will be the
        * same for all dimensions (equi-grid)
        */
       unsigned int noMembersNewLevel;
		
       /**
      	 * The maximum number of children under each new member, equal for all members of a dimension (equi-children).w
      	 * One entry per dimension
      	 */
      	vector<unsigned int> maxNoChildrenPerDim;	
       		
      	/**
      	 * This number shows how many of the newly inserted members are pseudo levels
      	 */
      	//unsigned int noPseudoLevels;
       		
       			
};//end class EquiGrid_EquiChildren       				

int main(){

        int noDims = 2;

        //create original range vector
        vector<LevelRange> originalRng(noDims);
        originalRng[0].leftEnd = 9;
        originalRng[0].rightEnd = 16;

        originalRng[1].leftEnd = 29;
        originalRng[1].rightEnd = 34;

        //init EquiGrid_EquiChildren data members
        EquiGrid_EquiChildren method(noDims);

        //create new range vector
      /*  vector<LevelRange> newRng(noDims);
        newRng[0].leftEnd = 0;
        newRng[0].rightEnd = 2;

        newRng[1].leftEnd = 0;
        newRng[1].rightEnd = 2;
       */
       	//now, create and store the new hierarchies per dimension
       	//use a vector of maps (one map per dimension)
       	vector<map<int, LevelRange> > newHierarchyVect(noDims);
       	try{
       		method.createNewHierarchies(originalRng,       	
       		                                newHierarchyVect);
       	}
      	catch(GeneralError& error) {
      		GeneralError e("main ==> ");
      		error += e;
      		cerr << error;
      	}			

      	//print new hierarchy
        for(int dimi = 0; dimi < noDims; dimi++) {
                cout << "____ Dim: " << dimi << " ____" << endl;     	
                for(map<int, LevelRange>::const_iterator mapiter = newHierarchyVect[dimi].begin(); mapiter != newHierarchyVect[dimi].end(); mapiter++){
                        cout << mapiter->first << " ==> " << "[" << mapiter->second.leftEnd << ", " << mapiter->second.rightEnd << "]" << endl;
                }//for
                cout << endl;      	
        }//for
}//main

void EquiGrid_EquiChildren::createNewHierarchies(
			const vector<LevelRange>& oldRangeVect, //input
			vector<map<int, LevelRange> >& newHierarchyVect //output
			)
//precondition:
//	The EquiGrid_EquiChildren() has calculated the number of artificial partitions (i.e., number of members of new level - equal for all dims))
//	along each dimension, stored in the EquiGrid_EquiChildren::noMembersNewLevel data member and the maximum number of children under each new member,
//      equal for all members of a dimension (equi-children), stored in the EquiGrid_EquiChildren::maxNoChildrenPerDim data member.
//      (NOTE: The latter is only needed for discriminating between pseudo and non-pseudo levels.)
//	The oldRangeVect contains the ranges of the original large (data) chunk. The output parameter
//      newHierarchyVect will store the result hierarchy. When the routine is called it is assumed that the newHierarchyVect has been
//      initialized like this: vector<map<int, LevelRange> > newHierarchyVect(noDims), where nodims is the number of dimensions of the cube.
//processing:
//	compute the parent child relationships between the newly inserted members and the grain-level members for each dimension (oldRangeVect);
//      then,store them in a vector of maps (each map corresponds to dimension). NOTE: if for a dimension the new level inserted is a
//	pseudo level, then in the map we store the original range (i.e., no chunking takes place along this dimension) associated with
//	the PSEUDO_CODE constant. The method used for uniformly distibuting the children among the parents is: iterate parents in a
//      cyclic way and each time assign a child to a parent, until no more children are left.
//postcondition:
//	the vector of maps contains for each dimension, the association of each parent order code to a range of grain level members
{
       	//for each dimension
       	for(int dimi = 0; dimi < noDims; dimi++) {
       		//if this is not a pseudo level in the new chunk
       		if(maxNoChildrenPerDim[dimi] != 0){
       			//init vector which counts the number of children of each parent along this dimension
       			vector<unsigned int> childrenCounterPerParent(noMembersNewLevel, 0);
       			
       			//the total number of children is
       			int totalChildren = oldRangeVect[dimi].rightEnd - oldRangeVect[dimi].leftEnd + 1;
       		
       		    	//loop
       		    	int childrenLeft = totalChildren;
       		    	int parent_index = 0;
       		    	do{
       		    		//assign one child to each parent
       		    		childrenCounterPerParent[parent_index]++;
       		    		//one more child has been assigned
       		    		childrenLeft--;
       		    		//cycle to the next parent
       		    		parent_index = (parent_index + 1) % noMembersNewLevel;
       		    	}while(childrenLeft); //while there are still children to assign
       		    	
       		    	//assert that the sum of all counters equals the total number of children
       		    	if(accumulate(childrenCounterPerParent.begin(), childrenCounterPerParent.end(), 0) != totalChildren)
       		    		throw GeneralError("AccessManager::EquiGrid_EquiChildren::createNewHierarchies ==>ASSERTION: wrong total of children");
       		    	
       		    	//start the actual assignement of ranges to parents
       		    	//init left end: 	left = 1st order code at grain level for this dimension
       		    	int left = oldRangeVect[dimi].leftEnd;
       		        //for each parent

               		for(int parent_code = Chunk::MIN_ORDER_CODE;
               		    parent_code <= int(noMembersNewLevel -1 + Chunk::MIN_ORDER_CODE); //***NOTE*** casting to int necessary if
               		                                                                // parent_code is initialized with a negative value!!!
               		    parent_code++) {

               			// create appropriate grain level range corresponding to its children
               			LevelRange grainrange;       			
                		// left end assignment
                		grainrange.leftEnd = left;
                		// right-end assignment = left + number of children assigned to this parent - 1;
                		grainrange.rightEnd = grainrange.leftEnd + childrenCounterPerParent[parent_code - Chunk::MIN_ORDER_CODE] - 1;
                		//Associate range to parent code
                		(newHierarchyVect[dimi])[parent_code] = grainrange;
                		//move left to the next free child
                		left = grainrange.rightEnd + 1;                		
			}//end for       		        	
        	}//end if
        	else { //this dimension corresponds to a pseudo level in the new chunk
       			// create appropriate grain level range covering ALL children (since no chunking takes place along this dim)
       			LevelRange grainrange;       			
        		// left end assignment
        		grainrange.leftEnd = oldRangeVect[dimi].leftEnd;
        		// right end assignment
        		grainrange.rightEnd = oldRangeVect[dimi].rightEnd;
        		//Associate range to parent code (pseudo code in this case)
        		(newHierarchyVect[dimi])[LevelMember::PSEUDO_CODE] = grainrange;        	
        	}//end else	
	} //end for
}// end AccessManager::EquiGrid_EquiChildren::createNewHierarchies

