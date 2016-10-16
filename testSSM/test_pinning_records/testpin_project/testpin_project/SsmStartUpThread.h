/***************************************************************************
                          SsmStartUpThread.h  -  description
                             -------------------
    begin                : Wed Jan 31 2001
    copyright            : (C) 2001 by Nikos Karayannidis
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef SSM_STARTUP_THREAD_H
#define SSM_STARTUP_THREAD_H

#include <sm_vas.h>

/**
 * A startup thread for the whole system. This thread is responsible for
 * creating a SystemManager (who in turn instantiates a ss_m), a CatalogManager, a BufferManager and a FileManager instance.
 * Then it launches a StdinThread for receiving input from the user.
 *
 * @see SystemManager
 * @see CatalogManager
 * @see BufferManager
 * @see FileManager
 * @see StdinThread
 *
 * @author Nikos Karayannidis
 */


class SsmStartUpThread : public smthread_t
{
private:

	/**
	 * Specifies the device name option, read from the configuration file
	 */
	option_t* optDeviceName;

	/**
	 * Specifies the device qouta option, read from the configuration file
	 */
	option_t* optDeviceQuota;

	/**
     	* Specifies whether the SHORE device should be initialised.
     	*/
    	bool initDevice;

public:

    	/**
     	* Creates a new startup thread with the specified options.
     	*
	* @param optDeviceName	the device name option specified in the configuration
	*			file.
	* @param optDeviceQuota	the device quota option specified in the configuration
	*			file.
     	* @param initDevice	a boolean specifying whether the SHORE device should
     	*                     	be initialised. Iff this is true, the device is created
     	*                     	anew, and, if it already existed, previous contents are
     	*                     	destroyed.
     	*/
	SsmStartUpThread(option_t * optDeviceName, option_t * optDeviceQuota, bool initDevice);

    	/**
     	* The destructor for the startup thread.
     	*/
        ~SsmStartUpThread();
        
	/**
	* The run method of the startup thread is responsible for booting up the
     	* system. It creates an instance for each of the following manager classes:
	* (SystemManager, CatalogManager, BufferManager, FileManager), and launches a
     	* StdinThread for receiving incoming user commands.
     	*/
	void run();
};

#endif // SSM_STARTUP_THREAD_H
