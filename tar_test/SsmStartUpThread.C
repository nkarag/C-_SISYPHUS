/***************************************************************************
                          SsmStartUpThread.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "SsmStartUpThread.h"
#include "SystemManager.h"
#include "CatalogManager.h"
#include "BufferManager.h"
#include "FileManager.h"
#include "StdinThread.h"

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
   
	CatalogManager* ctlgMgr; 
	try {	
		// Initialize Catalog manager
    		ctlgMgr = new CatalogManager();
	}
	catch(...) {
		cerr<< "SsmStartUpThread: Exception after calling CatalogManager constructor: server will be terminated!!!\n";
		delete ctlgMgr;
    		delete sysMgr;
		return;
	}

   	// Initialize Buffer Manager
   	BufferManager* bffrMgr = new BufferManager();

   	// Initialize File Manager
   	FileManager* flMgr = new FileManager();
  
   	// Spawn a stdin thread for getting input commands
	cout << "stdin thread starts out ...\n";
	StdinThread* stdinThrd = new StdinThread();

    	W_COERCE(stdinThrd->fork());

    	// wait for the stdin thread to finish
    	W_COERCE(stdinThrd->wait());
    	cout << "Stdin thread is done" << endl;
 
    	cout << "\nShutting down Sisyphus ..." << endl;
	delete bffrMgr;
	delete flMgr;
	delete ctlgMgr;
    	delete sysMgr;
}
 

