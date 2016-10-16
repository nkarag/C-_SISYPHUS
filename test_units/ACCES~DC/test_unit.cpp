#include <string>
#include <iostream>
#include <vector>

#include "Exceptions.h"

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

       		
	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }
	
	// empty chunk id
	bool empty() const {return cid.empty();}
	
	const int getNumDomains() const;

};//end class ChunkID


void removeArtificialDomainsFromChunkIDs(int nodoms, const vector<ChunkID>& inputVect,vector<ChunkID>& outputVect);

main()
{
        vector<ChunkID> inp;
        inp.reserve(3);
        inp.push_back(ChunkID(string("12|34|32.-1|2|786.8439|394|434.34|35|-1")));
        inp.push_back(ChunkID(string("12|34|32.-1|2|786.8439|394|434.34|35|-1")));
        inp.push_back(ChunkID(string("12|34|32.-1|2|786.8439|394|434.34|35|-1")));

        vector<ChunkID> out;
        out.reserve(3);
        try{
                removeArtificialDomainsFromChunkIDs(2, inp, out);
        }
        catch(GeneralError& e){
                cerr<<e<<endl;
                exit(1);
        }

        for(vector<ChunkID>::const_iterator iter = out.begin(); iter != out.end(); iter++){
                cout<<iter->getcid()<<", ";
        }//for
        cout<<endl;
}//main

void removeArtificialDomainsFromChunkIDs(int nodoms, const vector<ChunkID>& inputVect,vector<ChunkID>& outputVect)
//precondition:
//		nodoms > 0, inputVect non empty
//processing
//		remove domains form chunk ids and insert old ids without the removed part into outputVect
//postcondition:
//		outputVect.size() == inputVect.size()
{
	//ASSERTION: valid nodoms
	if(nodoms <= 0)
		throw GeneralError("AccessManager::removeArtificialDomainsFromChunkIDs ==>ASSERTION: requested No of removed domains <= 0!");	

	//for each input chunk id
	for(vector<ChunkID>::const_iterator inpID = inputVect.begin(); inpID != inputVect.end(); inpID++){
      		//ASSERTION: not too many domains will be removed: For a data chunk minimum domains that can be left after removal is 2		
      		if(inpID->getNumDomains() - nodoms < 2)
      			throw GeneralError("AccessManager::removeArtificialDomainsFromChunkIDs ==>ASSERTION: too many number of domains are to be removed from data chunk id!");

                //get a copy of current chunk id
                string copyid = inpID->getcid();			        			      			      			
        			
      		//get position of last dot
      		string::size_type last_dot_pos = copyid.rfind("."); //find last "."
               	if (last_dot_pos == string::npos) //then no "." found.
               		throw GeneralError("AccessManager::removeArtificialDomainsFromChunkIDs ==>ASSERTION: only one domain found, no interemediate domains to remove!");
      		//get end of substr to remove
      		string::size_type toRemoveEnd = last_dot_pos - 1;         		
               	//get beginning of substr to remove
               	string::size_type toRemoveBegin;
               	string::size_type pos = toRemoveEnd;
               	int counter = nodoms;
               	while(counter){
      			pos = copyid.rfind(".", pos); //get next "." moving backwards from end to begin
      			if (pos == string::npos) //then no "." found.			
      				throw GeneralError("AccessManager::removeArtificialDomainsFromChunkIDs ==>ASSERTION: cant find domains to remove!!!");
      			counter--;
      			pos -= 1; //go one before current dot         	
               	}//end while
               	// now pos is 2 characters before the beginning of the substr to remove
               	toRemoveBegin = pos + 1; // move only one character ahead in order to also remove one of the two dots        			
         	
		//remove substr
		copyid.erase(toRemoveBegin, toRemoveEnd - toRemoveBegin + 1);
		
		//insert in output vector
		outputVect.push_back(ChunkID(copyid));
	}//end for
	
	//ASSERTION
	if(inputVect.size() != outputVect.size())
		throw GeneralError("AccessManager::removeArtificialDomainsFromChunkIDs ==>ASSERTION: outputVect has a wrong number of elements!");	
}//AccessManager::removeArtificialDomainsFromChunkIDs()


const int ChunkID::getNumDomains() const
{
	if(cid.empty())
		return -1;
		
	if(cid == "root")
		return 0;		

	int nodoms = 1;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			nodoms++;
	}
	return nodoms;
}//end of ChunkID::getNumDomains()
