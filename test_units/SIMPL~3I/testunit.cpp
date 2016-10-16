//////////////////////////////////////////////
// This file test the following unit:
// SimpleClusteringAlg::operator()
//
// (C) Nikos Karayannidis     10/09/2001
/////////////////////////////////////////////
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>



class ChunkID {

private:
	string cid;

public:
	/**
	 * Default Constructor of the ChunkID
	 */
	ChunkID() : cid(""){ }

	/**
	 * Constructor of the ChunkID
	 */
	ChunkID(const string& s) : cid(s) {}
	/**
	 * copy constructor
	 */
	ChunkID(const ChunkID& id) : cid(id.getcid()) {}
	/**
	 * Destructor of the ChunkID
	 */
	~ChunkID() { }

	/**
	 * Required in order to use the find standard algortithm (see STL)
	 * for ChunkIDs
	 */
	//friend bool operator==(const ChunkID& c1, const ChunkID& c2);

	/**
	 * This operator is required in order to define map containers (see STL)
	 * with a ChunkID as a key.
	 */
	//friend bool operator<(const ChunkID& c1, const ChunkID& c2);


	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }

};


struct BucketID {
  	/**
	 * SSM Logical Volume id where this record's file resides
	 */
	//lvid_t vid; //you can retrieve this from the SystemManager

  	/**
	 * SSM logical record id
	 */
	int rid;

	/**
	 * Default constructor
	 */
//	BucketID() : vid(), rid(serial_t::null) {}
//	BucketID(const lvid_t& v, const serial_t& r) : vid(v), rid(r) {}
	/**
	 * copy constructor
	 */
//	BucketID(const BucketID& bid) : vid(bid.vid),rid(bid.rid) {}

	/**
	 * Default constructor
	 */
	BucketID() : rid(0) {}
	BucketID(const int& r) : rid(r) {}
	/**
	 * copy constructor
	 */
	BucketID(const BucketID& bid) : rid(bid.rid) {}

	/**
	 * Check for a null bucket id
	 */
	bool isnull() {return (rid == 0);}

	friend bool operator==(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid == b2.rid);
	}

	friend bool operator!=(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid != b2.rid);
	}

	friend bool operator<(const BucketID& b1, const BucketID& b2) {
	 	return (b1.rid < b2.rid);
	}

	/**
	 * This function creates a new BucketID. This id corresponds to a new SSM record id.
	 * NOTE: that the record is not created yet, just the id.
	 */
	static BucketID createNewID();
};


BucketID BucketID::createNewID()
{
	// Create a BucketID for the root Bucket.
	// NOTE: no bucket allocation performed, just id generation!
	//rc_t err;
	int record_ID;
        record_ID = random()% 100;
        /*err = ss_m::create_id(SystemManager::getDevVolInfo()->volumeID , 1, record_ID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"BucketID::createNewID ==> Error in ss_m::create_id"<< err <<endl;
		// throw an exeption
		throw error.str();
	}*/

	return BucketID(record_ID);
}// end of BucketID::createNewID

struct DiskBucket {
        enum {bodysize = 100};

        static const unsigned int BCKT_THRESHOLD = int(bodysize*0.5);
};

/**
* A structure for the case-vectors in AccessManager::putIntoBuckets.
* An instance corresponds to a tree (or single data chunk).
*/
struct CaseStruct {
        /**
        * The chunk id of the root of the tree
        */
        ChunkID id;
        /**
        * The size-cost of the tree
        */
        unsigned int cost;
};


class SimpleClusteringAlg {
        private:
                //no extra input parameters needed for this algorithm.
        public:
                /**
                * This implements the algorithm.The algorithm sequencially scans the
                * input vector and tries to put in the same bucket as many trees as possible.
                * NOTE: the algorithm does not sort the input vector nor it assumes any specific order.
                *	 However, due to the way the caseBvect is constructed, its entries are sorted in
                * 	 ascending order of their chunk id.
                */
                void operator()( const vector<CaseStruct>& caseBvect,
                                multimap<BucketID,ChunkID>& resultRegions,
                                vector<BucketID>& resultBucketIDs
                                );
};

void fillCaseVect(vector<CaseStruct>& casevect);
void printResults(multimap<BucketID,ChunkID>& resultRegions,
                                vector<BucketID>& resultBucketIDs);

main(){
        vector<CaseStruct> caseBvect;
        fillCaseVect(caseBvect);

	multimap<BucketID, ChunkID> resultRegions;
	vector<BucketID> resultBucketIDs;

        SimpleClusteringAlg algorithm; //init function object
        try {
                algorithm(caseBvect, resultRegions, resultBucketIDs);
        }
        catch(const char* message) {
                string msg("");
                msg += message;
                cerr<< msg.c_str();
                return 0;
        }

        printResults(resultRegions, resultBucketIDs);
}// end of main

void fillCaseVect(vector<CaseStruct>& casevect)
{
       CaseStruct h = {ChunkID("s1"),4};
       casevect.push_back(h);

       CaseStruct h2 = {ChunkID("s2"),9};
       casevect.push_back(h2);

       CaseStruct h3 = {ChunkID("s3"),4};
       casevect.push_back(h3);

       CaseStruct h4 = {ChunkID("s4"),4};
       casevect.push_back(h4);

       CaseStruct h5 = {ChunkID("s5"),4};
       casevect.push_back(h5);

       CaseStruct h6 = {ChunkID("s6"),4};
       casevect.push_back(h6);

}

void printResults(multimap<BucketID,ChunkID>& resultRegions,
                                vector<BucketID>& resultBucketIDs)
{
	//for each region...
	typedef multimap<BucketID, ChunkID>::const_iterator MMAP_ITER;
	// for each bucket id corresponding to a region
	for(vector<BucketID>::const_iterator buck_i = resultBucketIDs.begin();
	    buck_i != resultBucketIDs.end(); ++buck_i) {
                cout<<"Bucket: "<<buck_i->rid<<endl;
		//find the range of "items" in a specific region corresponding to a specific BucketID
        	pair<MMAP_ITER,MMAP_ITER> c = resultRegions.equal_range(*buck_i);

        	// iterate through all chunk ids of a region
                cout<<"\t";
        	for(MMAP_ITER iter = c.first; iter!=c.second; ++iter) {
                        cout<<iter->second.getcid()<<", ";
                } //end for
                cout<<endl;
         }//end for
}



void SimpleClusteringAlg::operator() (  const vector<CaseStruct>& caseBvect,
					multimap<BucketID,ChunkID>& resultRegions,
					vector<BucketID>& resultBucketIDs  )
// precondition:
//	each entry of caseBvect corresponds to a chunk-tree. All trees are hanging from the same parent
//	and have a size-cost less than the bucket threshold.
// postcondition:
//	resultBucketIDs contains the set of bucket ids that correspond to the buckets that will store
//	the formulated regions. Each region is stored in a single bucket. resultRegions contains the
// 	mapping of each bucket id to the chunk ids of the roots of the trees of the corresponding region.
{
        //ASSERTION: no empty input
        if(caseBvect.empty())
                throw "SimpleClusteringAlg::operator() ==> empty input vector\n";

	size_t CLUSTER_SIZE_LIMIT = DiskBucket::bodysize; //a cluster's size must not exceed this limit

	//cost of current cluster
	size_t curr_clst_cost = 0;

	//create a new bucket id
	BucketID curr_id = BucketID::createNewID();

	//insert bucket id into bucket id output vector
	resultBucketIDs.push_back(curr_id);

	//loop from the  1st item to the last of the input vector
	for(vector<CaseStruct>::const_iterator iter = caseBvect.begin(); iter != caseBvect.end(); iter++) {
                //ASSERTION: cost must be below Threshold
                if(iter->cost >= DiskBucket::BCKT_THRESHOLD)
                        throw "SimpleClusteringAlg::operator() ==> cost of tree exceeds bucket threshold!\n";
		//add the cost of current item to the cost of current cluster
		//if the cost of current cluster is still below the limit
		if(curr_clst_cost + (*iter).cost <= CLUSTER_SIZE_LIMIT) {
			//then insert current item in the current cluster
                        resultRegions.insert(make_pair(curr_id, iter->id));
			curr_clst_cost += (*iter).cost; // update cluster cost
		}
		else {
			//create a new bucket id
			curr_id = BucketID::createNewID();
                	//insert bucket id into bucket id output vector
                	resultBucketIDs.push_back(curr_id);
                	// re-initialize cluster cost
                	curr_clst_cost = 0;
			//then insert current item in the current cluster
			resultRegions.insert(make_pair(curr_id, iter->id));
			curr_clst_cost += (*iter).cost; // update cluster cost
		}// end else
	}//end for
}// end of AccessManager::SimpleClusteringAlg::operator()
