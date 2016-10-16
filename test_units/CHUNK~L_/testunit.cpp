#include <iostream>
#include <string>

void extractCoords(string& cid);

main(){
        cout <<"give me a chunk id: ";
        string chunkid;
        cin >> chunkid;

        extractCoords(chunkid);


}

void extractCoords(string& cid)
// precondition:
//	cid member contains a valid chunk id
// postcondition:
//	each coordinate from the last domain of the chunk id (member cid) has been stored
//	in the vector of the Coordinates struct in the same order as the interleaving order of the
//	chunk id: major-to-minor from left-to-right.
{
	if(cid == "root")
		throw "ChunkID::extractCoords ==> Can't extract coords from \"root\"\n";
		
	// 1. get last domain
	string::size_type pos = cid.rfind("."); //find last "."
	string lastdom;
	if (pos == string::npos){ //then no "." found. This must be a child of the root chunk.
		lastdom = cid;
	}
	else {
		pos += 1; // move on to the first char of the last domain
		string::size_type ldom_length = cid.length()-pos;
		lastdom = cid.substr(pos, ldom_length); // read substring cid[pos]..cid[pos+ldom_length -1]
	}
	
	// 2. update c
	string::size_type begin = 0;
	string::size_type end = lastdom.find("|", begin); // get 1st "|", i.e. coordinate for 1st dimension
	//cout<<"begin = "<<begin<<", end = "<<end<<endl;
	while(end != string::npos){
        	string coordstr(lastdom, begin, end-begin); // substring lastdom[begin]...lastdom[begin+(end-begin)-1]
        	//cout <<"coordstr = "<<coordstr<<endl;
        	int coord = (unsigned int)atoi(coordstr.c_str());
        	//c.cVect.pushback(coord);
        	cout <<" coord = "<<coord<<endl;
        	//c.numCoords += 1;
        	// read on
        	begin = end + 1;
        	end = lastdom.find("|", begin);
       		//cout<<"begin = "<<begin<<", end = "<<end<<endl;
        } //end while
        // repeat once more for the last dimension
       	string coordstr(lastdom, begin, lastdom.length()-begin); // substring lastdom[begin]...lastdom[begin+l-1], l=lastdom.length()-begin
        //cout <<"coordstr = "<<coordstr<<endl;
       	int coord = (unsigned int)atoi(coordstr.c_str());
       	//c.cVect.pushback(coord);
       	cout <<" coord = "<<coord<<endl;
       	//c.numCoords += 1;
       	
} // end of ChunkID::extractCoords
