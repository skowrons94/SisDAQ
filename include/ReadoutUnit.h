#ifndef READOUTUNIT_H
#define READOUTUNIT_H

#include <fstream>
#include <boost/thread.hpp>

#include "SISInterface.h"

class ReadoutUnit {

    ReadoutUnit( SISInterface* board );
    ~ReadoutUnit( );

    public:
        int  start( );
        void stop( );

        void setOutDir( std::string out ){ outDir = out; };

    private:

        void process( );
        int  configure( );

        int runCall = 1;

        std::string     outDir;
        std::ofstream   output;

        SISInterface*   sisBoard;
        boost::thread*  acqThread;

};

#endif // READOUTUNIT_H_INCLUDED