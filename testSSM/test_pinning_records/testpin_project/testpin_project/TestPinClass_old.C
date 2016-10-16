/***************************************************************************
                          TestPinClass.C  -  description
                             -------------------
    begin                : Wed Jan 31 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "TestPinClass.h"
#include "SystemManager.h"
#include <iostream>

TestPinClass::TestPinClass()
{
	cout <<"Beginning Testing...\n";
	W_COERCE(ss_m::begin_xct());
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
 	W_COERCE(ss_m::commit_xct());
 	cout<<"end of testing..."<<endl;
}

void TestPinClass::test_pin()
{
	//W_COERCE(ss_m::begin_xct());
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
	
	vector<DirChunk>* dirp = new vector<DirChunk>;
	vector<DataChunk>* datap = new vector<DataChunk>;
	DirChunk dirc;
	DataChunk datac;
        				
	ChunkID cid1("subtree1");
	map<ChunkID, pair<size_t,size_t> > mymap;
	mymap[cid1] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 1 : 3 DirChunks, 6 DataChunks\n";
	cin.get(c);	
        	
	dirc.bucketid = recordID;
	dirc.index = 0;
	dirp->push_back(dirc);
        	
	dirc.index = 1;
	dirp->push_back(dirc);

	dirc.index = 2;
	dirp->push_back(dirc);
        	
	datac.measure = 1.0;
	datap->push_back(datac);
        	
	datac.measure = 1.1;
	datap->push_back(datac);
        	
	datac.measure = 1.2;	
	datap->push_back(datac);
        	
	datac.measure = 1.3;
	datap->push_back(datac);
        	
	datac.measure = 1.4;		
	datap->push_back(datac);
        	
	datac.measure = 1.5;	
	datap->push_back(datac);
        	
	ChunkID cid2("subtree2");
	mymap[cid2] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 2 : 4 DirChunks, 7 DataChunks\n";
	cout<<"This will be stored at dirp index: "<<mymap[cid2].first<<" and at datap index: "<<mymap[cid2].second<<endl;
	cin.get(c);
        	
	dirc.bucketid = recordID;
	dirc.index = 3;
	dirp->push_back(dirc);
        	
	dirc.index = 4;
	dirp->push_back(dirc);

	dirc.index = 5;
	dirp->push_back(dirc);
        	
	dirc.index = 6;
	dirp->push_back(dirc);
        	
        	
	datac.measure = 2.0;
	datap->push_back(datac);
        	
	datac.measure = 2.1;
	datap->push_back(datac);
        	
	datac.measure = 2.2;	
	datap->push_back(datac);
        	
	datac.measure = 2.3;
	datap->push_back(datac);
        	
	datac.measure = 2.4;		
	datap->push_back(datac);
        	
	datac.measure = 2.5;	
	datap->push_back(datac);

	datac.measure = 2.6;	
	datap->push_back(datac);

	ChunkID cid3("subtree3");
	mymap[cid3] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 3 : 10 DirChunks, 20 DataChunks\n";
	cout<<"This will be stored at dirp index: "<<mymap[cid3].first<<" and at datap index: "<<mymap[cid3].second<<endl;
	cin.get(c);
        	
	for(int i = 7; i<=160; i++){
		dirc.bucketid
		= recordID;
		dirc.index = i;
		dirp->push_back(dirc);	
	}

	for(int i = 1; i<=200; i++){
        	datac.measure = 3.0+ i*0.1;
        	datap->push_back(datac);
	}

	ChunkID cid4("subtree4");
	mymap[cid4] = make_pair(dirp->size(),datap->size());		
	cout<<"Create sub-tree 4 : 50 DirChunks, 100 DataChunks\n";
	cout<<"This will be stored at dirp index: "<<mymap[cid4].first<<" and at datap index: "<<mymap[cid4].second<<endl;
	cin.get(c);
        	
	for(int i = 160; i<=500; i++){
		dirc.bucketid = recordID;
		dirc.index = i;
		dirp->push_back(dirc);	
	}

	for(int i = 1; i<=100; i++){
        	datac.measure = 4.0+ i*0.1;
        	datap->push_back(datac);
	}
        						
	cout <<"Create BucketHeader...\n";
	cin.get(c);	
        	
	// create a byte vector containing the DirChunk & DataChunk vectors of the bucket
	// 1st the dirp
	vector<DirChunk> dirv;
	dirv.reserve(dirp->size()); //allocate contiguous space
	dirv = *dirp;
	vec_t body_data(&dirv, sizeof(dirv));
cout<<"	sizeof(dirv) = "<<sizeof(dirv)<<endl;
	size_t datachunks_offset_in_body = body_data.size(); // i.e. if char* p = record.body(),
                                                             // then (p + offset) point to the data chunk section
cout<<"datachunks_offset_in_body = "<<datachunks_offset_in_body<<endl;
	
	// 2nd the datap
	vector<DataChunk> datav;
	datav.reserve(datap->size()); //allocate contiguous space
	datav = *datap;
	vec_t dataVect(&datav, sizeof(datav));
cout<<"	sizeof(datav) = "<<sizeof(datav)<<endl;	
	
        body_data.put(dataVect);


/*********	        	
	// calculate dirp size in bytes            	
	//The following ALSO WORKS but the sizes are larger than thos with sizeof?!!?
	size_t dirp_size = 0;
	for(vector<DirChunk>::const_iterator i = dirp->begin(); i!=dirp->end(); i++){
		dirp_size += (*i).size_in_bytes();	
	} //end for
	vec_t body_data(dirp, dirp_size);
        	
	//alternatively calculate size by creating a copy
	//vector<DirChunk> dirp_cpy = *dirp;
	//vec_t body_data(dirp, sizeof(dirp_cpy));
	size_t datachunks_offset_in_body = body_data.size(); // i.e. if char* p = record.body(),
                                                             // then (p + offset) point to the data chunk section
cout<<"datachunks_offset_in_body = "<<datachunks_offset_in_body<<endl;

	// add the DataChunk vector to the byte vector (body of record)
        	
	// calculate datap size in bytes
	// The following ALSO WORKS but the sizes are larger than thos with sizeof?!!?
	//size_t datap_size = 0;
	//for(vector<DataChunk>::const_iterator i = datap->begin(); i!=datap->end(); i++){
	//	datap_size += (*i).size_in_bytes();	
	//} //end for	
	//body_data.put(datap, datap_size);
        	
	//alternatively calculate size by creating a copy
	//vector<DataChunk> datap_cpy = *datap;
	//body_data.put(datap, sizeof(datap_cpy));
	
	// one more alternative
	//size_t datap_size = datap->size() * sizeof(DataChunk);
	//body_data.put(datap, datap_size);
	
	//body_data.put(datap, sizeof((*datap)));
        vec_t dataVec(datap, dirp_size);
	body_data.put(datap, sizeof((*datap)));
	
**************/
        	
cout<<"Total body size: body_data.size() = "<<body_data.size()<<endl;
cout<<"Total body length: body_data.len(0) = "<<body_data.len(0)<<endl;
        										
	BucketHeader* bcktHdrp = new BucketHeader(recordID, datachunks_offset_in_body, mymap);	
        	
	cout<<"Instatiate Bucket...\n";
	cin.get(c);	
        	
	Bucket newbucket(bcktHdrp, dirp, datap);	

        		
	// 5. Store the bucket
	cout<<"Store Bucket...\n";
	cin.get(c);	
        	
	// create also bucket header  byte vector
/*	BucketHeader hdr_cpy =  *newbucket.gethdrp();
	vec_t hdr(&hdr_cpy, sizeof(hdr_cpy));
cout<<"sizeof(hdr_cpy) = " <<sizeof(hdr_cpy)<<endl;*/


	vec_t hdr(newbucket.gethdrp(), newbucket.gethdrp()->size_in_bytes());	
cout<<"newbucket.gethdrp()->size_in_bytes() = " <<newbucket.gethdrp()->size_in_bytes()<<endl;	

/*********
	vec_t hdr(newbucket.gethdrp(), sizeof(*(newbucket.gethdrp()))	);	
cout<<"sizeof(*(newbucket.gethdrp())) = " <<sizeof(*(newbucket.gethdrp()))<<endl;
***********/
	// create SSM record corresponding to a Bucket. Use the id created earlier.
        err = ss_m::create_rec_id(SystemManager::getDevVolInfo()->volumeID , shore_fid,
                         hdr,       /* header  */
                         8192,  /* length hint          */
                         body_data, /* body    */
                         recordID);      /* rec id           */
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in ss_m::create_rec_id "<< err <<endl;
		// throw an exeption
		throw error.str();
	}
 	//W_COERCE(ss_m::commit_xct());         	         	
	        	
	cout<<"Pin the Bucket...\n";
	cin.get(c);			
	
 	//W_COERCE(ss_m::begin_xct());	
    	// pin the CubeInfo record in the buffer pool
    	pin_i handle;
    	err = handle.pin(SystemManager::getDevVolInfo()->volumeID, recordID, 0);
	if(err) {
		// then something went wrong
		ostrstream error;
		// Print Shore error message
		error <<"Error in pin_i::pin in CatalogManager::getCubeInfo "<< err <<endl;
		// throw an exeption
		throw error.str();
	}

        // first get the header
        const BucketHeader& bckhdr = *((const BucketHeader*)handle.hdr());
        BucketHeader* bckhdrp = new BucketHeader(bckhdr);

        // Now get DirChunk vector
    	char* dir_bytep = new char(bckhdrp->dataChkVectByteOffset);
    	cout<<"bckhdrp->dataChkVectByteOffset = "<<bckhdrp->dataChkVectByteOffset<<endl;
    	memcpy(dir_bytep, handle.body(), bckhdrp->dataChkVectByteOffset);
    	vector<DirChunk>* dirVectp = (vector<DirChunk>*)dir_bytep;
    	
    	cout<<"handle.body_size() = "<<handle.body_size()<<endl;
    	cout<<"handle.length() = "<<handle.length()<<endl;
    	cout<<"handle.is_small() = "<<handle.is_small()	<<endl;
    	// Finally, get DataChunk vector
    	char* data_bytep = new char(handle.body_size() - bckhdrp->dataChkVectByteOffset);
    	memcpy( data_bytep,
    	        handle.body()+bckhdrp->dataChkVectByteOffset,
    	        handle.body_size() - bckhdrp->dataChkVectByteOffset);
    	vector<DataChunk>* dataVectp = (vector<DataChunk>*)data_bytep;

    	// unpin the record   							
        handle.unpin();
    	
	cout<<"Read the Bucket contents...\n";
	cin.get(c);
	
	Bucket buck(bckhdrp, dirVectp, dataVectp); // new Bucket instance
	size_t beginIndexInDirVect;
	size_t beginIndexInDataVect;
	size_t endIndexInDirVect;
	size_t endIndexInDataVect;
	for(map<ChunkID, pair<size_t,size_t> >::const_iterator map_i = buck.gethdrp()->subtree2index.begin();
	        map_i != buck.gethdrp()->subtree2index.end(); ++map_i) { //for each sub-tree
	
	        beginIndexInDirVect = map_i->second.first;
	        beginIndexInDataVect = map_i->second.second;
	        string treeName = map_i->first.getcid();
	
                map<ChunkID, pair<size_t,size_t> >::const_iterator next_tree_i = map_i;
                next_tree_i++;
                if(next_tree_i != buck.gethdrp()->subtree2index.end()) {
        	        endIndexInDirVect = next_tree_i->second.first-1;
        	        endIndexInDataVect = next_tree_i->second.second-1;                                	
                }
                else {
        	        endIndexInDirVect = buck.getdirp()->size()-1;
        	        endIndexInDataVect = buck.getdatap()->size()-1;
                }
	
	        cout<<"DirChunks of sub-tree: "<<treeName<<endl;
	        for(size_t i = beginIndexInDirVect; i<=endIndexInDirVect; i++){
	        	vector<DirChunk>& dir = *(buck.getdirp());
        	        cout<<"\t"<<dir[i].index<<endl;	
	        }
	        cout<<"DataChunks of sub-tree: "<<treeName<<endl;
	        for(size_t i = beginIndexInDataVect; i<=endIndexInDataVect; i++){
	        	vector<DataChunk>& data = *(buck.getdatap());
        	        cout<<"\t"<<data[i].measure<<endl;	
	        }
		cin.get(c);
	} //end for			
        //W_COERCE(ss_m::commit_xct());

        //delete dirp;
        //delete datap;
        //delete bcktHdrp;
        //delete bckhdrp;
        //delete dirVectp;
        //delete dataVectp;

	// Now test updating a record!
    	cout << "now lets try to update a subtree!\n";    	
} // end of TestPinClass::test_pin()


