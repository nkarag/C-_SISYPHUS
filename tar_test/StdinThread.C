/***************************************************************************
                          StdinThread.C  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/


#include "StdinThread.h"
#include "AccessManager.h"


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

    _ready = new sfile_read_hdl_t(0);   // handle stdin
    if (!_ready) {
        cerr << "Error: Out of Memory" << endl;
        ::exit(1);
    }

    while (1) {
        cout << "Server> " ; cout.flush();
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

    cout << "Shutting down command thread" << endl;

} // end StdinThread::run 
