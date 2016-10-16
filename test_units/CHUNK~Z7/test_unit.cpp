#include <iostream>
#include <string>

class ChunkID{
        string cid;
public:
        ChunkID(string& s): cid(s){}
        const int getNumDomains() const;

};

main()
{
        cout<<"Give me chunk id: ";
        string in;
        cin>>in;

        ChunkID id(in);
        int nodoms = id.getNumDomains();
        if(nodoms == -1){
                cerr<<"ERROR: empty id!"<<endl;
                exit(1);
        }
        if(!nodoms){
                cout<<"root chunk encountered"<<endl;
                exit(1);
        }
        cout<<"the number of domains is: "<<nodoms<<endl;

}

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
