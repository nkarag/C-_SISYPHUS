/***************************************************************************
                          SystemManager.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "SystemManager.h"

// Static member initializations.
ss_m* SystemManager::ssm = NULL;
DeviceVolumeInfo SystemManager::devVolInfo = {"", 0, lvid_t::null};

SystemManager::SystemManager(const char* devName, 
			    unsigned int quota, 
			    bool initDev,
			    ostream& outputLogStream = cout,
                            ostream& errorLogStream = cerr
			    )
		: outputLogStream(outputLogStream),
	          errorLogStream(errorLogStream)
{
	// Update static members with their new values
	devVolInfo.deviceName = devName;
	devVolInfo.deviceQuotaKB = quota;

    	// Startup SSM at bootup time

    	outputLogStream << "Starting SSM and performing recovery ..." << endl;
    	ssm = new ss_m();

    	W_COERCE(setup_device_and_volume(initDev));

} // end SystemManager

SystemManager::~SystemManager() {
	// Shut down SSM if running
    	if (this->ssm) {
        	this->outputLogStream << "Shutting down SSM ..." << endl;
        	delete this->ssm;
        	this->outputLogStream << "SSM Shutdown." << endl;
    	}
}

/*
 * This function either formats a new device and creates a
 * volume on it, or mounts an already existing device and
 * returns the ID of the volume on it.
 */
rc_t SystemManager::setup_device_and_volume(bool init_device) {
    u_int vol_cnt;
    devid_t devid;

    if (init_device) {
        this->outputLogStream << "Formatting and mounting device: " << devVolInfo.deviceName
             << " with a " << devVolInfo.deviceQuotaKB << "KB quota ..." << endl;
        W_DO(ssm->format_dev(devVolInfo.deviceName.c_str(), devVolInfo.deviceQuotaKB, true));

        // mount the new device
        W_DO(ssm->mount_dev(devVolInfo.deviceName.c_str(), vol_cnt, devid));

        // generate a volume ID for the new volume we are about to
        // create on the device
        W_DO(ssm->generate_new_lvid(devVolInfo.volumeID));

        // create the new volume 
        this->outputLogStream << "Creating a new volume on the device" << endl;
        this->outputLogStream << "    with a " << devVolInfo.deviceQuotaKB << " KB quota ..." << endl;
        W_DO(ssm->create_vol(devVolInfo.deviceName.c_str(), devVolInfo.volumeID, devVolInfo.deviceQuotaKB));

        // create the logical ID index on the volume, reserving no IDs
        W_COERCE(ssm->add_logical_id_index(devVolInfo.volumeID, 0, 0));
    } else {
        this->outputLogStream << "Using already existing device: " << devVolInfo.deviceName << endl;
        // mount already existing device
        W_DO(ssm->mount_dev(devVolInfo.deviceName.c_str(), vol_cnt, devid));

        // find ID of the volume on the device
        lvid_t* lvid_list;
        u_int   lvid_cnt;
        W_DO(ssm->list_volumes(devVolInfo.deviceName.c_str(), lvid_list, lvid_cnt));
        if (lvid_cnt == 0) {
            this->errorLogStream << "Grid program error, device has no volumes" << endl;
            ::exit(1);
        }
        devVolInfo.volumeID = lvid_list[0];
        delete[] lvid_list; // delete is necessary to prevent memory leak
    }
    return RCOK;
} // end setup_device_and_volume


