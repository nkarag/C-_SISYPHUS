/***************************************************************************
                          sisyphus.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


/*
 * This file implements the main code for the sisyphus
 *
 * Author: Nikos Karayannidis
 * Last updated: 17/4/2000
 */

#include <sm_vas.h> // This include brings in all header files needed for writing a VAS

// other sisyphus header files
#include "SsmStartUpThread.h"

// other C++ header files
#include <iostream>
#include <string>

// typedefs
typedef w_rc_t rc_t; // shorten error code type name

// globals
const char* opt_file = "./config";      // option configuration file

///////////// Function Prototypes /////////////////////////////////
/**
 * The following version of init_config_options is copied from
 * ~shore1/doc/examples/vas/hello/hello.C and it assumes that the
 * device name and quota are hard coded in the program.
 * SSM options are read from the configuration file (opt_file)
 * @see SystemManager::devVolInfo
 */
//rc_t init_config_options(option_group_t& options);

/**
 * The following version of init_config_options is copied from
 * ~shore1/doc/examples/vas/grid/options.C and it assumes that the
 * device name and quota are given in the configuration file.
 *
 * @see SystemManager::devVolInfo
 *
 * Next, the comments from the Grid sources follow:
 *
 * init_config_options intialized configuration options for
 * both the client and server programs in the Grid example.
 *
 * The options parameter is the option group holding all the options.
 * It is assumed that all SSM options have been added if called
 * by the server.
 *
 * The prog_type parameter is should be either "client" or "server" (defaults to "server" 
 * in our case since there is no client program for sisyphus, since it is single user and accepts
 * command from a StdinThread running within the server process (Nikos Karayannidis)).
 *
 * The argc and argv parameters should be argc and argv from main().
 * Recognized options will be located in argv and removed.  argc
 * is changed to reflect the removal.
 *
 */
w_rc_t
init_config_options(option_group_t& options,
                    const char* prog_type,
                    int& argc, char** argv);

/**
 * This is a copy from the usage function in ~shore1/doc/example/vas/grid/server.C
 */
void usage(option_group_t& options);

/////////////////////// Main ///////////////////////////////////////


/**
 * Main routine for the sisyphus. Reads the configuration options
 * and launches a SsmStartUpThread.If the option "-i" is supplied on
 * the command line, the Database is initialized.
 *
 * @author Nikos Karayannidis
 * 
 */
int main(int argc, char* argv[]) {

	// Process options...
	cout <<  "Processing ssm specific configuration options and sisyphus config options from config file\n";

	// Ssm specific options cover such things as: location of the diskrw program, buffer pool size, log directory, etc.
	// sisyphus specific options : device name, i.e. full path name of the Unix file, in which the Shore device will
        //                               be implemented, device quota in KB (maximum device space).

	// create pointers to options we will use for sisyphus
	option_t* opt_device_name = 0;
    	option_t* opt_device_quota = 0;


	const int option_level_cnt = 3; 
	option_group_t options(option_level_cnt);
	

	W_COERCE(options.add_option("device_name", "device/file name",
                        NULL, "device containg volume to use for sisyphus program",
                        true, option_t::set_value_charstr,
                        opt_device_name));

   	W_COERCE(options.add_option("device_quota", "# > 1000",
                        "2000", "quota for device containing sisyphus volume",
                        false, option_t::set_value_long,
                        opt_device_quota));


	// have the SSM add its options to the group
       	W_COERCE(ss_m::setup_options(&options));

	// Simple version of init_config_options (from hello example)
	/* W_COERCE(init_config_options(options)); */

	// Grid example version of init_config_options
	if (init_config_options(options, "server", argc, argv)) {
       		usage(options);
       		::exit(1);
    	}

	// process command line: looking for the "-i" flag
    	bool init_device = false;
    	if (argc > 2) {
       		usage(options);
       		::exit(1);
    	} else if (argc == 2) {
       		if (strcmp(argv[1], "-i") == 0) {
           		cout << "Do you really want to initialize the sisyphus database? ";
            		char answer;
            		cin >> answer;
            		if (answer == 'y' || answer == 'Y') {
               			init_device = true;
           	 	} else {
               			cerr << "Please try again without the -i option" << endl;
               			::exit(0);
            		}
        	} else {
            		usage(options);
            		::exit(1);
        	}
    	} // end else if(argc == 2)

	// Start thread that will instantiate Shore Storage Manager
	SsmStartUpThread *startupThread = new SsmStartUpThread(opt_device_name,opt_device_quota,init_device);

	if(!startupThread) {
	W_FATAL(fcOUTOFMEMORY);
	}

	cout << "Forking start up thread..." << endl;

	// Execute the code in the run method of new thread.
	W_COERCE(startupThread->fork());

	cout << "Waiting on startup thread ..." << endl;
	W_COERCE(startupThread->wait());
	
	delete startupThread;
	startupThread = 0;
	cout << "Server shut down." << endl;
}

///////////////// Function Definitions ///////////////////////////

/**
 * The following version of init_config_options is copied from
 * ~shore1/doc/examples/vas/hello/hello.C and it assumes that the
 * device name and quota are hard coded in the program.
 *
 * @see SystemManager::devVolInfo
 */
/* 
rc_t init_config_options(option_group_t& options)
{

    rc_t rc;    // return code

    // read the config file to set options
    {
        ostrstream      err_stream;
        option_file_scan_t opt_scan(opt_file, &options);

        // scan the file and override any current option settings
        // options names must be spelled correctly
        rc = opt_scan.scan(true //override
				, err_stream, true);
        if (rc) {
            char* errmsg = err_stream.str();
            cerr << "Error in reading option file: " << opt_file << endl;
            cerr << "\t" << errmsg << endl;
            if (errmsg) delete errmsg;
            return rc;
        }
    }

    // check required options
    {
        ostrstream      err_stream;
        rc = options.check_required(&err_stream);
        char* errmsg = err_stream.str();
        if (rc) {
            cerr << "These required options are not set:" << endl;
            cerr << errmsg << endl;
            if (errmsg) delete errmsg;
            return rc;
        }
    }

    return RCOK;
}
*/

/**
 * The following version of init_config_options is copied from
 * ~shore1/doc/examples/vas/grid/options.C and it assumes that the
 * device name and quota are given in the configuration file.
 *
 * @see SystemManager::devVolInfo
 *
 * Next, the comments from the Grid sources follow:
 *
 * init_config_options intialized configuration options for
 * both the client and server programs in the Grid example.
 *
 * The options parameter is the option group holding all the options.
 * It is assumed that all SSM options have been added if called
 * by the server.
 *
 * The prog_type parameter is should be either "client" or "server" (defaults to "server" 
 * in our case since there is no client program for sisyphus, since it is single user and accepts
 * command from a StdinThread running within the server process (Nikos Karayannidis)).
 *
 * The argc and argv parameters should be argc and argv from main().
 * Recognized options will be located in argv and removed.  argc
 * is changed to reflect the removal.
 *
 */

w_rc_t
init_config_options(option_group_t& options,
                    const char* prog_type,
                    int& argc, char** argv)
{

    w_rc_t rc;  // return code

    // set prog_name to the file name of the program without the path
    char* prog_name = strrchr(argv[0], '/');
    if (prog_name == NULL) {
        prog_name = argv[0];
    } else {
        prog_name += 1; /* skip the '/' */
        if (prog_name[0] == '\0')  {
                prog_name = argv[0];
        }
    }

    W_COERCE(options.add_class_level("sisyphus_server"));       // program type, i.e. for all options concerning sisyphus programs
    W_COERCE(options.add_class_level("server"));        // server or client (in our case is always server)
    W_COERCE(options.add_class_level("sisyphus"));         // program name

    // read the .examplerc file to set options
    {
        ostrstream      err_stream;
        option_file_scan_t opt_scan(opt_file, &options);

        // scan the file and override any current option settings
        // options names must be spelled correctly
        rc = opt_scan.scan(true /*override*/, err_stream, true);
        if (rc) {
            char* errmsg = err_stream.str();
            cerr << "Error in reading option file: " << opt_file << endl;
            cerr << "\t" << errmsg << endl;
            if (errmsg) delete errmsg;
            return rc;
        }
    }

    // parce argv for options
    if (!rc) {
        // parse command line
        ostrstream      err_stream;
        rc = options.parse_command_line((const char**)argv, argc, 2, &err_stream);
        err_stream << ends;
        char* errmsg = err_stream.str();
        if (rc) {
            cerr << "Error on Command line " << endl;
            cerr << "\t" << w_error_t::error_string(rc.err_num()) << endl;
            cerr << "\t" << errmsg << endl;
            return rc;
        }
        if (errmsg) delete errmsg;
    }

    // check required options
    {
        ostrstream      err_stream;
        rc = options.check_required(&err_stream);
        if (rc) {
            char* errmsg = err_stream.str();
            cerr << "These required options are not set:" << endl;
            cerr << errmsg << endl;
            if (errmsg) delete errmsg;
            return rc;
        }
    }

    return RCOK;
}


void usage(option_group_t& options)
{
    cerr << "Usage: server [-i] [options]" << endl;
    cerr << "       -i will re-initialize the device/volume for the DB" << endl;
    cerr << "Valid options are: " << endl;
    options.print_usage(true, cerr);
}

