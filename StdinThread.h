/***************************************************************************
                          StdinThread.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef STDIN_THREAD_H
#define STDIN_THREAD_H

#include <fstream>
// ***NOTE***
// These two header files are necessary, in order to
// use sfile_read_hdl_t
// **********
#include <sthread.h>     
#include <sfile_handler.h>

#include <sm_vas.h>
#include "definitions.h"


/** 
 * This thread monitors stdin (or some file descriptor) for commands.
 * On construction it creates an AccessManager
 * object. The AccessManager::parseCommand() method will be called each time
 * StdinThread reads a new line of input.
 * The StdinThread terminates when the user quits. 
 *
 * @see: Shore grid example: stdin_thread_t (at rpc_thread.h/C) 
 * @author: Nikos Karayannidis
 */
class StdinThread : public smthread_t {

private:
	/**
	 * Name of error log file
	 */
	static const char* const ERROR_LOG_NAME = "error.log";
	
	/**
	 * Read handler for stdin thread, in order not
	 * to block the entire Unix process while waiting for
	 * input.
	 */
    	sfile_read_hdl_t*   _ready; 
	
public:
    	/**
	 * A buffer for reply messages.
	 */
    	char reply_buf[thread_reply_buf_size];
    	
	/**
	 * A file stream used as an error log
	 */    	
	static ofstream errorStream;

	/**
	 * Constructor of the stdin thread
	 */
	StdinThread();

	/**
	 * Destructor of the stdin thread
	 */
	~StdinThread();

	/**
	 * This is the code executed when the
	 * thread is forked. It creates an AccessManager instance.
	 */
	void run();
};

#endif // STDIN_THREAD_H
