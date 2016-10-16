/***************************************************************************
                          SystemManager.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <sm_vas.h>
#include <string>

/**
 * This struct stores information about the shore device used
 * and the volume within the device, used in order to
 * store the database.
 */
class DeviceVolumeInfo {
public:
	/**
	 * The file path and the name of the Unix file
	 * in which the device will be created.
	 */
	string deviceName;
	/**
	 The quota limit of the device (in KB).
	 */
	unsigned int deviceQuotaKB;
	/**
	 * The logical ID of the volume
	 * @see Shore documentation for devices, volumes and logical ids
	 */
	lvid_t volumeID;	
};
		

/**
 * The SystemManager class handles all system specific tasks.
 * This class wraps the ss_m class and provides public methods
 * for performing system specific tasks. In its DeviceVolumeInfo member it stores
 * system specific information, such as: the device name and quota as well as the volume id.
 * Its main tasks on construction are:
 * 	1. Instantiate a ss_m (performs recovery, buffer pool allocation).
 *	2. Set up device and volume for the database. 
 *
 * Note: Since it contains a ss_m member it should be instantiated only once each time the server is up!
 *
 * @author: Nikos Karayannidis
 */
class SystemManager {

private:
	/**
     	 * The output stream used for logging system messages.
     	 */
      	ostream& outputLogStream;

        /**
         * The output stream used for logging error messages.
         */
    	ostream& errorLogStream;

protected:
	/**
     	 * The Shore storage manager used.
     	 * 
     	 * Note: This variable in the grid example, which comes 
	 *       with shore 1.1 documentation, was global, but for
     	 *       data abstraction purposes it makes much more
     	 *       sense to put it in the class responsible for
     	 *       using it. We want to isolate the rest of the classes
	 *	 from the ss_m interfaces. Only the System Manager instance
         *	 should know about the existence of ss_m.
     	 *
     	 */
    	static ss_m* ssm;

	/**
	 * Information about the shore device used
	 * and the volume within the device, used in order to
 	 * store the database.
	 */
	static DeviceVolumeInfo devVolInfo;

public:
	/**
	 * Creates a new SystemManager for the whole system. This constructor
     	 * loads up the Shore storage manager. Hence, creating two instances
     	 * of a SystemManager results in a Shore error. This essentially makes
     	 * sense, since there is no point in having two SystemManagers running
     	 * at the same time.
 	 * The tasks on construction are:
	 * 	1. Store system information (device name and quota, volume id)
 	 * 	2. Instantiate a ss_m (performs recovery, buffer pool allocation).
 	 *	3. Set up device and volume for the database. 
	 *
	 * @param devName	The path of the unix file, where the device will be stored.
	 * @param quota		The quota (in KBs) of the device.
	 * @param initDev	if true, the Shore device used for holding the database
     	 *                      is initialized, erasing any contents that may have
     	 *                      previously existed. The device must be initialized the
     	 *                      first time the system is run, or a Shore error will
     	 *                      result in not finding the device.
	 * @param outputLogStream  an output stream for logging information messages.
     	 *                         Defaults to cout.
     	 * @param errorLogStream   an output stream for logging error messages.
     	 *                         Defaults to cerr.
	 */

	SystemManager(const char* devName, unsigned int quota, bool initDev,ostream& outputLogStream = cout,
                            ostream& errorLogStream = cerr);
	/**
	 * Destructor of SystemManager class, main tasks:	
	 * Delete the ssm instance.
	 */
	~SystemManager(); 
	
	/**
	 * Read-only method for the device and volume information stored in System Manager.
	 */	
	static const struct DeviceVolumeInfo* getDevVolInfo() { return &devVolInfo; }
private:

	/**
	 * This function either formats a new device and creates a
	 * volume on it, or mounts an already existing device and
	 * returns the ID of the volume on it.
	 */
	rc_t setup_device_and_volume(bool initDev);

};

#endif // SYSTEM_MANAGER_H
