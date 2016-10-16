#include <string>
//#include <vector>
//#include <map>
#include <iostream>

class Chunk{
public:
        static const int NULL_DEPTH = -1;
        static const int MIN_DEPTH = 0;
};

class ChunkID{
        string cid;
public:
        ChunkID(string& s): cid(s){}

        const int getChunkDepth(int localdepth = Chunk::NULL_DEPTH) const;		
};

main()
{
        cout<<"Give me chunk id: ";
        string in;
        cin>>in;

  //      cout<<"Give me local depth: ";
    //    int ld;
      //  cin>>ld;


        ChunkID id(in);
        int depth = id.getChunkDepth();//ld);
        if(depth == -1){
                cerr<<"ERROR!"<<endl;
                exit(1);
        }
         cout<<"the global depth is: "<<depth<<endl;

}

const int ChunkID::getChunkDepth(int localdepth) const
{
	if(cid.empty())
		return -1;

	if(cid == "root")
		return Chunk::MIN_DEPTH;

	int depth = Chunk::MIN_DEPTH + 1;
	for (int i = 0; i<cid.length(); i++)
	{
		if (cid[i] == '.')
			depth++;
	}
	depth = (localdepth == Chunk::NULL_DEPTH)? depth : depth - localdepth;
	
	if(localdepth != Chunk::NULL_DEPTH && depth < 1)
	        return -1; //an invalid local depth must have been given
	
	return depth;
}//end of ChunkID::getChunkDepth()
