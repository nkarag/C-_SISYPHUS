/***************************************************************************
                          DataVector.C  -  description
                             -------------------
    begin                : Tue Mar 19 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#include "DataVector.h"

#include <basics.h> // !!! needed before vec_t.h
#include <vec_t.h> //  SSM vec_t header file

DataVector::DataVector(){
	ssmvectp = new vec_t();
}
	
DataVector::DataVector(const void* p, size_t l){
	ssmvectp = new vec_t(p,l);
}
	
DataVector::DataVector(const DataVector& v, size_t offset, size_t limit){
	ssmvectp = new vec_t(v.dataVector2vec_t(),offset,limit);
}

DataVector::DataVector(const DataVector& v){
	ssmvectp = new vec_t(v.dataVector2vec_t(), 0, v.size());
}

DataVector::~DataVector(){
	if(ssmvectp) delete ssmvectp;
}
	
DataVector & DataVector::operator=(const DataVector & other)
{
	if(this != &other) {
		// deallocate current data
		delete ssmvectp;

		// duplicate other's data
		ssmvectp = new vec_t(other.dataVector2vec_t(), 0, other.size());		
	}
	return (*this);
}
	
void DataVector::split(size_t l1, DataVector& v1, DataVector& v2){
	// reference to the hidden vec_t in v1
	vec_t& vec1 = const_cast<vec_t&>(v1.dataVector2vec_t());
	// reference to the hidden vec_t in v2	
	vec_t& vec2 = const_cast<vec_t&>(v2.dataVector2vec_t());
	// call the vec_t counterpart
	ssmvectp->split(l1, vec1, vec2);
}

DataVector& DataVector::put(const DataVector& v){
	ssmvectp->put(v.dataVector2vec_t());
	return (*this);
}


DataVector& DataVector::put(const void* p, size_t l){
	ssmvectp->put(p,l);
	return (*this);
}

DataVector& DataVector::put(const DataVector& v, size_t offset, size_t nbytes){
	ssmvectp->put(v.dataVector2vec_t(),offset,nbytes);
	return (*this);
}

DataVector& DataVector::reset(){
	ssmvectp->reset();
	return (*this);
}	

void DataVector::init(){
	ssmvectp->init();
}

DataVector& DataVector::set(const DataVector& v1, const DataVector& v2){
	ssmvectp->set(v1.dataVector2vec_t(),v2.dataVector2vec_t());
	return (*this);
}

DataVector& DataVector::set(const DataVector& v){
	ssmvectp->set(v.dataVector2vec_t());
	return (*this);
}

DataVector& DataVector::set(const void* p, size_t l){
	ssmvectp->set(p,l);
	return (*this);
}

DataVector& DataVector::set(const DataVector& v, size_t offset, size_t limit){
	ssmvectp->set(v.dataVector2vec_t(),offset,limit);
	return (*this);
}

size_t DataVector::size() const{
	return ssmvectp->size();
}

int DataVector::count() const{
	return ssmvectp->count();
};	

size_t DataVector::copy_to(void* p, size_t limit = 0x7fffffff) const{
	return ssmvectp->copy_to(p,limit);
}

const DataVector& DataVector::copy_from(
    const void* p,
    size_t limit,  // number of bytes to copy
    size_t offset = 0) const   // offset tells where
                                //in the vector to begin to copy
{
	ssmvectp->copy_from(p,limit,offset);
	return (*this);
}

DataVector& DataVector::copy_from(const DataVector& v)
{
	ssmvectp->copy_from(v.dataVector2vec_t());
	return (*this);
}


DataVector& DataVector::copy_from(
    const DataVector& v,
    size_t offset,          // offset in v
    size_t limit,           // # bytes
    size_t myoffset = 0)   // offset in this
{
	ssmvectp->copy_from(v.dataVector2vec_t(),offset,limit,myoffset);
	return (*this);
}


const unsigned char*  DataVector::ptr(int index) const {
	return ssmvectp->ptr(index);
}

size_t      DataVector::len(int index) const{
	return ssmvectp->len(index);
}

const vec_t& DataVector::dataVector2vec_t() const{
	return *ssmvectp;	
}

