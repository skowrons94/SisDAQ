#ifndef SISETHERNETINTERFACE_H
#define SISETHERNETINTERFACE_H

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/uio.h>

#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "VMEInterface.h"

class SISEthernetInterface : public VMEInterface
{

private:
	char char_messages[128];
	int udp_socket_status;
	int udp_socket;
	unsigned int udp_port;
	struct sockaddr_in SIS3316_sock_addr_in;
	struct sockaddr_in myPC_sock_addr;
	unsigned int recv_timeout_sec;
	unsigned int recv_timeout_usec;
    char udp_send_data[2048];
    char udp_recv_data[16384];

public:
	SISEthernetInterface( void );

	int vmeopen ( void );
	int vmeclose( void );
	int get_vmeopen_messages( char* messages, uint32_t* nof_found_devices );

	//int get_device_informations( uint16_t* idVendor, uint16_t* idProduct, uint16_t* idSerNo, uint16_t* idFirmwareVersion, uint16_t* idDriverVersion );
	int get_UdpSocketStatus( void );
	int get_UdpSocketPort( void );
	int set_UdpSocketOptionTimeout( unsigned int receive_timeout_sec, unsigned int receive_timeout_usec );
	int set_UdpSocketOptionBufSize( int sockbufsize );
	int set_UdpSocketBindToDevice( char* eth_device );
	int set_UdpSocketBindMyOwnPort( char* pc_ip_addr_string );
	int set_UdpSocketSIS3316_IpAddress( char* sis3316_ip_addr_string );

	int udp_reset_cmd( void );
	int udp_register_read( uint32_t addr, uint32_t* data );
	int udp_sis3316_register_read ( unsigned int nof_read_registers, uint32_t* addr_ptr, uint32_t* data_ptr );
	int udp_register_write( uint32_t addr, uint32_t data );
	int udp_sis3316_register_write( unsigned int nof_read_registers, uint32_t* addr_ptr, uint32_t* data_ptr );

private:
	int udp_sub_sis3316_fifo_read( unsigned int nof_read_words, uint32_t  addr, uint32_t* data_ptr );
	int udp_sis3316_fifo_read( unsigned int nof_read_words, uint32_t  addr, uint32_t* data_ptr, uint32_t* got_nof_words );

	int udp_sub_sis3316_fifo_write( unsigned int nof_write_words, uint32_t  addr, uint32_t* data_ptr );
	int udp_sis3316_fifo_write( unsigned int nof_write_words, uint32_t addr, uint32_t* data_ptr, uint32_t* written_nof_words );

public:
	int vme_A32D32_read( uint32_t addr, uint32_t* data );
	int vme_A32DMA_D32_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32BLT32_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32MBLT64_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2EVME_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2ESST160_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2ESST267_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2ESST320_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );

	int vme_A32DMA_D32FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32BLT32FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32MBLT64FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2EVMEFIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2ESST160FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2ESST267FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );
	int vme_A32_2ESST320FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words );

	int vme_A32D32_write( uint32_t addr, uint32_t data );
	int vme_A32DMA_D32_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words );
	int vme_A32BLT32_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words );
	int vme_A32MBLT64_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words );
	int vme_A32DMA_D32FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words );
	int vme_A32BLT32FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words );
	int vme_A32MBLT64FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words );

	int vme_IRQ_Status_read( uint32_t* data ) ;

};

#endif // SISETHERNETINTERFACE_H_INCLUDED
