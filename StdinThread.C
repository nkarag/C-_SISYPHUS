/***************************************************************************
                          StdinThread.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "StdinThread.h"
#include "AccessManager.h"

// open error log stream in append mode
ofstream StdinThread::errorStream(StdinThread::ERROR_LOG_NAME, ios::app);

StdinThread::StdinThread() :
    smthread_t(t_regular,       /* regular priority */
               false,           /* will run ASAP    */
               false,           /* will not delete itself when done */
               "stdin")         /* thread name */
{
}

StdinThread::~StdinThread() {

}

void StdinThread:: run() {
    cerr << "Command thread is running" << endl;

    char        line_buf[256];
    char*       line;
    bool        quit = false;
    rc_t        rc;

/* OLD
    // start a command server
    command_server_t cmd_server;
*/
    // start a new access manager
    AccessManager* accessMgr = new AccessManager();

    //_ready = new sfile_read_hdl_t(0);   // handle stdin
    _ready = new (nothrow) sfile_read_hdl_t(0);   // handle stdin.
    						//Nikos 4/Feb/02: add nothrow form of new
    if (!_ready) {
        cerr << "(" << __FILE__ << ", " <<  __LINE__ << "): " << "Error: Out of Memory. Cant allocate input handle. Aborting ..." << endl;
        ::exit(1);
    }

    while (1) {
        cout << "Sisyphus> " ; cout.flush();
        rc = _ready->wait(WAIT_FOREVER);
        if(!rc) {
            //cerr << "stdin ready" << endl;
            line = fgets(line_buf, sizeof(line_buf)-1, stdin);
            if (line == 0) {
                // end-of-file
                break;
            }
            accessMgr->parseCommand(line_buf, quit);
            if (quit) {
                // quit command was entered
                break;
            }
        } else {
            // someone called _ready->shutdown().
            cerr << "exiting command thread " <<  endl;
            break;
        }
    }
    assert(sfile_read_hdl_t::is_active(0));
    delete _ready;
    _ready = 0;
    assert(!sfile_read_hdl_t::is_active(0));

    delete accessMgr;
    accessMgr = 0;

    cout << "Shutting down command thread" << endl;

} // end StdinThread::run 
