#include <iostream>
#include <fstream>
#include <string>

//const char* NAME = "lala.txt";

//ofstream error_log_file_stream(NAME, ios::app);

class Test{
        static const char* const FILENAME = "filename.txt";
public:
        static ofstream errorStream;
        test(){

        }
};

ofstream Test::errorStream(Test::FILENAME, ios::app);

class ManagerA {
private:

      	ostream& outputLogStream;
    	ofstream& errorLogStream;

public:
	ManagerA(ostream& out = cout, ofstream& error = Test::errorStream)
		:outputLogStream(out), errorLogStream(error)
	{
                if(!Test::errorStream)
                                cerr<<"Error in opening file\n";
        }

	~ManagerA() {}

	void writeError(string& s){
	        errorLogStream<<s<<endl;
	}

};

class ManagerB {
private:

      	ostream& outputLogStream;
    	ofstream& errorLogStream;

public:
	ManagerB(ostream& out = cout, ofstream& error = Test::errorStream)
		:outputLogStream(out), errorLogStream(error)
	{}

	~ManagerB() {}

	void writeError(string& s){
	        errorLogStream<<s<<endl;
	}

};


main(){
        Test t;
        ManagerA a;
        ManagerB b;
        string s("Hello I am A");
        a.writeError(s);

        string s2("Hello I am B");
        b.writeError(s2);

/*        ofstream f("lala.txt", ios::app);
        if(!f) {
                cerr<<"error"<<endl;
                exit(1);
        }
        f<<"lalalalal"<<endl;
*/
}//main