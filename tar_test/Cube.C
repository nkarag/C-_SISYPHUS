/***************************************************************************
                          Cube.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "Cube.h"
#include <string>
#include <stdlib.h>
#include <iostream>
#include <cmath>

const cubeID_t CubeInfo::null_id = -1000; // the null cube id
				       // **NOTE** null_id must be != from CatalogManager::MAXKEY !!!
const int FIELD_SIZE = 18;

//------------------------------- class CubeInfo ------------------------------------------

CubeInfo::CubeInfo() : rootChnkIndex(0)
{
	cbID = CubeInfo::null_id;
}

CubeInfo::CubeInfo(const string& nm): rootChnkIndex(0)
{
	name = nm;
	cbID = CubeInfo::null_id;
}

CubeInfo::~CubeInfo() {

}
// The following function was implemented by Loukas Sinos, September 2000.
void CubeInfo::Get_dimension_information(const string& filename)
{
	ifstream ifstrm(filename.c_str());
	if (!ifstrm){
	    	throw "CubeInfo::Get_dimension_information ==> Creating ifstream object failed. Cannot get information for dimensions";
    	}
    	string buffer; //stores a word from the input file
    	int child_code; //stores first and last child order code
	num_of_dimensions = 0;

	Dimension dim; //object to store temporarily the dimension that we get from the input file
	Dimension_Level level; //object to store temporarily the level that we get from the input file
	Level_Member member; //object to store temporarily the member that we get from the input file

	ifstrm >> buffer; //expected word "DIMENSION"
	if (buffer != "DIMENSION")
		throw "CubeInfo::Get_dimension_information ==> Wrong syntax in input file. Cannot get information for dimensions";

	while (buffer != "ENDOFDIMENSIONS")
	{
		ifstrm >> buffer; //name of dimension
		dim.set_name(buffer);
		dim.set_num_of_levels(0);

		ifstrm >> buffer; //expected word "LEVEL"
		if (buffer != "LEVEL")
			throw "CubeInfo::Get_dimension_information ==> Wrong syntax in input file. Cannot get information for dimensions";

		while ((buffer != "DIMENSION") && (buffer != "ENDOFDIMENSIONS"))
		{
			ifstrm >> buffer; //name of level
			level.set_name(buffer);
			level.set_level_number(dim.get_num_of_levels());
			level.set_num_of_members(0);

			ifstrm >> buffer; //word "MEMBER"
			ifstrm >> buffer; //name of first member
			while ((buffer != "LEVEL") && (buffer != "DIMENSION") && (buffer != "ENDOFDIMENSIONS"))
			{
				member.set_name(buffer);
				member.set_order_code(level.get_num_of_members());
				ifstrm >> child_code;
				member.set_first_child_order_code(child_code);
				ifstrm >> child_code;
				member.set_last_child_order_code(child_code);
				//add member to members' vector and increase members' counter
				level.increase_num_of_members();
				level.get_vectMember().push_back(member);
				ifstrm >> buffer; //one of the words "MEMBER", "LEVEL", "DIMENSION", "ENDOFDIMENSIONS"
				if (buffer == "MEMBER")
				{
					ifstrm >> buffer; //name of member
				}
			}
			//add level to levels' vector and increase levels' counter
			dim.increase_num_of_levels();
			dim.get_vectLevel().push_back(level);
			level.get_vectMember().clear();
		}
		//add dimension to dimensions' vector and increase dimensions' counter
		vectDim.push_back(dim);
		dim.get_vectLevel().clear();
		num_of_dimensions++;
	}

	ifstrm.close(); //close file stream

	// Insert pseudo levels
	(*this).Insert_pseudo_levels();
	// Now, all dimensions have the same number of levels

	// Create member codes for all members of all dimensions
	(*this).Create_member_codes();
	// Now, we have all the stored information needed for the dimensions of the cube
	
// Nikos:	
	// The max chunking depth of this cube can be derived from the number of levels
	// of the dimensions (which now is the same for all of them).
	maxDepth = vectDim[0].get_num_of_levels() - 1;
        #ifdef DEBUGGING
              cerr << "MAXIMUM CHUNKING DEPTH = "<<maxDepth<<endl;
        #endif	
}

void CubeInfo::getFactInfo(const string& filename)
{
 	factNames.push_back(string("Sales"));
 	factNames.push_back(string("Cost")); 	
 	numFacts = factNames.size();
 	assert(numFacts == 2);
}

void CubeInfo::Insert_pseudo_levels()
{
    cout << "Entering Insert_pseudo_levels function..." << endl;
    //first find the maximum dimension depth (in order to do padding)
    int max_num_of_levels = 0;
    int max;
    for (vector<Dimension>::iterator iter_dim = vectDim.begin(); iter_dim != vectDim.end(); ++iter_dim)
    {
   		if (((*iter_dim).get_num_of_levels()) < 2)
   			throw "CubeInfo::Insert_pseudo_levels() ==> Cannot continue the insertion of pseudo levels. A dimension has only one level.";
   		if (((*iter_dim).get_num_of_levels()) > max_num_of_levels)
   			max_num_of_levels = (*iter_dim).get_num_of_levels();
	}
   	//cout << "Max num of levels: " << max_num_of_levels << endl;
   	max = max_num_of_levels;
	//For each dimension with number of levels < max number of levels
	//add levels (before the last level) to reach the maximum
	// ************************ ATTENTION *************************
	// We assume that each dimension has at least two levels !!!!!!
	// ************************************************************
	char str[5];

	vector<Dimension_Level>::iterator iter_lev, iter_lev2, iter_lev4; //iterators for levels' vector
	vector<Level_Member>::iterator iter_mem; //iterator for members' vector


    for (vector<Dimension>::iterator iter_dim = vectDim.begin(); iter_dim != vectDim.end(); ++iter_dim)
    {
   		cout << "Working for dimension <" << (*iter_dim).get_name() << "> with " << (*iter_dim).get_num_of_levels() << " levels..." << endl;
   		//cout << "Max num of levels: " << max_num_of_levels << endl;
   		if (((*iter_dim).get_num_of_levels()) < max_num_of_levels)
   		{
   			int levels_to_add = max_num_of_levels - (*iter_dim).get_num_of_levels();
   			cout << levels_to_add << " levels must be added" << endl;

			//go to the level before the last level
   			iter_lev2 = (*iter_dim).get_vectLevel().begin();
			for (int n = 0; n<(((*iter_dim).get_num_of_levels()) - 2); n++)
				iter_lev2++;

  			//pointer to the last level
   			iter_lev = iter_lev2 + 1;
   			//cout << "The last level is <" << (*iter_lev).get_name() << ">" << endl;
   			//change its number to the new last level number. Last level must remain last
   			(*iter_lev).set_level_number(max_num_of_levels - 1);

   			//cout << "Before the last level there is level <" << (*iter_lev2).get_name() << ">" << endl;
   			iter_mem = (*iter_lev2).get_vectMember().begin(); //points to the first member of level before last
   			//cout << "	with first member: " << (*iter_mem).get_name() << endl;
    		//add new level objects to the vector of levels
  			for (int i = 0; i<levels_to_add; i++)
   			{
				Dimension_Level level; //object to temporarily store new level
				sprintf(str,"Pseudo-level %d", (levels_to_add - i));
				cout << "Creating " << str << "..." << endl;
				level.set_name(str);
				level.set_level_number(max_num_of_levels - 2 - i);
				level.set_num_of_members((*iter_lev2).get_num_of_members());
   				for (int j = 0; j<(*iter_lev2).get_num_of_members(); j++)
   				{
					Level_Member member; //object to temporarily store new member
					sprintf(str,"Pseudo-member %d", (j+1));
					cout << "Creating " << str << "..." << endl;
					member.set_name(str);
					//member.set_order_code(-1-j);
					member.set_order_code(-1);
					if (i == 0) //only for pseudo level before the last level
					{
						member.set_first_child_order_code((*iter_mem).get_first_child_order_code());
						member.set_last_child_order_code((*iter_mem).get_last_child_order_code());
						iter_mem++;
					}
					level.get_vectMember().push_back(member);
				}
				//go to the place where the new level must be inserted
				//the levels are inserted in the following order: we insert "pseudo_level3" first,
				//then before "pseudo_level3" we insert "pseudo_level2" etc.
				iter_lev4 = (*iter_dim).get_vectLevel().begin();
				for (int k=0; k<(((*iter_dim).get_num_of_levels()) - i - 1); k++)
					iter_lev4++;
   				cout << "Inserting pseudo level before level <" << (*iter_lev4).get_name() << "> ..." << endl;
				(*iter_dim).get_vectLevel().insert(iter_lev4, level);
				(*iter_dim).increase_num_of_levels();
				cout << "Pseudo_level" << (levels_to_add - i) << " was successfully created!!!" << endl;
				level.get_vectMember().clear();
				/*for (vector<Dimension_Level>::iterator iter_lev3 = (*iter_dim).get_vectLevel().begin();
			 		 iter_lev3 != (*iter_dim).get_vectLevel().end();
			 		 ++iter_lev3)
					cout << "LEVEL:" << (*iter_lev3).get_name() << " - LEVEL NUMBER: " << (*iter_lev3).get_level_number() << endl;*/
			}

			//update the old level before the last and all the new levels except the last
			//with the right values for attributes "first_child_order_code" and "last_child_order_code"
			//of their members
   			iter_lev2 = (*iter_dim).get_vectLevel().begin(); //pointer to first level
   			//go to the level before the first pseudo level
			for (int m = 0; m<(((*iter_dim).get_num_of_levels()) - levels_to_add - 2); m++)
				iter_lev2++;
			for (int i = 0; i<levels_to_add; i++)
			{
				cout << "Changing children codes for members of level <" << (*iter_lev2).get_name() << "> ..." << endl;
				iter_mem = (*iter_lev2).get_vectMember().begin();
				for (int j = 0; j<(*iter_lev2).get_num_of_members(); j++)
				{
					cout << " - Changing children codes for member <" << (*iter_mem).get_name() << "> ..." << endl;
					//(*iter_mem).set_first_child_order_code(-j-1);
					//(*iter_mem).set_last_child_order_code(-j-1);
					(*iter_mem).set_first_child_order_code(-1);
					(*iter_mem).set_last_child_order_code(-1);					
					iter_mem++;
				}
				iter_lev2++;
			}
		}
   		cout << "End of work for dimension <" << (*iter_dim).get_name() << ">" << endl;
	}
	/*for (vector<Dimension>::iterator iter_dimx = vectDim.begin(); iter_dimx != vectDim.end(); ++iter_dimx)
	{
		cout << "<" << (*iter_dimx).get_name() << "> with " << (*iter_dimx).get_num_of_levels() << " levels" << endl;
		cout << "DIMENSION:" << (*iter_dimx).get_name() << " - LEVELS: " << (*iter_dimx).get_num_of_levels() << endl;
		for (vector<Dimension_Level>::iterator iter_levx = (*iter_dimx).get_vectLevel().begin();
			 iter_levx != (*iter_dimx).get_vectLevel().end();
			 ++iter_levx)
		{
			cout << "LEVEL:" << (*iter_levx).get_name() << " - LEVEL NUMBER: " << (*iter_levx).get_level_number();
			cout << " - MEMBERS: " << (*iter_levx).get_num_of_members() << endl;
			for (vector<Level_Member>::iterator iter_memx = (*iter_levx).get_vectMember().begin();
				 iter_memx != (*iter_levx).get_vectMember().end();
				 ++iter_memx)
			{
				cout << "MEMBER:" << (*iter_memx).get_name() << " - ORDER CODE: " << (*iter_memx).get_order_code();
				cout << " - MCODE:" << (*iter_memx).get_member_code() << " - PMCODE: " << (*iter_memx).get_parent_member_code();
				cout << " - FCOCODE:" << (*iter_memx).get_first_child_order_code();
				cout << " - LCOCODE: " << (*iter_memx).get_last_child_order_code() << endl;
			}
		}
	}*/
}

void CubeInfo::Create_member_codes()
{
    vector<Dimension_Level>::iterator iter_lev2;
    vector<Level_Member>::iterator iter_mem2;
    char str[5];

    //for each dimension of the cube
    for (vector<Dimension>::iterator iter_dim = vectDim.begin(); iter_dim != vectDim.end(); ++iter_dim)
    {
		//for each level of the dimension
		for (vector<Dimension_Level>::iterator iter_lev = (*iter_dim).get_vectLevel().begin();
			 iter_lev != (*iter_dim).get_vectLevel().end();
			 ++iter_lev)
		{
			iter_lev2 = iter_lev + 1; //points to the next level
			if (iter_lev2 != (*iter_dim).get_vectLevel().end())
				iter_mem2 = (*iter_lev2).get_vectMember().begin(); //points to the first member of the next level

			//for each member of the level
			for (vector<Level_Member>::iterator iter_mem = (*iter_lev).get_vectMember().begin();
				 iter_mem != (*iter_lev).get_vectMember().end();
				 ++iter_mem)
			{
				if ((*iter_lev).get_level_number() == 0) //if this is the top level
				{
					(*iter_mem).set_parent_member_code(""); //member has no parent
					sprintf(str,"%d", (*iter_mem).get_order_code());
					(*iter_mem).set_member_code(str);
				}
				else
				{
					sprintf(str,"%d", (*iter_mem).get_order_code());
					(*iter_mem).set_member_code((*iter_mem).get_parent_member_code() + "." + str);
				}
				//update the parent_member_code of each child of the member
				//if the member belongs to a level except the last last
				if ((*iter_lev).get_level_number() < ((*iter_dim).get_num_of_levels() - 1)) //if this is not the last level
				{
					for (int i=0; i<=((*iter_mem).get_last_child_order_code() - (*iter_mem).get_first_child_order_code()); i++)
					{
						(*iter_mem2).set_parent_member_code((*iter_mem).get_member_code());
						iter_mem2++;
					}
				}
			}
		}
	}
}

void CubeInfo::Print_children(vector<Dimension_Level>::iterator iter, const int levels, const int first, const int last)
{
	int length, length1, first_order_code;
	string dash = "------------------";
	string blank = "                  ";
	vector<Dimension_Level>::iterator iter_help;

	vector<Level_Member>::iterator iter_mem = (*iter).get_vectMember().begin();
	// Move to the first child
	if (first >= 0) //for normal members
		first_order_code = first;
	else //for pseudo members
		first_order_code = -first-1;
	for (int m=0; m<first_order_code; m++)
		iter_mem++;
	bool first_child = true;

	for (int i = 0; i <= abs(last)-abs(first); i++)
	{
		if (first_child == false)
		{
			for (int j = 0; j < (*iter).get_level_number(); j++)
			{
				cout << "|";
				cout << blank;
			}
		}
		else
			first_child = false;
		cout << "|";
		//cout.width(FIELD_SIZE);
		//cout.setf(ios::left, ios::adjustfield);
		length = (*iter_mem).get_name().size();
		length1 = (*iter_mem).get_member_code().size();
		if (length <= FIELD_SIZE-length1-1)
		{
			cout << (*iter_mem).get_name() + ":" + (*iter_mem).get_member_code();
			for (int k = 0; k < (FIELD_SIZE-length-length1-1); k++)
				cout << " ";
		}
		else
		{
			string help = (*iter_mem).get_name();
			for (int k = 0; k < (FIELD_SIZE-length1-1); k++)
				cout << help[k];
			cout << ":" << (*iter_mem).get_member_code();
		}
		//cout << (*iter_mem).get_member_code() + ":" +(*iter_mem).get_name();
		//if we are not at the last level
		if ((*iter).get_level_number() < (levels - 1))
		{
			iter_help = iter + 1;
			Print_children(iter_help, levels,
						   (*iter_mem).get_first_child_order_code(), (*iter_mem).get_last_child_order_code());
		}
		else
		{
			cout << "|";
			cout << endl;
			cout << "|";
			for (int i=0; i<levels; i++)
			{
				cout << dash;
				cout << "|";
			}
			cout << endl;
		}
		iter_mem++;
	}
}

void CubeInfo::Show_dimensions()
{
	string dash = "------------------";
	int length, length1;

	//for each dimension of the cube
	for (vector<Dimension>::iterator iter_dim = vectDim.begin(); iter_dim != vectDim.end(); ++iter_dim)
	{
		//make heading for dimension
		cout << endl;
		cout << "MEMBERS OF THE DIMENSION \"" << (*iter_dim).get_name() << "\"" << endl;

		//make headings for the columns of levels
		cout << "|";
		for (int i=0; i<(*iter_dim).get_num_of_levels(); i++)
		{
			cout << dash;
			cout << "|";
		}
		cout << endl;
		for (vector<Dimension_Level>::iterator iter_lev = (*iter_dim).get_vectLevel().begin();
			iter_lev != (*iter_dim).get_vectLevel().end();
			++iter_lev)
		{
			//for each level
			cout << "|";
			//cout.width(FIELD_SIZE);
			//cout.setf(ios::left, ios::adjustfield/*, ios::uppercase*/);
			length = (*iter_lev).get_name().size();
			//cout << setw(FIELD_SIZE) << (*iter_lev).get_name();
			cout << (*iter_lev).get_name();
			for (int k = 0; k < (FIELD_SIZE-length); k++)
				cout << " ";
		}
		cout << "|" << endl;
		cout << "|";
		for (int i=0; i<(*iter_dim).get_num_of_levels(); i++)
		{
			cout << dash;
			cout << "|";
		}
		cout << endl;

		//start printing members
		vector<Dimension_Level>::iterator iter_lev = (*iter_dim).get_vectLevel().begin(); //points to first level
		for (vector<Level_Member>::iterator iter_mem = (*iter_lev).get_vectMember().begin();
			 iter_mem != (*iter_lev).get_vectMember().end();
			 ++iter_mem)
		{
			cout << "|";
			//cout.width(FIELD_SIZE);
			//cout.setf(ios::left, ios::adjustfield);
			length = (*iter_mem).get_name().size();
			length1 = (*iter_mem).get_member_code().size();
			if (length <= FIELD_SIZE-length1-1)
			{
				cout << (*iter_mem).get_name() + ":" + (*iter_mem).get_member_code();
				for (int k = 0; k < (FIELD_SIZE-length-length1-1); k++)
					cout << " ";
			}
			else
			{
				string help = (*iter_mem).get_name();
				for (int k = 0; k < (FIELD_SIZE-length1-1); k++)
					cout << help[k];
				cout << ":" << (*iter_mem).get_member_code();
			}
			Print_children((iter_lev + 1), (*iter_dim).get_num_of_levels(),
						   (*iter_mem).get_first_child_order_code(), (*iter_mem).get_last_child_order_code());
		}


	}
}
//------------------------------- endof CubeInfo --------------------------------------

//------------------------------- class Cube ------------------------------------------
Cube::Cube()
{
}

Cube::~Cube()
{
	delete info;
}
//-------------------------------- end of Cube ------------------------------------------

//-------------------------------- class Dimension_Level --------------------------------
unsigned int Dimension_Level::get_num_of_sibling_members(string parentMbrCode)
{
	unsigned int mbCounter = 0; // member counter
	for(	vector<Level_Member>::iterator mb_iter = vectMember.begin();
		mb_iter!=vectMember.end();
		++mb_iter) {
         	if((*mb_iter).get_parent_member_code() == parentMbrCode)
			mbCounter++;
	}		
	return mbCounter;	    	
}

vector<Level_Member>::const_iterator  Dimension_Level::getMbrByMemberCode(string& mbCode)
{
	for(	vector<Level_Member>::const_iterator mb_iter = vectMember.begin();
		mb_iter!=vectMember.end();
		++mb_iter) {
         	if((*mb_iter).get_member_code() == mbCode)
			return mb_iter;
	}
	return 	vectMember.end(); //not found	
}
//-------------------------------- end of Dimension_Level -------------------------------
