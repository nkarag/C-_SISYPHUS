/***************************************************************************
                          Exceptions.h  -  A simple hierachy of exceptions
                             -------------------
    begin                : Fri Sep 21 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <string>
#include <iostream>
#include <fstream>

/**
 * This class will serve as the base for deriving other
 * classes corresponing to more specific errors.
 * It will be the main class that will be "thrown" and "caught" in
 * exception handling.
 */
class GeneralError{
protected:
	/**
	 * Error message
	 */
	string msg;
public:
	/**
	 * Default constructor
	 */
	GeneralError(){}
	
	/**
	 * Constructor
	 */	
	GeneralError(string& s):msg(s){}

	/**
	 * Constructor
	 */			
	GeneralError(const char* s): msg(string(s)){}
	
	/**
	 * Destructor
	 */		
	~GeneralError(){}
	
	/**
	 * Returns the error message
	 */
	virtual const string& getErrorMessage() const {return msg;}
	
	/**
	 * Adds an error message to the existing one
	 */
	virtual GeneralError& operator+=(const GeneralError& error) {	
	        msg += "\n";
	        msg += error.getErrorMessage();
	        return *this;
	}
};

/**
 * For output to ostream objects (e.g. cout, cerr)
 */
ostream& operator<<(ostream& stream, const GeneralError& error);

/**
 * For output to file streams
 */
ofstream& operator<<(ofstream& stream, const GeneralError& error);

#endif //EXCEPTIONS_H