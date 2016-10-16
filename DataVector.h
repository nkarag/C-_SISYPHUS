/***************************************************************************
                          DataVector.h  -  description
                             -------------------
    begin                : Tue Mar 19 2002
    copyright            : (C) 2002 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#ifndef DATA_VECTOR_H
#define DATA_VECTOR_H

#include <cstddef>

class vec_t; //fwd declaration
/**
 * This class encapsulates the functionality of the vec_t class defined
 * in the SSM library (see: .../ssm_docs.html/man/vec_t.common.html and
 * "The Shore Storage Manager Programming Interface"). It provides
 * the same interface with vec_t (or at least a subset). Essentially a data vector is an array of
 * (pointer, length) pairs. Each pointer points at a location in memory. The array
 * can be arbitrarily long, and methods are provided to comparing and copying data.
 * A data vector allows a sequence of memory chunks to be treated like one contiguous buffer.
 * This is very useful, e.g., for reducing the number of parameters passed to FileManager
 * methods (e.g., storeDataVectorsInCUBE_FileBucket) and to the underlying SSM methods.
 *
 * NOTE: Try to reduce the use of this class to a necessary minimum in order to avoid
 * code dependencies with  a class that is based on an SSM class.(Of course one can
 * always implement Data Vector independently of SSM vec_t)
 *
 * @see .../ssm_docs.html/man/vec_t.common.html
 * @see "The Shore Storage Manager Programming Interface"
 */
class DataVector {
public:
	/**
	 * Default constructor: empty data vector
	 */
	DataVector();
	
        /**
         * Constructor: create data vector from the memory pointed
         * by p, of size l bytes
         * @param	p 	pointer to any data type
         * @param	l	the number of bytes pointed to by p
         */
        DataVector(const void* p, size_t l);
	
        /**
         * Constructor: create a data vector from a part of another data vector v
         * @param v		source data vector
         * @param offset	byte offset in v where the desired data reside
         * @param limit 	#bytes we want from v starting at "offset"
         */
        DataVector(const DataVector& v, size_t offset, size_t limit);


        /**
         * Copy constructor
         */
        DataVector(const DataVector& v);
        	
        /**
         * Destructor
         */
	~DataVector();
	
	/**
	 * Assignment operator
	 */
	DataVector & operator=(const DataVector & other);
	
        /**
         * Split a data vector at byte offset l1 and produce two new data vectors
         * v1 and v2 provided by the caller
         */	
	void split(size_t l1, DataVector& v1, DataVector& v2);

	/**
	 * Put at the end (i.e., append) of this data vector, data vector v and
	 * return *this.
	 */
        DataVector& put(const DataVector& v);		

	/**
	 * Put at the end (i.e., append) of this data vector, the memory pointed
         * by p, of size l bytes
         * @param	p 	pointer to any data type
         * @param	l	the number of bytes pointed to by p	
	 */
        DataVector& put(const void* p, size_t l);

        /**
         * Put at the end (i.e., append) of this data vector,the part of data vector v
         * which resides at byte offset "offset" and is of length "limit" bytes
         *
         * @param v		source data vector
         * @param offset	byte offset in v where the desired data reside
         * @param limit 	#bytes we want from v starting at "offset"
         */	
        DataVector& put(const DataVector& v, size_t offset, size_t nbytes);

	/**
	 * Reset this data vector and return it also. Reset means
	 * to release all memory blocks pointed to by this vector
	 * and create an empty vector
	 */
        DataVector& reset();	

	/**
	 * Reset this data vector. Reset means
	 * to release all memory blocks pointed to by this vector
	 * and create an empty vector
	 */
        void init();

        /**
         * The set methods have similar functionality with put with the only
         * difference that first reset the target vector (i.e., overwrite previous
         * contents of this vector)
         */
    	DataVector& set(const DataVector& v1, const DataVector& v2);
        DataVector& set(const DataVector& v);
        DataVector& set(const void* p, size_t l);
        DataVector& set(const DataVector& v, size_t offset, size_t limit);

        /**
         * Return the total size in bytes of the memory areas that this vector includes
         */
	size_t size() const;

	/**
	 * Number of distinct memory areas that this vector includes
	 */	
        int count() const;	

        /**
         * Copy up to "limit" bytes from this vector to the memory pointer p.
         * It returns the number of bytes copied.
         * Note: it is assumed that p points at a valid allocated memory area ready
         * to be copied with the new data. copy_to performs no memory allocation for p!
         */
        size_t copy_to(void* p, size_t limit = 0x7fffffff) const;

 	/**
 	 *  Copy "limit" bytes from p into this data vector starting at byte offset "offset".
         *  copy_from() does not change DataVector itself, but overwrites
         *  the data area to which this vector points
         *  Returns *this
         */
        const DataVector& copy_from(
            const void* p,
            size_t limit,  // number of bytes to copy
            size_t offset = 0) const;   // offset tells where
                                        //in the vector to begin to copy

        DataVector& copy_from(const DataVector& v);

        DataVector& copy_from(
            const DataVector& v,
            size_t offset,          // offset in v
            size_t limit,           // # bytes
            size_t myoffset = 0);   // offset in this

	/**
	 * Return pointer at memory area included in this vector. The
	 * desired memory area is specified through "index". Values for
	 * "index" should be in the range [0, this->count()-1]
	 */
        const unsigned char*       ptr(int index) const;

        /**
         * Return the length in bytes of a memory area.The
	 * desired memory area is specified through "index". Values for
	 * "index" should be in the range [0, this->count()-1]
         */
        size_t      len(int index) const;

	/**
	 * Returns a reference to a vec_t created from this data vector.
	 * This is used when we want to pass a vec_t parameter to an SSM method.
	 *
	 * @see .../ssm_docs.html/man/vec_t.common.html	
	 */
        const vec_t& dataVector2vec_t() const;
        		
private:
	/**
	 * Pointer to an SSM vec_t instance
	 */
	vec_t* ssmvectp;		
};//DataVector

#endif // DATA_VECTOR_H