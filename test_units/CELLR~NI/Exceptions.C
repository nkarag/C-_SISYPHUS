/***************************************************************************
                          Exceptions.C  -  description
                             -------------------
    begin                : Sat Sep 22 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include <ctime>

#include "Exceptions.h"

ostream& operator<<(ostream& stream, const GeneralError& error)
{
        stream<<error.getErrorMessage()<<endl;
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
        stream<<error.getErrorMessage()<<endl;
        stream.flush(); //write now to file!
        return stream;
}
