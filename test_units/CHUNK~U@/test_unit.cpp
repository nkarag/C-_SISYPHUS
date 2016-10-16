#include <string>
#include <vector>
#include <iostream>
#include <strstream>

class LevelMember {
public:
        static const int PSEUDO_CODE = -1;
};
struct Coordinates {
	int numCoords; //number of coordinates
	vector<int> cVect;  // a coordinate is represented by a signed integer, because
	                   // there is also the case of Level_Member::PSEUDO_CODE, who
	                   // can have a negative value (e.g., -1)
	/**
	 * default constructor
	 */
	Coordinates() : numCoords(0), cVect(){}

	/**
	 * constructor
	 */
	Coordinates(int nc, vector<int>& cds) : numCoords(nc), cVect(cds){}

	/**
	 * copy constructor
	 */
	Coordinates(Coordinates const& c) : numCoords(c.numCoords), cVect(c.cVect) {}

	/**
	 * It fills in another Coordinates struct the coordinates that this->cVect contains
	 * excluding the pseudo coordinates (i.e. coordinates that correspond to pseudo-levels)
	 *
	 * @param coords        the initially empty coord struct that will be filled with the coordinate values
	 */
        //void excludePseudoCoords(Coordinates & coords)const;

        /**
         * returns true if vector with coordinates is empty
         */
        bool empty() const {return cVect.empty();}
};//end struct Coordinates


void coords2domain(const Coordinates& coords, string& domain);

main(){
        //create coordinates
        int numDims = 10;
        vector<int> v(numDims,50);
        v[1] = v[9] = LevelMember::PSEUDO_CODE;
        Coordinates coords(numDims, v);

        //Coordinates coords;
        string dom;
        coords2domain(coords, dom);
        cout<<dom<<endl;
}//main

void coords2domain(const Coordinates& coords, string& domain){
        if(coords.empty()){
                domain = string("");
                return;
        }

        //create an output string stream
        ostrstream dom;
        //for each coordinate
        for(int cindex = 0; cindex < coords.numCoords; cindex++){
                //if this is not the last coordinate
                if(cindex < coords.numCoords - 1)
                        dom<<coords.cVect[cindex]<<"|";
                else
                        dom<<coords.cVect[cindex];
        }//end for
        dom<<ends;
        domain = string(dom.str());
}//coords2domain
