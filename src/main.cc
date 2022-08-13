#include <iostream>

#include "SISSettings.h"
#include "SISInterface.h"
#include "SISEthernetInterface.h"

int main() {

    SISEthernetInterface* eth;
    eth = new SISEthernetInterface( );

    int return_code;

    char buff[64];
    strcpy( buff, "" );
    return_code = eth->set_UdpSocketOptionBufSize( 33554432 );
    return_code = eth->set_UdpSocketBindMyOwnPort( buff );

    strcpy( buff, "192.168.1.100" );
    eth->set_UdpSocketSIS3316_IpAddress( buff );

	eth->udp_reset_cmd( );
	eth->vme_A32D32_write( SIS3316_INTERFACE_ACCESS_ARBITRATION_CONTROL, 0x80000000 );
	eth->vme_A32D32_write( SIS3316_INTERFACE_ACCESS_ARBITRATION_CONTROL, 0x1 );

    SISInterface* sisBoard;
	sisBoard = new SISInterface( eth, 0x0 );

    unsigned int data;
    return_code = eth->vme_A32D32_read( SIS3316_MODID, &data );
	printf("return_code = 0X%08x   Module ID = 0X%08x \n",return_code, data);

    SISSettings* sisSettings;
    sisSettings = new SISSettings( sisBoard );

    sisSettings->writeSetting( "Input TAP Delay", 1, 10 );
    sisSettings->readRegisters( );
    sisSettings->saveRegisters( "prova.json" );

    return 0;
}