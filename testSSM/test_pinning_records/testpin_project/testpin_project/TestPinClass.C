/***************************************************************************
                          TestPinClass.C  -  description
                             -------------------
    begin                : Wed Jan 31 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : 
 ***************************************************************************/

#include "TestPinClass.h"
#include "SystemManager.h"
#include <iostream>
#include <list>
#include <cstring>


TestPinClass::TestPinClass()
{
	cout <<"Beginning Testing...\n";
	//W_COERCE(ss_m::begin_xct());
	try {
		test_pin();
	 	cout<<"end of test_pin"<<endl;
	}
	catch(const char* message) {
		string msg("");
		msg += message;
		cerr << msg.c_str();
		W_COERCE(ss_m::abort_xct());
	}		
 	//W_COERCE(ss_m::commit_xct());
 	cout<<"end of testing..."<<endl;
}

void TestPinClass::createTestChunks(vector<DirChunk>* dirp,
			vector<DataChunk>* datap,
			map<ChunkID, pair<size_t,size_t> >& mymap,
			serial_t& recordID)
{
       	DirChunk dirc;
	DataChunk datac;
	char c;
				
	ChunkID cid1("subtree1");	
	mymap[cid1] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 1 : 30 DirChunks, 60 DataChunks\n";
	cin.get(c);	

	unsigned int next_chunk_slot = 0;	
	for(int i=0; i<30; i++){
       		next_chunk_slot = i;
       		int no_ent = rand() % 10;
       		// update chunk header
       		dirc.hdr.no_entries = no_ent;
       		dirc.hdr.no_measures = 0;       		
       		strcpy(dirc.hdr.id, cid1.c_str());
		for(int j = 0; j<= no_ent; j++) {
        		DirChunk::Entry e;
        		e.bucketid = recordID;
        		e.index = j;
        		dirc.entries.push_back(e);
        	}
		dirp->push_back(dirc);
		dirc.entries.erase(dirc.entries.begin(), dirc.entries.end());
		cout<<"dir chunk "<<i<<" with "<<no_ent<<" entries\n";
	}	

	next_chunk_slot++;
	unsigned int end = next_chunk_slot+60;		
	for(int i=next_chunk_slot; i < end; i++){
		next_chunk_slot = i;
       		int no_ent = rand() % 15;
       		// update chunk header
       		datac.hdr.no_entries = no_ent;
       		datac.hdr.no_measures = 2;       		
       		strcpy(datac.hdr.id, cid1.c_str());
       		
		for(int j = 0; j<= no_ent; j++) {		
        		DataChunk::Entry e;
        		e.measures.push_back(1+ j*0.1); //[0] = (1+ j*0.1);
        		e.measures.push_back(1+ j*0.2); //[1] = (1+ j*0.2);
        		datac.entries.push_back(e);		
        	}		
		datap->push_back(datac);
		datac.entries.erase(datac.entries.begin(), datac.entries.end());
		cout<<"data chunk "<<i<<" with "<<no_ent<<" entries\n";	
	}		
        	        	
	ChunkID cid2("subtree2");
	mymap[cid2] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 2 : 100 DirChunks, 300 DataChunks\n";
	cout<<"This will be stored at dirp index: "<<mymap[cid2].first<<" and at datap index: "<<mymap[cid2].second<<endl;
	cin.get(c);

	next_chunk_slot++;
	end = next_chunk_slot + 100;		
	for(int i=next_chunk_slot; i<end; i++){
       		next_chunk_slot = i;
		
       		int no_ent = rand() % 10;
       		// update chunk header
       		dirc.hdr.no_entries = no_ent;
       		dirc.hdr.no_measures = 0;       		
       		strcpy(dirc.hdr.id, cid2.c_str());

		for(int j = 0; j<= no_ent; j++) {
        		DirChunk::Entry e;
        		e.bucketid = recordID;
        		e.index = j;
        		dirc.entries.push_back(e);
        	}
		dirp->push_back(dirc);
		dirc.entries.erase(dirc.entries.begin(), dirc.entries.end());
		cout<<"dir chunk "<<i<<" with "<<no_ent<<" entries\n";
	}	
	
	next_chunk_slot++;
	end = next_chunk_slot+300;	
	for(int i=next_chunk_slot; i<end; i++){
		next_chunk_slot = i;
       		int no_ent = rand() % 15;
       		// update chunk header
       		datac.hdr.no_entries = no_ent;
       		datac.hdr.no_measures = 2;       		
       		strcpy(datac.hdr.id, cid2.c_str());
       		
		for(int j = 0; j<= no_ent; j++) {		
        		DataChunk::Entry e;
        		e.measures.push_back(3+ j*0.1); //[0] = (1+ j*0.1);
        		e.measures.push_back(3+ j*0.2); //[1] = (1+ j*0.2);        		
        		//e.measures[0] = (3+ j*0.1);
        		//e.measures[1] = (3+ j*0.2);
        		datac.entries.push_back(e);		
        	}		
		datap->push_back(datac);
		datac.entries.erase(datac.entries.begin(), datac.entries.end());
		cout<<"data chunk "<<i<<" with "<<no_ent<<" entries\n";	
	}			

	ChunkID cid3("subtree3");
	mymap[cid3] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 3 : 10 DirChunks, 20 DataChunks\n";
	cout<<"This will be stored at dirp index: "<<mymap[cid3].first<<" and at datap index: "<<mymap[cid3].second<<endl;
	cin.get(c);
	
	next_chunk_slot++;
	end = next_chunk_slot+10;		
	for(int i=next_chunk_slot; i<end; i++){
       		next_chunk_slot = i;		
       		
       		int no_ent = rand() % 10;
       		// update chunk header
       		dirc.hdr.no_entries = no_ent;
       		dirc.hdr.no_measures = 0;       		
       		strcpy(dirc.hdr.id, cid3.c_str());
       		
		for(int j = 0; j<= no_ent; j++) {
        		DirChunk::Entry e;
        		e.bucketid = recordID;
        		e.index = j;
        		dirc.entries.push_back(e);
        	}
		dirp->push_back(dirc);
		dirc.entries.erase(dirc.entries.begin(), dirc.entries.end());
		cout<<"dir chunk "<<i<<" with "<<no_ent<<" entries\n";
	}		

	next_chunk_slot++;
	end = next_chunk_slot+20;	
	for(int i=next_chunk_slot; i<end; i++){
		next_chunk_slot = i;
       		int no_ent = rand() % 15;
       		// update chunk header
       		datac.hdr.no_entries = no_ent;
       		datac.hdr.no_measures = 2;       		
       		strcpy(datac.hdr.id, cid3.c_str());
       		
		for(int j = 0; j<= no_ent; j++) {		
        		DataChunk::Entry e;
        		e.measures.push_back(5+ j*0.1); //[0] = (1+ j*0.1);
        		e.measures.push_back(5+ j*0.2); //[1] = (1+ j*0.2);        		        		
        		//e.measures[0] = (5+ j*0.1);
        		//e.measures[1] = (5+ j*0.2);
        		datac.entries.push_back(e);		
        	}		
		datap->push_back(datac);
		datac.entries.erase(datac.entries.begin(), datac.entries.end());
		cout<<"data chunk "<<i<<" with "<<no_ent<<" entries\n";	
	}		
		
	ChunkID cid4("subtree4");
	mymap[cid4] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 4 : 10 DirChunks, 10 DataChunks\n";
	cout<<"This will be stored at dirp index: "<<mymap[cid4].first<<" and at datap index: "<<mymap[cid4].second<<endl;
	cin.get(c);
	
	next_chunk_slot++;
	end = next_chunk_slot+10;		
	for(int i=next_chunk_slot; i<end; i++){
       		next_chunk_slot = i;
		
       		int no_ent = rand() % 10;
       		// update chunk header
       		dirc.hdr.no_entries = no_ent;
       		dirc.hdr.no_measures = 0;       		
       		strcpy(dirc.hdr.id, cid4.c_str());
       		
		for(int j = 0; j<= no_ent; j++) {
        		DirChunk::Entry e;
        		e.bucketid = recordID;
        		e.index = j;
        		dirc.entries.push_back(e);
        	}
		dirp->push_back(dirc);
		dirc.entries.erase(dirc.entries.begin(), dirc.entries.end());
		cout<<"dir chunk "<<i<<" with "<<no_ent<<" entries\n";
	}		

	next_chunk_slot++;
	end = next_chunk_slot+10;	
	for(int i=next_chunk_slot; i<end; i++){
		next_chunk_slot = i;
       		int no_ent = rand() % 15;
       		// update chunk header
       		datac.hdr.no_entries = no_ent;
       		datac.hdr.no_measures = 2;       		
       		strcpy(datac.hdr.id, cid4.c_str());
       		
		for(int j = 0; j<= no_ent; j++) {		
        		DataChunk::Entry e;
        		e.measures.push_back(7+ j*0.1); //[0] = (1+ j*0.1);
        		e.measures.push_back(7+ j*0.2); //[1] = (1+ j*0.2);        		        		
        		//e.measures[0] = (7+ j*0.1);
        		//e.measures[1] = (7+ j*0.2);
        		datac.entries.push_back(e);		
        	}		
		datap->push_back(datac);
		datac.entries.erase(datac.entries.begin(), datac.entries.end());
		cout<<"data chunk "<<i<<" with "<<no_ent<<" entries\n";	
	}			

} // end TestPinClass::createTestChunks		

void TestPinClass::test_pin()
{
	W_COERCE(ss_m::begin_xct());
	cout<<"Create the File ID of the cube....\n";
	char c;
	cin.get(c);
	
	serial_t shore_fid;
	rc_t err = ss_m::create_file(SystemManager::getDevVolInfo()->volumeID, shore_fid, ss_m::t_regular);
	if(err) {
		ostrstream error;
		// Print Shore error message
		error <<"FileManager::createCubeFile ==> "<< err <<endl;
		// throw an exeption
		throw error.str();
	}

	cout<<"Create the Bucket ID...\n";
	cin.get(c);
	
	//rc_t err;
	serial_t recordID;
        err = ss_m::create_id(SystemManager::getDevVolInfo()->volumeID , 1, recordID);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in ss_m::create_id : "<< err <<endl;
		// throw an exeption
		throw error.str();
	}
	
	W_COERCE(ss_m::commit_xct());         	         	
	
	// create some test sub-trees
	vector<DirChunk>* dirp = new vector<DirChunk>;
	vector<DataChunk>* datap = new vector<DataChunk>;
	map<ChunkID, pair<size_t,size_t> > mymap;
	
	createTestChunks(dirp, datap, mymap, recordID);
	
	
	cout <<"Create DiskBucket...\n";
	cin.get(c);	
	
	W_COERCE(ss_m::begin_xct());	
	
	DiskBucket* dbuckp = new DiskBucket;
	// initialize directory pointer to point one beyond last byte of body
	dbuckp->offsetInBucket = reinterpret_cast<DiskBucket::dirent_t*>(&(dbuckp->body[DiskBucket::bodysize]));
	// initialize header
	dbuckp->hdr.id.id = recordID; // store the bucket id
	dbuckp->hdr.freespace = DiskBucket::bodysize;
	dbuckp->hdr.next.id = serial_t::null;
	dbuckp->hdr.previous.id = serial_t::null;
	dbuckp->hdr.no_chunks = 0;
	dbuckp->hdr.next_offset = 0;

	// For each subtree that will be stored in this bucket	
	char* currentp = dbuckp->body; //init current byte pointer
	size_t curr_offs = dbuckp->hdr.next_offset;          // init current byte offset
	for(map<ChunkID, pair<size_t,size_t> >::const_iterator map_i = mymap.begin();
	        map_i != mymap.end(); ++map_i) {
		
	        ChunkID subtree = map_i->first;
        	cout <<"Putting "<<subtree<<" in the DiskBucket...\n";
        	cin.get(c);	
        	
        	// initialize current pointer in the bucket body
        	// to the offset of the 1st available chunk slot
        	//currentp = dbuckp->body + dbuckp->hdr.next_offset;
        	//if(currentp > reinterpret_cast<char*>(dbuckp->offsetInBucket - reinterpret_cast<DiskBucket::dirent_t*>(dbuckp->hdr.no_chunks*sizeof(DiskBucket::dirent_t))))
        		//throw "bucket directory has been corrupted\n";

                if(dbuckp->hdr.freespace < 200) {
                	cout<<"No more space in this bucket!!!\n";
                	break;
                }
        		
        	if( curr_offs > (DiskBucket::bodysize - dbuckp->hdr.no_chunks*sizeof(DiskBucket::dirent_t)) )
                        throw "bucket directory has been corrupted\n";        	
        	
        	// first initialize the "index range" for this tree        	
       		int beginIndexInDirVect;
         	int endIndexInDirVect;        	
        	beginIndexInDirVect = map_i->second.first;
                map<ChunkID, pair<size_t,size_t> >::const_iterator next_tree_i = map_i;
                next_tree_i++;
                if(next_tree_i != mymap.end()) {
        	        endIndexInDirVect = next_tree_i->second.first-1;
                }
                else {
        	        endIndexInDirVect = dirp->size()-1;
                }
                vector<DirChunk>& dir = *dirp;
                // for each dir chunk of this subtree
	        for(int i = beginIndexInDirVect; i<=endIndexInDirVect; i++){
        	//for(vector<DirChunk>::iterator dir_i = (dirp->begin())[map_i->second.first];
        	//	dir_i != (dirp->begin())[(map_i+1)->second.first];
        	//	dir_i++) {
        		
                        if(dbuckp->hdr.freespace < 200) {
                        	cout<<"No more space in this bucket!!!\n";
                        	break;
                        }
        		
        		// update bucket directory, beginning from chunk slot 0
        		//dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = reinterpret_cast<DiskBucket::dirent_t>(currentp - dbuckp->body);
        		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = curr_offs;
        		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
        		// store the chunk header first
        		DiskChunkHeader* c_hdrp = &(dir[i].hdr);
			memcpy(currentp, reinterpret_cast<char*>(c_hdrp), sizeof(DiskChunkHeader));
       			currentp += sizeof(DiskChunkHeader); // move on to the next empty position
       			curr_offs += sizeof(DiskChunkHeader);        		
       			dbuckp->hdr.freespace -= sizeof(DiskChunkHeader);
        		//delete c_hdrp;
        		
        		// for each entry of this dir chunk
        		for(vector<DirChunk::Entry>::iterator ent_i = dir[i].entries.begin();
        			ent_i != dir[i].entries.end();	ent_i++){
        		
        			DirChunk::Entry* ep = &(*ent_i);
        			memcpy(currentp, reinterpret_cast<char*>(ep), sizeof(DirChunk::Entry));
        			currentp += sizeof(DirChunk::Entry); // move on to the next entry position
        			curr_offs += sizeof(DirChunk::Entry);
        			dbuckp->hdr.freespace -= sizeof(DirChunk::Entry);
        		} // end for
        		// dir chunk stored, update bucket header
                 	//dbuckp->hdr.freespace -= static_cast<size_t>(currentp - dbuckp->body);
                 	dbuckp->hdr.no_chunks++;
                 	//dbuckp->hdr.next_offset = reinterpret_cast<size_t>(currentp - dbuckp->body);
                 	dbuckp->hdr.next_offset = curr_offs;
                 	
                 	cout<<"dirchunk : "<<i<<" just stored.\n";
                 	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
                 	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
                 	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
                 	cout<<"current offset = "<<curr_offs<<endl;
                        cout<<"---------------------\n";
        	} // end for
        	
	       	// first initialize the "index range" for this tree        	
       		int beginIndexInDataVect;
         	int endIndexInDataVect;        	
        	beginIndexInDataVect = map_i->second.second;
                if(next_tree_i != mymap.end()) {
        	        endIndexInDataVect = next_tree_i->second.second-1;                                	
                }
                else {
        	        endIndexInDataVect = datap->size()-1;
                }
                vector<DataChunk>& data = *datap;
              	// for each data chunk of this subtree
	        for(int i = beginIndexInDataVect; i<=endIndexInDataVect; i++){
        	
        	//for(vector<DataChunk>::const_iterator data_i = datap->begin() + map_i->second.second;
        	//	data_i != datap->begin() + (map_i+1)->second.second;
        	//	data_i++) {

                        if(dbuckp->hdr.freespace < 200) {
                        	cout<<"No more space in this bucket!!!\n";
                        	break;
                        }

        		// update bucket directory, beginning from chunk slot 0
        		//dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = reinterpret_cast<DiskBucket::dirent_t>(currentp - dbuckp->body);
        		dbuckp->offsetInBucket[-(dbuckp->hdr.no_chunks)-1] = curr_offs;
        		dbuckp->hdr.freespace -= sizeof(DiskBucket::dirent_t);
        		
        		// store the chunk header first
        		DiskChunkHeader* c_hdrp = &(data[i].hdr);
			memcpy(currentp, reinterpret_cast<char*>(c_hdrp), sizeof(DiskChunkHeader));
       			currentp += sizeof(DiskChunkHeader); // move on to the next empty position         		
       			curr_offs += sizeof(DiskChunkHeader);
       			dbuckp->hdr.freespace -= sizeof(DiskChunkHeader);       		
       			//delete c_hdrp;
        		        		
        		// for each entry of this data chunk
        		for(vector<DataChunk::Entry>::iterator ent_i = data[i].entries.begin();
        			ent_i != data[i].entries.end();	ent_i++){
        			
        			// for each measure of this entry
        			//for(vector<float>::iterator m_iter = ent_i->measures.begin();
        			//	 m_iter != ent_i->measures.end(); m_iter++){
        			for(int k = 0; k < data[i].hdr.no_measures; k++ ){
        				
        				float* fp = &ent_i->measures[k];//&(*m_iter);
        				memcpy(currentp, reinterpret_cast<char*>(fp), sizeof(float));
        				currentp += sizeof(float); // move on to the next emtpy position							
        				curr_offs += sizeof(float);
        				dbuckp->hdr.freespace -= sizeof(float);
        			} // end for
        		} // end for
        		// dir chunk stored, update bucket header
        		//dbuckp->hdr.freespace -= static_cast<size_t>(currentp - dbuckp->body);                 	
                 	dbuckp->hdr.no_chunks++;
                 	//dbuckp->hdr.next_offset = reinterpret_cast<size_t>(currentp - dbuckp->body);
                 	dbuckp->hdr.next_offset = curr_offs;
                 	
                	cout<<"datachunk : "<<i<<" just stored.\n";
                 	cout<<"freespace = "<<dbuckp->hdr.freespace<<endl;
                 	cout<<"no chunks = "<<dbuckp->hdr.no_chunks<<endl;
                 	cout<<"next offset = "<<dbuckp->hdr.next_offset<<endl;
                 	cout<<"current offset = "<<curr_offs<<endl;
                 	cout<<"---------------------\n";
        	} // end for
        } //end for
       	//if(currentp > reinterpret_cast<char*>(dbuckp->offsetInBucket - reinterpret_cast<DiskBucket::dirent_t*>(dbuckp->hdr.no_chunks*sizeof(DiskBucket::dirent_t))))
       		//throw "bucket directory has been corrupted\n";
        if( curr_offs > (DiskBucket::bodysize - dbuckp->hdr.no_chunks*sizeof(DiskBucket::dirent_t)) )
                        throw "bucket directory has been corrupted (outside for)\n";        	       		
  		
	cout <<"DiskBucket created successfully...\n\n";
	cout <<"store DiskBucket in SSM record...\n";	
	cin.get(c);	
		
	// create SSM record corresponding to a Bucket. Use the id created earlier.
        err = ss_m::create_rec_id(SystemManager::getDevVolInfo()->volumeID , shore_fid,
                         vec_t(0, 0),       /* header  */
                         PAGESIZE,  /* length hint          */
                         vec_t(dbuckp, sizeof(DiskBucket)), /* body    */
                         recordID);      /* rec id           */
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in ss_m::create_rec_id "<< err <<endl;
		// throw an exeption
		throw error.str();
	}

 	W_COERCE(ss_m::commit_xct());         	         	
 	
 	// free up memory
 	delete dbuckp;
 	delete datap;
 	delete dirp;
	        	
	cout <<"SSM record created successfully...\n\n";
	cout<<"Pin the Bucket...\n";
	cin.get(c);			
	
 	W_COERCE(ss_m::begin_xct());
 	
 	BucketID bid;
 	bid.id = recordID;
 	dbuckp = cpBucketToHeap(bid);
 	
 	/************

    	pin_i handle;
    	err = handle.pin(SystemManager::getDevVolInfo()->volumeID, recordID, 0);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in pin_i::pin  "<< err <<endl;
		// throw an exeption
		throw error.str();
	}

	// get pointer 	to the pinned bucket
	dbuckp = reinterpret_cast<DiskBucket*>(handle.body());
	
	// print some statistics about the record    	    	
    	cout<<"handle.length() = "<<handle.length()<<endl;
    	cout<<"handle.hdr_size() = "<<handle.hdr_size()<<endl;
    	cout<<"handle.body_size() = "<<handle.body_size()<<endl;
    	cout<<"handle.is_small() = "<<handle.is_small()	<<endl;
    	cout<<"handle.pinned_all() = "<<handle.pinned_all()<<endl;
    	*/
    	cout<<"\nRead the contents of the bucket...\n\n";
  	cin.get(c);			
  	
	ReadBucket readb(dbuckp);
  	cout<<"Read bucket header:\n";
  	readb.readHeader();
  	//readb.readBody(handle.pinned_all(), handle.length(), handle);
  	readb.readBody();
  		
 	W_COERCE(ss_m::commit_xct());         	         		

	// Now test updating a record!
    	cout << "now lets try to update a subtree!\n";
    	  	
    	//delete dbuckp; // no need heap space freed by ~ReadBucket()
} // end of TestPinClass::test_pin()

DiskBucket* TestPinClass::cpBucketToHeap(BucketID bid)
{
	//allocate space in heap
	DiskBucket* dbp = new DiskBucket;
	char* bytep = reinterpret_cast<char*>(dbp);
	
	//pin all the pages of the underlying SSM record
	// and copy their bodies to the allocated space in heap
	pin_i handle;
    	rc_t err = handle.pin(SystemManager::getDevVolInfo()->volumeID, bid.id, 0);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in pin_i::pin  "<< err <<endl;
		// throw an exeption
		throw error.str();
	}
	memcpy(bytep, handle.body(), handle.length());
	
	if(!handle.pinned_all()){
         	//get more pages
         	bool eof = false;
         	while(!eof){
         		bytep += handle.length(); //set pointer to next empty pos
                     	err = handle.next_bytes(eof);
                 	if(err) {
                 		// then something went wrong
                 		ostrstream error;
                 		// Print Shore error message
                 		error <<"Error in pin_i::next_bytes  "<< err <<endl;
                 		// throw an exeption
                 		throw error.str();
                 	}//end if
                 	memcpy(bytep, handle.body(), handle.length());
         	}//end while
	}//end if	
	
	return dbp;
}//end of DiskBucket::cpBucketToHeap()

void ReadBucket::readHeader()
{
       	cout<<"Read bucket header:\n";
     	cout <<"dbuckp->hdr.id.id = "<<dbuckp->hdr.id.id <<endl;
     	cout <<"dbuckp->hdr.freespace = "<<dbuckp->hdr.freespace<<endl;	
     	//cout <<"buckp->hdr.next = "<<buckp->hdr.next<<endl;	
     	//cout <<"buckp->hdr.previous = "<<buckp->hdr.previous<<endl;	
     	cout <<"dbuckp->hdr.no_chunks = "<<dbuckp->hdr.no_chunks<<endl;
     	cout <<"dbuckp->hdr.next_offset = "<<dbuckp->hdr.next_offset<<endl;	
} //end ReadBucket::readHeader()

void ReadBucket::readBody()
{
       	cout <<"\n\nRead the bucket body";
       	for(int slot = 0; slot < dbuckp->hdr.no_chunks; slot++) {
       		//const char* p = buckp->body + buckp->offsetInBucket[-slot-1];	
       		cout <<"dbuckp->offsetInBucket[-slot-1] = "<<dbuckp->offsetInBucket[-slot-1]<<endl;
         		
       		const char* p = &(dbuckp->body[dbuckp->offsetInBucket[-slot-1]]);
       		const DiskChunkHeader* chp = reinterpret_cast<const DiskChunkHeader*>(p);
       		(chp->no_measures == 0)?cout<<"Directory Chunk => ":cout<<"Data Chunk => ";
       		cout << "Chunk id = "<< chp->id<<endl;
       		cout << "no of entries = "<< chp->no_entries<<endl;
       		cout << "Entries : "<<endl;
       		cout << "-------------------"<<endl;
         		
       		p += sizeof(DiskChunkHeader); // move to 1st entry
       		if(chp->no_measures == 0) {//dir chunk
       			for(int i = 0; i<chp->no_entries; i++) {
       				DirChunk::Entry* ep = reinterpret_cast<DirChunk::Entry*>(p);
       				cout<<ep[i].index<<endl;			
       			}// end for	
       		}// end if
       		else { //data chunk
       		        const float* fp = reinterpret_cast<const float*>(p);
       			for(int i = 0; i<chp->no_entries; i++) {
       			        for(int j = 0; j<chp->no_measures; j++){						
       			                cout<<fp[j]<<",  ";
         			
       			        }
       			        fp += chp->no_measures; // move to next entry
       			        cout << endl;
       				//DataChunk::Entry* ep = reinterpret_cast<DataChunk::Entry*>(p);
       				//cout<<ep[i].measures[0]<<",   "<<ep[i].measures[1]<<endl;			
       			}// end for			
       		} // end else
       		cin.get();			
       	} //end for
}//end of ReadBucket::readBody()

void ReadBucket::readBody(bool whole_buck_pinned, size_t fst_pg_length, pin_i& fst_pg_hdl)
{
        if(whole_buck_pinned) { // all the bucket has been pinned
                 	
         	cout <<"\n\nRead the bucket body";
         	for(int slot = 0; slot < dbuckp->hdr.no_chunks; slot++) {
         		//const char* p = buckp->body + buckp->offsetInBucket[-slot-1];	
         		cout <<"dbuckp->offsetInBucket[-slot-1] = "<<dbuckp->offsetInBucket[-slot-1]<<endl;
         		
         		const char* p = &(dbuckp->body[dbuckp->offsetInBucket[-slot-1]]);
         		const DiskChunkHeader* chp = reinterpret_cast<const DiskChunkHeader*>(p);
         		(chp->no_measures == 0)?cout<<"Directory Chunk => ":cout<<"Data Chunk => ";
         		cout << "Chunk id = "<< chp->id<<endl;
         		cout << "no of entries = "<< chp->no_entries<<endl;
         		cout << "Entries : "<<endl;
         		cout << "-------------------"<<endl;
         		
         		p += sizeof(DiskChunkHeader); // move to 1st entry
         		if(chp->no_measures == 0) {//dir chunk
         			for(int i = 0; i<chp->no_entries; i++) {
         				DirChunk::Entry* ep = reinterpret_cast<DirChunk::Entry*>(p);
         				cout<<ep[i].index<<endl;			
         			}// end for	
         		}// end if
         		else { //data chunk
         		        const float* fp = reinterpret_cast<const float*>(p);
         			for(int i = 0; i<chp->no_entries; i++) {
         			        for(int j = 0; j<chp->no_measures; j++){						
         			                cout<<fp[j]<<",  ";
         			
         			        }
         			        fp += chp->no_measures; // move to next entry
         			        cout << endl;
         				//DataChunk::Entry* ep = reinterpret_cast<DataChunk::Entry*>(p);
         				//cout<<ep[i].measures[0]<<",   "<<ep[i].measures[1]<<endl;			
         			}// end for			
         		} // end else
         		cin.get();			
         	} //end for
        }//end if
	else { // just a part of the bucket has been pinned

                 cout<<"just a part of the bucket has been pinned\n";
         	// Pin the whole directory in the buffer pool
         	unsigned int no_dir_entries = dbuckp->hdr.no_chunks;  // number of directory entries
         	size_t dir_size = no_dir_entries * sizeof(DiskBucket::dirent_t); // byte-size of directory
         	
                pin_i* dir_handlep = new pin_i();
                // pin the page which contains the DiskBucket::offsetInBucket (bucket directory pointer)
                cout << " pin the page which contains the DiskBucket::offsetInBucket (bucket directory pointer)\n";
             	rc_t err = dir_handlep->pin(SystemManager::getDevVolInfo()->volumeID, dbuckp->hdr.id.id, sizeof(DiskBucket)-sizeof(DiskBucket::dirent_t*));
         	if(err) {
         		// then something went wrong
         		ostrstream error;
         		// Print Shore error message
         		error <<"Error in pin_i::pin  "<< err <<endl;
         		// throw an exeption
         		throw error.str();
         	}//end if
         	
         	DiskBucket::dirent_t chnkOffsInBucket; // chunk's begining offset
         	DiskBucket::dirent_t next_chnkOffsInBucket; // offset of the beginning of the next chunk        	         	
         	bool dir_in_one_page;
		list<pin_i*> directory; // because pin_i copy constructor is private, I have no choice
				       // than to use pin_i* in the list. The pointed-to objects must
				       // be freed up before the list is destructed!!
         	if(dir_handlep->length() >  dir_size) { // then the whole directory has been pinned
         		cout<<"OK the whole directory is in one page and has been pinned\n";
         		dir_in_one_page = true;
		}//end if
         	else { // some part of the directory resides at previous pages
         		// Pin the whole directory in the buffer pool and organize it in a list
         		// of pinned pages
         		cout<<"Pin the whole directory in the buffer pool and organize it in a list\n";
         		dir_in_one_page = false;
			directory.push_back(dir_handlep); // first page at the front of the list
			
			size_t offs_of_dir_end = sizeof(DiskBucket) - sizeof(DiskBucket::dirent_t*) - dir_size; //offset of end of directory
			
			while(directory.back()->start_byte() > offs_of_dir_end) { // there are more pages to pin
			        // find offset of next page to pin
			        size_t offset = directory.back()->start_byte() - 2; //subtract 2 bytes
			        		// from the beginning of this page in order to request
			        		// the previous one
                                pin_i* dir_pg_hdlp = new pin_i;
                                // pin the page which contains the DiskBucket::offsetInBucket (bucket directory pointer)
                                cout << " pin the page which contains the offset "<<offset<<endl;
                             	rc_t err = dir_pg_hdlp->pin(SystemManager::getDevVolInfo()->volumeID, dbuckp->hdr.id.id, offset);
                         	if(err) {
                         		// then something went wrong
                         		ostrstream error;
                         		// Print Shore error message
                         		error <<"Error in pin_i::pin  "<< err <<endl;
                         		// throw an exeption
                         		throw error.str();
                         	}
                         	//insert into list
                         	directory.push_back(dir_pg_hdlp);
			}// end while
         	}//end else
                	                	                	
              	cout <<"\n\nRead the bucket body";
              	pin_i newhdl; //used for pinning pages other than the first page (pointed-to by ReadBucket::dbackp)       		                		              	
              	for(int slot = 0; slot < dbuckp->hdr.no_chunks; slot++) {              		              	              	
              		if(dir_in_one_page) {
              		
                        	// go to the directory pointer
                        	DiskBucket::dirent_t* offInBuckp = reinterpret_cast<DiskBucket::dirent_t*>(dir_handlep->body() + dir_handlep->length() - sizeof(DiskBucket::dirent_t*));
                        	chnkOffsInBucket = offInBuckp[-slot-1];
                        	if(slot == dbuckp->hdr.no_chunks - 1) { // this is the last slot                        		
					next_chnkOffsInBucket = dbuckp->hdr.next_offset;
                        	}
                        	else { // this is not the last slot
                        		next_chnkOffsInBucket = offInBuckp[-(slot+1)-1];            		
                        	}                        	
              		}//end if
              		else { // dir pages in a list
              		
				// find the page which contains the directory slot
				size_t dir_slot_offs = sizeof(DiskBucket) - sizeof(DiskBucket::dirent_t*) + (-slot-1)*sizeof(DiskBucket::dirent_t);
				// iterate through directory pages
				list<pin_i*>::const_iterator list_i = directory.begin();
				while((*list_i)->start_byte() > dir_slot_offs) {
					list_i++;				
				}// end while
				size_t reloffs = dir_slot_offs - (*list_i)->start_byte(); //relative slot offset within pinned page
				DiskBucket::dirent_t* offsp = reinterpret_cast<DiskBucket::dirent_t*>((*list_i)->body() + reloffs);
				chnkOffsInBucket = *offsp;
                        	if(slot == dbuckp->hdr.no_chunks - 1) { // this is the last slot                        		
					next_chnkOffsInBucket = dbuckp->hdr.next_offset;
                        	}
                        	else { // this is not the last slot
                        		next_chnkOffsInBucket = offsp[-1]; // read next directory entry            		
                        	}                        									
              		}//end else
              		
              		//const char* p = buckp->body + buckp->offsetInBucket[-slot-1];	                		
              		const char* p = 0; // byte pointer
	       		bool chunk_in_fst_pg = false;
              		cout <<"chnkOffsInBucket = "<<chnkOffsInBucket<<endl;                       		
              		if(chnkOffsInBucket < fst_pg_length) {
              			//then the requested offset is in the page body pointed to by dbuckp
              			chunk_in_fst_pg = true;                     		
                		cout<<"requested offset is in 1st page: no pin occurs\n";
                		p = &(dbuckp->body[chnkOffsInBucket]); // set byte pointer
                      	}// end if
                      	else {// a new page has to pinned, containing the requested offset
                      		chunk_in_fst_pg = false;                     		
      				cout<<"requested chunk offset is in another page: need to be pinned\n";
                                cout << " pin the page which contains the "<<chnkOffsInBucket<<" byte offset"<<endl;
                                //bool eof = false;
                                //size_t sb = fst_pg_hdl.start_byte();
                                //size_t l =  fst_pg_hdl.length();
                                //size_t sbl = sb+l;
                                //while((fst_pg_hdl.start_byte()+fst_pg_hdl.length() < chnkOffsInBucket) && (!eof)){
                                //        err = fst_pg_hdl.next_bytes(eof);
                                 //       if(err) {
                                       		// then something went wrong
                                 //      		ostrstream error;
                                       		// Print Shore error message
                                 //      		error <<"Error in pin_i::pin  "<< err <<endl;
                                       		// throw an exeption
                                 //      		throw error.str();
                                       	//}end if
                                //} //end while
                                if(newhdl.pinned()){ // a page has been already pinned
                                	// check if we really need to pin another page
                                	if(newhdl.start_byte()+newhdl.length() < chnkOffsInBucket) {
                                		newhdl.unpin(); //unpin the old page
                                		// pin the new one
                                                rc_t err = newhdl.pin(SystemManager::getDevVolInfo()->volumeID, dbuckp->hdr.id.id, chnkOffsInBucket);
                                               	if(err) {
                                               		// then something went wrong
                                               		ostrstream error;
                                               		// Print Shore error message
                                               		error <<"Error in pin_i::pin  "<< err <<endl;
                                               		// throw an exeption
                                               		throw error.str();
                                               	} //end if
					}//end if
                               	}//end if
                               	else { // this is the first time we pin a new page
                                        rc_t err = newhdl.pin(SystemManager::getDevVolInfo()->volumeID, dbuckp->hdr.id.id, chnkOffsInBucket);
                                       	if(err) {
                                       		// then something went wrong
                                       		ostrstream error;
                                       		// Print Shore error message
                                       		error <<"Error in pin_i::pin  "<< err <<endl;
                                       		// throw an exeption
                                       		throw error.str();
                                       	} //end if                               	
                               	}//end else
                               	// calculate requested offset in pinned page
                               	size_t start_byte = newhdl.start_byte();
                               	size_t reloffs_in_pg = chnkOffsInBucket - newhdl.start_byte();
                               	p = newhdl.body() + reloffs_in_pg; // set byte pointer
                               	//size_t offs_in_pg = chnkOffsInBucket - fst_pg_hdl.start_byte();
                               	//p = fst_pg_hdl.body() + offs_in_pg; // set byte pointer
                      	}//end else
                      	
                      	//check if this is a "fragmented" chunk
                      	// ***NOTE*** It is assumed that a chunk's size is < 8116 bytes,
                      	// that is, a chunk of a bucket might be fragmented (or dispersed) to a maximum
                      	// of two consecutive SSM pages (each with a 8116 length)
                      	bool fragmentedChunk = false;
                      	if(chunk_in_fst_pg){// chunk offset is in 1st page
                      		if(chnkOffsInBucket < fst_pg_length && (next_chnkOffsInBucket-1) > fst_pg_length) {
					fragmentedChunk = true;
					cout << "this is a fragmented chunk\n";
					// copy whole chunk to heap space
					p = cpFragChunkToHeap(p,
							      chnkOffsInBucket,
							      next_chnkOffsInBucket-1,
							      fst_pg_hdl);
				}//end if
                      	}//end if
                      	else{ // chunk offset is in new page
                                if(chnkOffsInBucket < newhdl.length() && (next_chnkOffsInBucket-1) > newhdl.length()) {
					fragmentedChunk = true;
					cout << "this is a fragmented chunk\n";                      	
					p = cpFragChunkToHeap(p,
							      chnkOffsInBucket,
							      next_chnkOffsInBucket-1,
							      newhdl);
				}//end if							
                      	}//end else
                        	
               		const DiskChunkHeader* chp = reinterpret_cast<const DiskChunkHeader*>(p);
               		(chp->no_measures == 0)?cout<<"Directory Chunk => ":cout<<"Data Chunk => ";
               		cout << "no of entries = "<< chp->no_entries<<endl;
               		cout << "Chunk id = "<< chp->id<<endl;               		
               		cout << "Entries : "<<endl;
               		cout << "-------------------"<<endl;
                        		
               		p += sizeof(DiskChunkHeader); // move to 1st entry
               		if(chp->no_measures == 0) {//dir chunk
               			for(int i = 0; i<chp->no_entries; i++) {
               				DirChunk::Entry* ep = reinterpret_cast<DirChunk::Entry*>(p);
               				cout<<ep[i].index<<endl;			
               			}// end for	
               		}// end if
               		else { //data chunk
               		        const float* fp = reinterpret_cast<const float*>(p);
               			for(int i = 0; i<chp->no_entries; i++) {
               			        for(int j = 0; j<chp->no_measures; j++){						
               			                cout<<fp[j]<<",  ";
                        			
               			        }
               			        fp += chp->no_measures; // move to next entry
               			        cout << endl;
               				//DataChunk::Entry* ep = reinterpret_cast<DataChunk::Entry*>(p);
               				//cout<<ep[i].measures[0]<<",   "<<ep[i].measures[1]<<endl;			
               			}// end for			
               		} // end else
               		
			if(fragmentedChunk)
				delete [] p; //delete chunk from the heap
                      	cin.get();			
              	} //end for
              	
              	// free up dynamic memory
              	if(dir_in_one_page)
              		delete dir_handlep;
              	else {
			for(list<pin_i*>::iterator i = directory.begin(); i!=directory.end(); i++) {
				delete (*i);
			}//end for
		} // end else
         }// end else
} //end ReadBucket::readBody()

const char* ReadBucket::cpFragChunkToHeap(const char*startp, DiskBucket::dirent_t startoffs, DiskBucket::dirent_t endoffs, pin_i& curr_pg_hdl)
{
	// allocate heap space	
	char* bytep = new char[endoffs-startoffs+1];
	
	// copy chunk bytes from current page
	size_t thismuch = curr_pg_hdl.length() - startoffs;
	memcpy(bytep, startp, thismuch);
	
	// pin new page containing the 2nd part of the chunk
	pin_i nextpg_hdl;
        rc_t err = nextpg_hdl.pin(SystemManager::getDevVolInfo()->volumeID, curr_pg_hdl.serial_no(), endoffs);
       	if(err) {
       		// then something went wrong
       		ostrstream error;
       		// Print Shore error message
       		error <<"Error in pin_i::pin  "<< err <<endl;
       		// throw an exeption
       		throw error.str();
       	} //end if
       	
       	char* nextbytep = &bytep[thismuch];
	// copy chunk bytes from next page
	thismuch = endoffs - nextpg_hdl.start_byte() + 1;
	memcpy(nextbytep, nextpg_hdl.body(), thismuch);
	
	return bytep;
}//end ReadBucket::cpFragChunkToHeap()

