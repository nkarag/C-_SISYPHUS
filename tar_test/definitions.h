/***************************************************************************
                          definitions.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#ifndef DEF_H
#define DEF_H

/**
 * Boolean values
 */
//const bool TRUE = 1;
//const bool FALSE = 0;

/**
 * Stdin thread reply buffer size
 */
const unsigned int thread_reply_buf_size = 256;

/**
 * Disk page size in bytes
 */
const unsigned int PAGE_SIZE = 8192;

/**
 * Minimum bucket occupancy threshold (in bytes)
 */
const unsigned int BCKT_THRESHOLD = int(8192*0.5);

/**
 * Sparseness Threshold : for a compression factor <= 0.75 <=> SPARSENESS_THRSHLD <= 0.75 - (1/(8*entry_size))
 */
const float SPARSENESS_THRSHLD = 0.73; // for entry_size == 8 bytes

/**
 * Max number of characters in a chunk id
 */
const unsigned int MAXCHAR_ID = 100;

#endif // DEF_H