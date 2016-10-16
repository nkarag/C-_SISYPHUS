/***************************************************************************
                          SsmStartUpThread.C  -  description
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


#include "SsmStartUpThread.h"
#include "SystemManager.h"
#include "TestPinClass.h"

SsmStartUpThread::SsmStartUpThread(option_t * optDeviceName, option_t * optDeviceQuota, bool initDevice)
	: smthread_t(t_regular, false, false, "startup"),
	optDeviceName(optDeviceName),
	optDeviceQuota(optDeviceQuota),	
	initDevice(initDevice) 
{
}
/*
SsmStartUpThread::SsmStartUpThread(bool initDevice) 
	: smthread_t(t_regular, false, false, "startup"),
	 initDevice(initDevice) 
{
}
*/
SsmStartUpThread::~SsmStartUpThread() {
}

void SsmStartUpThread::run()
{
	cout << "Startup thread running ..." << endl;

    	cout << "Starting System Manager... " << endl;
	
	// get quota out of option class
	unsigned int quota = strtol(optDeviceQuota->value(), 0, 0);

	// get device name out of option class
	const char* devName = optDeviceName->value();
	
	// instantiate SystemManager, which in turn will instatiate its ss_m member
	// and also will setup the device and volume of the database.
	SystemManager* sysMgr = new SystemManager(devName, quota, initDevice);
	
	// Start application
	TestPinClass tp;	
cout<<"BEFORE delete\n";
    	delete sysMgr;
cout<<"AFTER delete\n";    	
}
 

