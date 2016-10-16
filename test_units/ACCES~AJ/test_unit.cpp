#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <strstream>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <climits>

//#include "help.h"
#include "Exceptions.h"

const unsigned int PAGESIZE = 131072;

struct BucketID {
        int rid;
};

struct DiskChunkHeader {
	/**
	 * Define the type of an order-code
	 */
        typedef int ordercode_t;

};

struct DiskBucket {
	/**
	 * Define the type of a directory entry
	 */
        typedef unsigned int dirent_t;

        /**
         * Size of the body of the bucket.
         */
        //enum {bodysize = PAGESIZE-sizeof(DiskBucketHeader)-sizeof(dirent_t*)};
        static const unsigned int bodysize = PAGESIZE-sizeof(dirent_t*);

        /**
         * Minimum bucket occupancy threshold (in bytes)
         */
        static const unsigned int BCKT_THRESHOLD = int(bodysize*0.5);
	
	/**
	 * The body of the bucket. I.e. where the chunks and the
         * directory entries will be stored.
	 */
	char body[bodysize];
	
	/**
	 * The bucket directory (grows backwards!).
	 * (Initialize this directory pointer to point one beyond last byte of body).
	 * The chunk at slot x (where x = 0 for the 1st chunk) resides at the byte offset:
	 * offsetInBucket[-x-1]
	 */
	dirent_t* offsetInBucket;	
}; //end of DiskBucket

struct DiskDirChunk {
	/**
	 * Define the type of a directory chunk entry.
	 */
        struct Entry {
        	BucketID bucketid;
        	unsigned short chunk_slot;
        };
        typedef Entry DirEntry_t;
};

/**
 * This is a simple range structure over the order-codes of the members of a level
 * @author: Nikos Karayannidis
 */
struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const DiskChunkHeader::ordercode_t NULL_RANGE = -1;
	string dimName;
	string lvlName;
	DiskChunkHeader::ordercode_t leftEnd;
	DiskChunkHeader::ordercode_t rightEnd;

	LevelRange(): dimName(""), lvlName(""), leftEnd(NULL_RANGE), rightEnd(NULL_RANGE){}
	LevelRange(const string& dn, const string& ln, DiskChunkHeader::ordercode_t le, DiskChunkHeader::ordercode_t re):
		dimName(dn),lvlName(ln),leftEnd(le),rightEnd(re){}
	/**
	 * assignment operator, because the compiler complains that it cant use
	 * the default operator=, due to the const member
	 */
	LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 LevelRange::LevelRange(const LevelRange& l);
};//end struct LevelRange


class AccessManager {
public:
	/**
	 * AccessManager constructor
	 */
	AccessManager()	{}

	/**
	 * AccessManager destructor
	 */
	~AccessManager() {}
    	    	
	/**
	 * This function class represents a method for resolving the storage of a large chunk.
	 */
	class EquiGrid_EquiChildren {
       	public:
       		/**
       		 * Constructor of the Equi_grid-equi_children method of resolving the storage of a large data chunk.
       		 * This routine is responsible for initializing specific data members in order when operator() member function
       		 * is called will be able to apply the method. If it encounters a large data chunk consiting of only one cell(!!)
       		 * it throws a GeneralError exception.
       		 *
       		 * @param	e	Maximum number of entries that fit in a bucket
       		 * @param 	d 	Number of dimensions of the cube, on which  the method will be applied
       		 * @param	rangeVect A vector containing the order-code ranges for each dimension of the
       		 *				input large data chunk (the one that the method will be applied on)
       		 */
       		EquiGrid_EquiChildren(int e, int d, vector<LevelRange>& rangeVect);
       		
       		/**
       		 * Destructor
       		 */
       		~EquiGrid_EquiChildren() {}
       		
                /**
                 * Prints the data members of this*. Used for debugging
                 */
                 void print() {
                        cout << "maxDirEntries = " << maxDirEntries << endl;
                        cout << "noDims" << noDims << endl;
                	cout << "noMembersNewLevel = " << noMembersNewLevel << endl;
                  	cout << "maxNoChildrenPerDim: ";
                  	for(int i=0; i<noDims; i++){
                  	        cout << maxNoChildrenPerDim[i] <<", ";
                  	}//for
                  	cout << endl;
                  	cout << "noPseudoLevels = " << noPseudoLevels << endl;
                 }//print()

       	private:
		/**
		 * Maximum number of entries that fit in a bucket
		 */
		unsigned int maxDirEntries;
		
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
	       	 * The maximum number of children under each new member, equal for all members of a dimension (equi-children).
	       	 * One entry per dimension. A value of zero in a entry means that the corresponding dimension level is a pseudo level.
	       	 * Actually, we require that a valid maxNoChildrenPerDim entry, in order to have a no pseudo level is >= 2,
	       	 * otherwise we insert a 0, which means than no chunking takes place along the corresponding dimension.
	       	 */
       		vector<unsigned int> maxNoChildrenPerDim;	
       		
       		/**
       		 * This number shows how many of the newly inserted members are pseudo levels
       		 */
       		unsigned int noPseudoLevels;
       		
	};//end class EquiGrid_EquiChildren       				
	
	friend class EquiGrid_EquiChildren;


//______________________ PRIVATE DATA MEMBERS __________________________________________________________________________

    	
//______________________ PRIVATE METHOD DECLARATIONS ____________________________________________________________________    	
    	    	    	                									     	
};//class AccessManager

int main(){
        //create large data chunk CostNode
	// maxdepth numfacts init
        unsigned int numDims = 2;

        // create order code ranges
       	// insert order-code ranges per dimension level
       	vector<LevelRange> vectRange(numDims);
        vectRange[0] = LevelRange(string("Location"),string("country"),9,16);
        vectRange[1] = LevelRange(string("Product"),string("type"),29,34);

        //create EquiGrid_EquiChildren  instance
	// find the maximum number of directory entries (E) that fit in a bucket
	// Since this bucket will store only a single DirChunk,it will occupy one entry in the bucket internal directory
	// (DiskBucket::direntry_t)
	unsigned int maxDirEntries = ( DiskBucket::bodysize - sizeof(DiskBucket::dirent_t) )/sizeof(DiskDirChunk::DirEntry_t);
	cout << "maxDirEntries = " << maxDirEntries << ". Accept? (y/n): "<<endl;
	string ans;
	cin >> ans;
	if(ans[0] != 'y') {
                cout << "Give your maxDirEntries: "<<endl;
                cin >> maxDirEntries;
	}//end if
		
   	//call equi-grid-equi-children method
       	AccessManager::EquiGrid_EquiChildren method(maxDirEntries, numDims, vectRange);

       	method.print();
}//end main


AccessManager::EquiGrid_EquiChildren::EquiGrid_EquiChildren(int e, int d, vector<LevelRange>& rangeVect): maxDirEntries(e), noDims(d)
//precondition:
//      It is assumed that this routine is called on behalf of a *large data chunk*. Therefore, for example
//      it does not allow pseudo levels at all for input chunk (since it is a data chunk).
//processing:
//      calculates the number of members (equal for all dimensions) of the newly inserted levels. Decides
//      which of these new levels will be pseudo levels. Also calculates the maximum number of children
//      (equal for all members of the same dimension) of a member of a new level.
//postcondition:
//      All data members have valid values.
{
       		
	//find the number of artificial partitions (i.e., No of members of new level) along each dimension (m = floor(E**1/N))
	// this will be the same for all dimensions (equi-grid)       		
       	noMembersNewLevel = int( ::floor( ::pow(double(maxDirEntries),1.0/double(noDims)) ) );
       			
       	//find the maximum number of children under each new member (c = N/m), equal for all members of a dimension (equi-children).
       	// One entry per dimension
       	maxNoChildrenPerDim.reserve(noDims);//the order of the dimensions in this vector are the same with the interleaving order
       	noPseudoLevels = 0; //init pseudo level counter
       	vector<unsigned int> noMembersGrainVect(noDims); //init vector to hold no of grain members per dim                       	
       	for(int i=0; i<noDims; i++){
       		//if this not a pseudo level
       		if(rangeVect[i].lvlName.find("Pseudo-level") == string::npos){
               		// calc no of members on each dimension at the grain level of the original chunk                       		
        		noMembersGrainVect[i] = rangeVect[i].rightEnd - rangeVect[i].leftEnd + 1;
                        		
        		// calc the max number of children at the grain level that a parent member can have
        		// if the new members are fewer than the existing members at the grain level
        		if(noMembersGrainVect[i] >= noMembersNewLevel){
        			//then the hierarchy is meaningful
                		maxNoChildrenPerDim.push_back(int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel))));
                	}//end if
                	else {
                		//there is no meaning in creating a hierarchy along this dimension; instead create a pseudo level
                		maxNoChildrenPerDim.push_back(0);
                		noPseudoLevels++; //one more pseudo level
                	}//end else
        	}//end if
        	else {  //this is a Pseudo level
        		//since we apply (for now) this method only to large DATA chunks (and not dir chunks)
        		//there shouldn't be any pseudo level at the data level
        		throw GeneralError("AccessManager::EquiGrid_EquiChildren() ==> Pseudo level encountered in data chunk\n");
        	}//end else
       	}//for
                       	
       	//If all new levels will be pseudo levels
       	if(noPseudoLevels == noDims){                       	
                //then we have to reduce the number of members of the new levels
                // we will make it equal with the minimum number of grain level members for each dimension
                //noMembersNewLevel = minimum(noMembersGrain)  such that minimum(noMembersGrain) != 1

                // find the minimum number of grain members in the original chunk.
                vector<unsigned int>::const_iterator minimumMembersGrainIter = min_element(noMembersGrainVect.begin(), noMembersGrainVect.end());
                //if the min number is not equal with 1
                if(*minimumMembersGrainIter != 1) {
                        // The *new* number of new members inserted will be:
                        noMembersNewLevel = *minimumMembersGrainIter;
                        //Now, the maximum number of children per dim becomes:
                        for(int i=0; i<noDims; i++){
                                maxNoChildrenPerDim[i] = int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel)));
                        }//for
                        // No pseudo levels now
                        noPseudoLevels = 0;
                }//end if
                else {//else if min number equals 1
                        //find all dimensions with only 1 member and make them pseudo levels
                        noPseudoLevels = 0;
                        for(int i=0; i<noDims; i++){
                                if(noMembersGrainVect[i] == 1){
                                        //then this must be a
                                        maxNoChildrenPerDim[i] = 0; //pseudo level
                                        noPseudoLevels++;
                                        //also put maxint in then num of members vect in order to get it out of the way
                                        noMembersGrainVect[i] = UINT_MAX; //numeric_limits<unsigned int>::max();
                                }//end if
                        }//for
                        if(noPseudoLevels == noDims)
                                throw GeneralError("AccessManager::EquiGrid_EquiChildren() ==> Found large data chunk consisting of only ONE(!!!) cell. Sorry, cant handle this case, I think you should consider a re-design of your cube!!! :-)");
                        //Now that the 1's are out of the way,
                        //find the minimum No of grain members in the original chunk that is != 1
                        minimumMembersGrainIter = min_element(noMembersGrainVect.begin(), noMembersGrainVect.end());
                        // The *new* number of new members inserted will be:
                        noMembersNewLevel = *minimumMembersGrainIter;
                        //Now, the maximum number of children per dim becomes:
                        for(int i=0; i<noDims; i++){
                                //if not a pseudo level
                                if(maxNoChildrenPerDim[i] != 0)
                                        maxNoChildrenPerDim[i] = int(::ceil(double(noMembersGrainVect[i])/double(noMembersNewLevel)));
                        }//for
                }//end else
       	} //end if
                       	
}//end AccessManager::EquiGrid_EquiChildren::EquiGrid_EquiChildren()

LevelRange::LevelRange(const LevelRange& l)
	: dimName(l.dimName),lvlName(l.lvlName),leftEnd(l.leftEnd),rightEnd(l.rightEnd)
{
}
LevelRange const& LevelRange::operator=(LevelRange const& other)
{
	dimName = other.dimName;
	lvlName = other.lvlName;
	leftEnd = other.leftEnd;
	rightEnd = other.rightEnd;
	return (*this);
}

