#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "Exceptions.h"

/**
 * This class represents a chunk id. i.e. the unique string identifier that is
 * derived from the interleaving of the member-codes of the members of the pivot-set levels
 * that define this chunk.
 * @author: Nikos Karayannidis
 */
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
	friend bool operator==(const ChunkID& c1, const ChunkID& c2)
        {
        	return (c1.cid == c2.cid);
        }

       		
	// get/set chunk id
	const string& getcid() const { return cid; }
	void setcid(const string& id) { cid = id; }
	
	// empty chunk id
	bool empty() const {return cid.empty();}

};//end class ChunkID


class CellMap {

public:
	/**
	 * default constructor, initializes an empty vector
	 */
	CellMap::CellMap() : chunkidVectp(new vector<ChunkID>) {}
	CellMap::~CellMap() { delete chunkidVectp; }

	/**
	 * copy constructor
	 */
	CellMap(CellMap const & map){
        	// copy the data
		chunkidVectp = new vector<ChunkID>(*(map.getchunkidVectp()));
	}

	/**
	 * overload assignment operator
	 */
	CellMap const& operator=(CellMap const& other);
	
	/**
	 * Returns true if cell map is empty
	 */
         bool empty() const{
                return chunkidVectp->empty();
         }//end empty()
	
	/**
	 * This function inserts a new id in the Chunk Id vector. If the id
	 * already exists then it returns false without inserting the id.
	 *
	 * @param	id	the chunk id string
	 */
	 bool insert(const string& id);

	/**
	 * get/set
	 */
	const vector<ChunkID>* getchunkidVectp() const {return chunkidVectp;}
	void setchunkidVectp(vector<ChunkID>* const chv){chunkidVectp = chv;}

	//const vector<ChunkID>& getchunkidVectp() const {return chunkidVect;}
	//void setchunkidVect(const vector<ChunkID>& chv);


private:
	vector<ChunkID>* chunkidVectp;
	//vector<ChunkID>& chunkidVect;
}; //end of class CellMap


/**
* This function scans the input fact file to find prefix matches of the chunk ids in the file
* with the chunk id of a specific source chunk (dir or data). It then creates a CellMap structure
* containing the chunk ids (cells of the source chunk) that were located in the file.
*
* @param	factFile	the input file with fact values
* @param	prefix		the prefix (id of source chunk) that we use as a matching pattern - input parameter
* @param	isDataChunk	a flag indicating whether the prefix corresponds to a data chunk or a directory chunk. - input parameter
*/
CellMap* scanFileForPrefix(const string& factFile,const string& prefix, bool isDataChunk = false);

int main(){
        cout << "Give me input file: ";
        string file;
        cin >> file;

        cout << "Give me a prefix: ";
        string prefix;
        cin >> prefix;

        cout << "Is this a Data chunk: ";
        string ans;
        cin >> ans;
        bool isDataChunk = (ans[0] == 'y') ? true : false;

        CellMap* mapp = 0;
        try{
                mapp = scanFileForPrefix(file,prefix, isDataChunk);
        }
        catch(GeneralError& e){
                cout << e;
                exit(1);
        }

        //print CellMap
        for(vector<ChunkID>::const_iterator citer = mapp->getchunkidVectp()->begin(); citer != mapp->getchunkidVectp()->end(); citer++){
                cout << citer->getcid() << endl;
        }//end for

}//main

CellMap* scanFileForPrefix(const string& factFile,const string& prefix, bool isDataChunk = false)
//precondition:
//      We assume that the fact file contains in each line: <cell chunk id>\t<cell value1>\t<cell value2>...<\t><cell valueN>
//      All the chunk ids in the file correspond only to grain level data points. Also all chunk id contain at least 2 domains, i.e.,
//      all dimensions have at least a 2-level hierarchy. "prefix" input parameter corresponds to the chunk id of a specific chunk (source chunk)
//      whose cells we want to search for in the input file. "isDataChunk" indicates whether the latter is data chunk or a directory chunk.
//processing:
//      For directory chunks when we find a match, then we insert in a CellMap the prefix+the "next domain"
//      from the chunk-id that matched. All following matches with this "next domain" are discarded
//                                                              -----------------------
//      (i.e. no insertion takes place).
//      For data chunks if we do find a second match with the same "next domain" following the prefix, then
//      we throw an exception, since that would mean that we have found values for the same cell, more
//      than once in the input file.
//postcondition:
//      A pointer to a CellMap is returned containing the existing cells of the source chunk that were found in the input file
{
	// open input file for reading
	ifstream input(factFile.c_str());
	if(!input)
		throw GeneralError("Chunk::scanFileForPrefix ==> Error in creating ifstream obj, in Chunk::scanFileForPrefix\n");
		
        #ifdef DEBUGGING
              cerr<<"Chunk::scanFileForPrefix ==> Just opened file: "<<factFile.c_str()<<endl;
              cerr<<"Chunk::scanFileForPrefix ==> Prefix is : "<<prefix<<endl;
        #endif		
		
	string buffer;
	// skip all schema staff and get to the fact values section
	do{
		input >> buffer;
	}while(buffer != "VALUES_START");

	CellMap* mapp = new CellMap;
	input >> buffer;
	while(buffer != "VALUES_END"){		
                /*#ifdef DEBUGGING
                	cerr<<"Chunk::scanFileForPrefix ==> buffer = "<<buffer<<endl;
                #endif*/		         	
		if(prefix == "root"){ //then we are scanning for the root chunk
                	// get the first domain from each entry and insert it into the CellMap
			string::size_type pos = buffer.find("."); // Find character "." in buffer starting from left
			if (pos == string::npos){
				throw GeneralError("Chunk::scanFileForPrefix ==> Assertion 1: ChunkID syntax error: no \".\" in id in fact load file\n");
			}
			string child_chunk_id(buffer,0,pos);
                        //#ifdef DEBUGGING
                        //      cerr<<"Chunk::scanFileForPrefix ==> child_chunk_id = "<<child_chunk_id<<endl;
                        //#endif		         				
			mapp->insert(child_chunk_id);
		}
		else {
			string::size_type pos = buffer.find(prefix);
			if(pos==0) { // then we got a prefix match
        			//get the next domain
        			// find pos of first character not of prefix
        			//string::size_type pos = buffer.find_first_not_of(prefix); // get the position after the prefix (this should be a ".")
        			pos = prefix.length(); // this must point to a "."
        			if (buffer[pos] != '.') {
                                        throw GeneralError("Chunk::scanFileForPrefix ==> Assertion2:  ChunkID syntax error: no \".\" (after input prefix) in id in fact load file\n");
                                }//end if
        			pos = pos + 1; //move on one position to get to the first character
                                /*#ifdef DEBUGGING
                                      cerr<<"pos = "<<pos<<endl;
                                #endif*/		         							
        			string::size_type pos2 = buffer.find(".", pos); // find the end of the next domain
                                /*#ifdef DEBUGGING
                                      cerr<<"pos2 = "<<pos2<<endl;
                                #endif*/		         										
        			string nextDom(buffer,pos,pos2-pos); //get next domain
                                /*#ifdef DEBUGGING
                                      cerr<<"nextDom = "<<nextDom<<endl;
                                #endif*/		         										
        			string child_chunk_id = prefix + string(".") + nextDom; // construct child chunk's id
                                #ifdef DEBUGGING
                                      cerr<<"Chunk::scanFileForPrefix ==> child_chunk_id = "<<child_chunk_id<<endl;
                                #endif		         				
        			// insert into CellMap
       				bool firstTimeInserted = mapp->insert(child_chunk_id);        				
       				if(!firstTimeInserted && isDataChunk) {
                               		//then we have found a double entry, i.e. the same cell is given a value more than once
       					string msg = string ("Chunk::scanFileForPrefix ==> Error in input file: double chunk id: ") + child_chunk_id;
       					throw GeneralError(msg.c_str());
       				}//end if        			
        		}// end if
		} // end else
		// read on until the '\n', in order to skip the fact values
		getline(input,buffer);
		// now, read next id
		input >> buffer;
	}//end while
	input.close();
        #ifdef DEBUGGING
		/*cerr<<"Printing contents of CellMap : \n";
                for(vector<ChunkID>::const_iterator i = mapp->getchunkidVectp()->begin();
                	  i != mapp->getchunkidVectp()->end(); ++i){

  			cerr<<(*i).getcid()<<", "<<endl;            	
                }*/
                if(mapp->getchunkidVectp()->empty()) {
                 	cerr<<"Chunk::scanFileForPrefix ==> CellMapp is empty!!!\n";
                }
        #endif		         		
	return mapp;
} //end Chunk::scanFileforPrefix

bool CellMap::insert(const string& id)
{
	if(id.empty())
		throw GeneralError("Error inside CellMap::insert : empty chunk id!\n");
	// check if the chunk id already exists
	ChunkID newId(id);
	vector<ChunkID>::iterator result = find(chunkidVectp->begin(), chunkidVectp->end(), newId);
	/*#ifdef DEBUGGING
		cerr<<"CellMap::insert ==> result =  "<<result<<", chunk id to insert = "<<newId.getcid()<<endl;
	#endif*/		
	if(result == chunkidVectp->end()){
         	// OK its a new one
         	chunkidVectp->push_back(newId);
         	#ifdef DEBUGGING
         		cerr<<"CellMap::insert ==> Just inserted into Cellmap id : "<<chunkidVectp->back().getcid()<<endl;
         	#endif
	
		return true;
	}
	return false;
}//end CellMap::insert
