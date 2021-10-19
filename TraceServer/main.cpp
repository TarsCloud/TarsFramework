#include "TraceServer.h"

int main(int argc, char *argv[]) {
    try {
        TraceServer app;
        app.main(argc, argv);
        app.waitForShutdown();
    }
    catch (exception &ex) {
        cerr << ex.what() << endl;
    }
    return 0;
}