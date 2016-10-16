#include <iostream>
#include <fstream>
#include <ctime>
#include <string>

/**
 * This class will serve as the base for deriving other
 * classes corresponing to more specific errors.
 * It will be the main class that will be "thrown" and "caught" in
 * exception handling.
 */
class GeneralError{
protected:
	string msg;
	/**
	 * Name of the file where the error occured
	 */
	 string fileName;
	
	 /**
	  * Line in the file where the eror occured
	  */
	  int line;
	
public:
	GeneralError(){}
	GeneralError(string& s):msg(s){}
	GeneralError(const char* s): msg(string(s)){}
	/**
	 * Constructor
	 */			
	GeneralError(const char* f, int l, const char* s): msg(string(s)), fileName(string(f)), line(l){}
	
	
	~GeneralError(){}
	
	virtual const string& getErrorMessage() const {return msg;}
	
	/**
	 * Returns the file name
	 */
	virtual const string& getfileName() const {return fileName;}
	
	/**
	 * Returns the line number
	 */
	virtual const int getline() const {return line;}	
		
	virtual GeneralError& operator+=(const GeneralError& error) {	
	        msg += "\n";
	        msg += error.getErrorMessage();
	        return *this;
	}
};

ostream& operator<<(ostream& stream, const GeneralError& error);
ofstream& operator<<(ofstream& stream, const GeneralError& error);
	

void functionA();
void functionB();

main()
{
        ofstream error_log("error.log", ios::app);

        try{
                functionA();
        }
        catch(GeneralError& error){
                cout<<error<<endl;
                cerr<<error<<endl;
                error_log<<error<<endl;
        }
}

void functionA()
{
        try{
              functionB();
        }
        catch(GeneralError& error){
                string s("Hi I'm A!");
                GeneralError e(s);
                error += e;
                throw error;
        }
}

void functionB()
{
        throw GeneralError(__FILE__, __LINE__, "Hi, I'm B!");
}

ostream& operator<<(ostream& stream, const GeneralError& error)
{
        stream << "(File: " << error.getfileName() << ", line: " << error.getline() << "): " << error.getErrorMessage()<<endl;
        return stream;
}

ofstream& operator<<(ofstream& stream, const GeneralError& error)
{
        time_t* timep = new time_t;
        time(timep);
        stream<<endl;
        stream<<"============================================"<<endl;
        stream<<"Error Log"<<endl;
        stream<<ctime(timep);
        stream<<"============================================"<<endl;
        stream<<"(File: " << error.getfileName() << ", line: " << error.getline() << "): " << error.getErrorMessage()<<endl;
        stream.flush(); //write now to file!
        return stream;
}