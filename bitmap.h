/***************************************************************************
                          bitmap.h  -  Bitmap handling macros
                             -------------------
    begin                : Sat Sep 22 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/
#ifndef BITMAP_H
#define BITMAP_H

#include <cmath>

/**
 * This namespace encapsulates several constants and inline functions (i.e. macros)
 * for bitmap handling.
 *
 * @author Nikos Karayannidis
 * @see struct DiskDataChunk
 */
namespace bmp {


        typedef unsigned int WORD; // a bitmap will be represented by an array of WORDS

        // Function declarations
        double log2(double x);
        unsigned int numOfWords(unsigned int no_bits);
        WORD create_mask();

        /**
         * Number of bits per word, e.g., per unsigned integer, if the
         * bitmap is represented by an array of unsigned integers
         */
        const unsigned int BITSPERWORD = sizeof(WORD)*8;

        /**
         * Used in order to locate the WORD in which a bit has been stored
         */
        const unsigned int SHIFT = static_cast<unsigned int>(ceil(log2(BITSPERWORD)));
} //namespace bmp

/**
* logarithm of base 2
*/
inline double bmp::log2(double x) {
        return log(x)/log(2);
}

/**
* Returns the number of words needed to stored a bitmap
* of size no_bits bits
*/
inline unsigned int bmp::numOfWords(unsigned int no_bits) {
        //return no_bits/BITSPERWORD + 1;
        return static_cast<unsigned int>(ceil(double(no_bits)/double(bmp::BITSPERWORD)));
}

/**
* This mask is used in order to isolate the number of LSBits from an integer i (representing a
* bit position in a bitmap) that correspond to the position of the bit within a WORD.
*			                     			        ~~~~~~
*/
inline bmp::WORD bmp::create_mask(){
        bmp::WORD MASK = 1;
        // turn on the #SHIFT LSBits
        for(int b = 1; b<bmp::SHIFT; b++){
                MASK |= (1 << b);
        }
        return MASK;
}


#endif //BITMAP_H