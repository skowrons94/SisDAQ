#include "ReadoutUnit.h"

ReadoutUnit::ReadoutUnit( SISInterface* board ) 
{
    this->sisBoard = board;
} 

ReadoutUnit::~ReadoutUnit( )
{
    this->stop( );
}

int ReadoutUnit::start( )
{
    if( this->configure( ) != 0 )
        return -1;

    runCall = 1;
    acqThread = new boost::thread( &ReadoutUnit::process );
    return 0;
}

void ReadoutUnit::stop( )
{
    runCall = 0;
    acqThread->join( );
    delete acqThread;
    acqThread = nullptr;

    if( output.is_open( ) )
        output.close( );
}

int ReadoutUnit::configure( )
{
    if( output.is_open( ) )
        output.close( );

    output.open( outDir );
    if( !output.is_open( ) )
        return -1;
    
}

void ReadoutUnit::process( )
{
    
}