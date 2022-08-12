#include "SISEthernetInterface.h"

using namespace std;

SISEthernetInterface::SISEthernetInterface( void )
{
 	int status;
	strcpy (this->char_messages ,  "no valid UDP socket");

	this->udp_socket_status = -1 ;
	this->udp_port = 0xE000 ; // default

	this->myPC_sock_addr.sin_family = AF_INET;
	this->myPC_sock_addr.sin_port = htons(udp_port);
	this->myPC_sock_addr.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(myPC_sock_addr.sin_zero),0,8);

	this->SIS3316_sock_addr_in.sin_family = AF_INET;
	this->SIS3316_sock_addr_in.sin_port = htons(udp_port);
	this->SIS3316_sock_addr_in.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(SIS3316_sock_addr_in.sin_zero),0,8);
	// Create udp_socket
	this->udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->udp_socket == -1) {
		this->udp_socket_status = -1 ;
	}
	else {
		this->udp_socket_status = 0 ;
	}
 
	// Set  Receive Timeout
	this->recv_timeout_sec  = 0 ;
	//this->recv_timeout_usec = 100000 ; // default 100ms
	this->recv_timeout_usec = 50000 ; // default 50ms

	status = this->set_UdpSocketOptionTimeout(recv_timeout_sec, recv_timeout_usec ) ;

}


int SISEthernetInterface::vmeopen( void )
{
//	int status;
//	struct SIS3150USB_Device_Struct info[MAX_SIS3150USB_DEVICES];
//	uint32_t data;
//	uint32_t found=0;

	strcpy (this->char_messages ,  "sis3316 UDP port is open");

	return 0;
}



int SISEthernetInterface::vmeclose( void )
{
	//CloseHandle (this->sis3150_device);
	return 0;
}

int SISEthernetInterface::get_vmeopen_messages( char* messages, uint32_t* nof_found_devices )
{
	strcpy (messages,  this->char_messages);
	*nof_found_devices = 1 ;

	return 0;
}
	int udp_socket;
	unsigned int udp_port ;
	struct sockaddr_in SIS3316_sock_addr_in   ;
	struct sockaddr_in myPC_sock_addr   ;


int SISEthernetInterface::get_UdpSocketStatus( void )
{
	return this->udp_socket_status;
}

int SISEthernetInterface::get_UdpSocketPort( void )
{
	return this->udp_port;
}

int SISEthernetInterface::set_UdpSocketOptionTimeout( unsigned int receive_timeout_sec, unsigned int receive_timeout_usec )
{
	int return_code ;

	struct timeval struct_time;
	struct_time.tv_sec  = this->recv_timeout_sec;
	struct_time.tv_usec = this->recv_timeout_usec; //  
	return_code = (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&struct_time,sizeof(struct timeval)));

	return return_code ;
}

int SISEthernetInterface::set_UdpSocketOptionBufSize( int sockbufsize )
{
	int return_code ;
	return_code =  (setsockopt(this->udp_socket, SOL_SOCKET,SO_RCVBUF, (char *) &sockbufsize, (int)sizeof(sockbufsize)));
	return return_code ;
}

int SISEthernetInterface::set_UdpSocketBindToDevice( char* eth_device )
{
	int return_code=0;

	//return_code = setsockopt(this->udp_socket, SOL_SOCKET, SO_BINDTODEVICE, eth_device, sizeof(eth_device)) ;

	return return_code;
}

int SISEthernetInterface::set_UdpSocketBindMyOwnPort( char* pc_ip_addr_string )
{
	int return_code;

	//this->udp_port = 0xE000 ; // default start UDP port
	this->myPC_sock_addr.sin_family = AF_INET;
	this->myPC_sock_addr.sin_port = htons(this->udp_port);
	this->myPC_sock_addr.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(myPC_sock_addr.sin_zero),0,8);

	//if((int)sizeof(pc_ip_addr_string) != 0) {
	//unsigned int uint_string_length = strlen(pc_ip_addr_string) ;
	if(strlen(pc_ip_addr_string) != 0) {
		this->myPC_sock_addr.sin_addr.s_addr = inet_addr(pc_ip_addr_string);
	}

	do {
		return_code = bind(this->udp_socket,  (struct sockaddr *)&this->myPC_sock_addr, sizeof(this->myPC_sock_addr));
		if (return_code != 0) {
			this->udp_port++;
			this->myPC_sock_addr.sin_port = htons(this->udp_port);
		}
	} while ((return_code == -1) && (this->udp_port < 0xF000)) ;
	return return_code;
}

int SISEthernetInterface::set_UdpSocketSIS3316_IpAddress( char* sis3316_ip_addr_string )
{
	int return_code=0;

	this->SIS3316_sock_addr_in.sin_family = AF_INET;
	this->SIS3316_sock_addr_in.sin_port = htons(this->udp_port);
	this->SIS3316_sock_addr_in.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(SIS3316_sock_addr_in.sin_zero),0,8);

	if(strlen(sis3316_ip_addr_string) != 0) {
		this->SIS3316_sock_addr_in.sin_addr.s_addr = inet_addr(sis3316_ip_addr_string);
	}
	else {
		return_code=-1;	
	}
	return return_code;
}

int SISEthernetInterface::udp_reset_cmd( void )
{
	int return_code;
     // write Cmd
    this->udp_send_data[0] = 0xFF ; // reset
    return_code = sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));

	return return_code;
}

int SISEthernetInterface::udp_register_read( uint32_t addr, uint32_t* data )
{
	int return_code;

    socklen_t addr_len;

    unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);
  // send Read Req Cmd
    this->udp_send_data[0] = 0x10 ; // register read CMD
    this->udp_send_data[1] = (unsigned char)  (addr & 0xff) ;        // address(7 dwonto 0)
    this->udp_send_data[2] = (unsigned char) ((addr >>  8) & 0xff) ; // address(15 dwonto 8)
    this->udp_send_data[3] = (unsigned char) ((addr >> 16) & 0xff) ; // address(23 dwonto 16)
    this->udp_send_data[4] = (unsigned char) ((addr >> 24) & 0xff) ; // address(31 dwonto 24)
    return_code = sendto(this->udp_socket, udp_send_data, 5, 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));


    return_code = recvfrom(udp_socket, udp_recv_data, 9, 0,  (struct sockaddr *)&this->SIS3316_sock_addr_in, &addr_len );


	if(return_code == 9) {
		data_ptr = (unsigned int*)&udp_recv_data[5];
		*data = *data_ptr;
		return_code = 0;
    }
	else {
		return_code = -1;
	}
	return return_code;
}

int SISEthernetInterface::udp_sis3316_register_read( unsigned int nof_read_registers, uint32_t* addr_ptr, uint32_t* data_ptr )
{
    unsigned int i;
	int return_code;

    socklen_t addr_len;

    unsigned int nof_32bit_word;
    unsigned int reg_addr;
  
	//unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);

	nof_32bit_word = nof_read_registers ;
	if (nof_read_registers == 0) {
		nof_32bit_word = 1 ;
	}
	if (nof_read_registers > 64) {
		nof_32bit_word = 64 ;
	}
	
	// send Read Req Cmd
	this->udp_send_data[0] = 0x20 ; // send SIS3316 Register Read Req Cmd
    this->udp_send_data[1] = (unsigned char)  ((nof_32bit_word-1) & 0xff);           //  lower length
    this->udp_send_data[2] = (unsigned char) (((nof_32bit_word-1) >>  8) & 0xff);    //  upper length 

	for (i=0;i<nof_32bit_word;i++) {
		reg_addr = addr_ptr[i] ;
		this->udp_send_data[(4*i)+3] = (unsigned char)  (reg_addr & 0xff) ;        // address(7 dwonto 0)
		this->udp_send_data[(4*i)+4] = (unsigned char) ((reg_addr >>  8) & 0xff) ; // address(15 dwonto 8)
		this->udp_send_data[(4*i)+5] = (unsigned char) ((reg_addr >> 16) & 0xff) ; // address(23 dwonto 16)
		this->udp_send_data[(4*i)+6] = (unsigned char) ((reg_addr >> 24) & 0xff) ; // address(31 dwonto 24)
	}
    return_code = sendto(this->udp_socket, udp_send_data, 3 + (4*nof_32bit_word), 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));

	return_code = recvfrom(udp_socket, udp_recv_data, 512, 0,  (struct sockaddr *)&this->SIS3316_sock_addr_in, &addr_len );

    if(return_code == (int)(2 + (4*nof_32bit_word))) {
		for (i=0;i<nof_32bit_word;i++) {
			data_ptr[i] = *((unsigned int *)&udp_recv_data[2+(4*i)]) ;
		}
		return_code = 0;
    }
	else {
		return_code = -1;
	}
	return return_code;
}

int SISEthernetInterface::udp_register_write( uint32_t addr, uint32_t data )
{
	int return_code;

    socklen_t addr_len;

    //unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);
  // send Read Req Cmd
    this->udp_send_data[0] = 0x11 ; // register write CMD  
    this->udp_send_data[1] = (unsigned char)  (addr & 0xff) ;        // address(7 dwonto 0)
    this->udp_send_data[2] = (unsigned char) ((addr >>  8) & 0xff) ; // address(15 dwonto 8)
    this->udp_send_data[3] = (unsigned char) ((addr >> 16) & 0xff) ; // address(23 dwonto 16)
    this->udp_send_data[4] = (unsigned char) ((addr >> 24) & 0xff) ; // address(31 dwonto 24)
    this->udp_send_data[5] = (unsigned char)  (data & 0xff) ;        // data(7 dwonto 0)
    this->udp_send_data[6] = (unsigned char) ((data >>  8) & 0xff) ; // data(15 dwonto 8)
    this->udp_send_data[7] = (unsigned char) ((data >> 16) & 0xff) ; // data(23 dwonto 16)
    this->udp_send_data[8] = (unsigned char) ((data >> 24) & 0xff) ; // data(31 dwonto 24)
    return_code = sendto(this->udp_socket, udp_send_data, 9, 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));
	return_code = 0;
	return return_code;
}

int SISEthernetInterface::udp_sis3316_register_write( unsigned int nof_read_registers, uint32_t* addr_ptr, uint32_t* data_ptr )
{
    unsigned int i;
	int return_code;

    socklen_t addr_len;

    unsigned int nof_32bit_word;
    unsigned int reg_addr, reg_data;

    addr_len = sizeof(struct sockaddr);

	nof_32bit_word = nof_read_registers ;
	if (nof_read_registers == 0) {
		nof_32bit_word = 1 ;
	}
	if (nof_read_registers > 64) {
		nof_32bit_word = 64 ;
	}
	
	// send Read Req Cmd
	this->udp_send_data[0] = 0x21 ; // send SIS3316 Register Read Req Cmd
    this->udp_send_data[1] = (unsigned char)  ((nof_32bit_word-1) & 0xff);           //  lower length
    this->udp_send_data[2] = (unsigned char) (((nof_32bit_word-1) >>  8) & 0xff);    //  upper length 

	for (i=0;i<nof_32bit_word;i++) {
		reg_addr = addr_ptr[i] ;
		this->udp_send_data[(8*i)+3] = (unsigned char)  (reg_addr & 0xff) ;        // address(7 dwonto 0)
		this->udp_send_data[(8*i)+4] = (unsigned char) ((reg_addr >>  8) & 0xff) ; // address(15 dwonto 8)
		this->udp_send_data[(8*i)+5] = (unsigned char) ((reg_addr >> 16) & 0xff) ; // address(23 dwonto 16)
		this->udp_send_data[(8*i)+6] = (unsigned char) ((reg_addr >> 24) & 0xff) ; // address(31 dwonto 24)
		reg_data = data_ptr[i] ;
		this->udp_send_data[(8*i)+7]  = (unsigned char)  (reg_data & 0xff) ;        // reg_data(7 dwonto 0)
		this->udp_send_data[(8*i)+8]  = (unsigned char) ((reg_data >>  8) & 0xff) ; // reg_data(15 dwonto 8)
		this->udp_send_data[(8*i)+9]  = (unsigned char) ((reg_data >> 16) & 0xff) ; // reg_data(23 dwonto 16)
		this->udp_send_data[(8*i)+10] = (unsigned char) ((reg_data >> 24) & 0xff) ; // reg_data(31 dwonto 24)
	}
    return_code = sendto(this->udp_socket, udp_send_data, 3 + (8*nof_32bit_word), 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));

	return_code = recvfrom(udp_socket, udp_recv_data, 512, 0,  (struct sockaddr *)&this->SIS3316_sock_addr_in, &addr_len );

    if(return_code == 2 ) {
		return_code = 0;
    }
	else {
		return_code = -1;
	}
	return return_code;
}

/* nof_read_words: max. 0x10000 (64k 32-bit words = 256KBytes )  */
#define UDP_MAX_NOF_READ_32bitWords    0x10000

int SISEthernetInterface::udp_sub_sis3316_fifo_read( unsigned int nof_read_words, uint32_t  addr, uint32_t* data_ptr )
{
    int i;
	int return_code;
    socklen_t addr_len;
    unsigned int nof_32bit_word;
  	int send_length;
 
	unsigned int udp_data_copy_to_buffer_index ;
	unsigned int nof_read_data_bytes ;
	int rest_length_byte ;
	unsigned int soft_packet_number;
	unsigned int packet_number;
	unsigned int packet_status;
	unsigned int packet_cmd;
	unsigned char* uchar_ptr;
	int receive_bytes;

	//unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);

	nof_32bit_word = nof_read_words ;
	if (nof_read_words == 0) {
		return 0 ;
		//nof_32bit_word = 1 ;
	}
	if (nof_read_words > UDP_MAX_NOF_READ_32bitWords) {
		nof_32bit_word = UDP_MAX_NOF_READ_32bitWords ;
	}
	
	// send Read Req Cmd
	this->udp_send_data[0] = 0x30 ; // send SIS3316 Fifo Read Req Cmd
    this->udp_send_data[1] = (unsigned char)  ((nof_32bit_word-1) & 0xff);           //  lower length
    this->udp_send_data[2] = (unsigned char) (((nof_32bit_word-1) >>  8) & 0xff);    //  upper length 

	this->udp_send_data[3] = (unsigned char)  (addr & 0xff) ;        // address(7 dwonto 0)
	this->udp_send_data[4] = (unsigned char) ((addr >>  8) & 0xff) ; // address(15 dwonto 8)
	this->udp_send_data[5] = (unsigned char) ((addr >> 16) & 0xff) ; // address(23 dwonto 16)
	this->udp_send_data[6] = (unsigned char) ((addr >> 24) & 0xff) ; // address(31 dwonto 24)

	send_length = 7 ;
    return_code = sendto(this->udp_socket, udp_send_data, send_length, 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));
	if (return_code != send_length) {
	// sendto error !
	}


	uchar_ptr = (unsigned char* ) data_ptr ;
	rest_length_byte = 4 * nof_32bit_word ;
	udp_data_copy_to_buffer_index = 0 ;
	soft_packet_number = 0;


	do {
		receive_bytes = recvfrom(this->udp_socket, this->udp_recv_data, 9000, 0,   (struct sockaddr *)&this->SIS3316_sock_addr_in, &addr_len);
		if (receive_bytes == -1) { // Timeout
			//printf("Error udp_sub_sis3316_fifo_read: Timeout\n");
			//printf("Error: recvfrom receive_bytes =  %d   \n",receive_bytes);
			//printf("receive_bytes  %d   Ack = %2x  Status = %2x  data = %2x \n",receive_bytes, (unsigned char) udp_recv_data[0], (unsigned char) udp_recv_data[1], (unsigned char) udp_recv_data[2]);
			//printf("soft_packet_number  %d   \n",soft_packet_number);
			//printf("udp_data_copy_to_buffer_index  %d   \n",udp_data_copy_to_buffer_index);
			return -1 ;
			//break ;
		}
		packet_cmd    = (unsigned int) udp_recv_data[0]  ;
		packet_status = (unsigned int) (udp_recv_data[1] & 0x70) >> 4 ;
		packet_number = (unsigned int) udp_recv_data[1] & 0xf ;
		// check Packet cmd
		if (packet_cmd != 0x30) {
			//printf("Error udp_sub_sis3316_fifo_read: packet cmd error\n");
			// packet cmd error !
		}
		if (packet_status != 0x0) {
			//printf("Error udp_sub_sis3316_fifo_read: packet status error\n");
			// packet status error !
		}
		// check Packet number
		if (packet_number != soft_packet_number) {
			//printf("Error udp_sub_sis3316_fifo_read: lost packet  error\n");
			// lost packet error !
		}
		soft_packet_number = (soft_packet_number + 1) & 0xf;

		nof_read_data_bytes = receive_bytes-2  ;
		memcpy(&uchar_ptr[udp_data_copy_to_buffer_index], &udp_recv_data[2], nof_read_data_bytes) ;
		udp_data_copy_to_buffer_index = udp_data_copy_to_buffer_index +  nof_read_data_bytes;
		rest_length_byte = rest_length_byte - nof_read_data_bytes ;
	} while (rest_length_byte > 0) ;

    return 0;
    // -1: timeout, > 0; wrong number in receive_bytes 

}

int SISEthernetInterface::udp_sis3316_fifo_read( unsigned int nof_read_words, uint32_t addr, uint32_t* data_ptr, uint32_t* got_nof_words )
{
    int error;
	int return_code;
    unsigned int rest_length_words;
    unsigned int req_nof_words;
    unsigned int data_buffer_index;

	*got_nof_words = 0x0 ;
	if (nof_read_words == 0) {
		return 0 ;
	}
	
	error = 0 ;
	rest_length_words = nof_read_words ;
	data_buffer_index = 0 ;


	do {
		if (rest_length_words >= UDP_MAX_NOF_READ_32bitWords) {
			req_nof_words = UDP_MAX_NOF_READ_32bitWords ;
		}
		else {
			req_nof_words = rest_length_words ;
		}
		return_code = this->udp_sub_sis3316_fifo_read ( req_nof_words, addr, &data_ptr[data_buffer_index]) ;
		//printf("udp_sub_sis3316_fifo_read: req_nof_words    = %d     \n", req_nof_words);

		if (return_code == -1) { // Timeout
			error = -1 ;
		}

		data_buffer_index = data_buffer_index + req_nof_words ;
		rest_length_words = rest_length_words - req_nof_words ; 


	} while ((error == 0) && (rest_length_words>0)) ;

	*got_nof_words = data_buffer_index ;
	return_code = error ;


	
    return return_code;

}

/* nof_write_words: max. 0x100 (256 32-bit words = 1KBytes )  */
#define UDP_MAX_NOF_WRITE_32bitWords    0x100

int SISEthernetInterface::udp_sub_sis3316_fifo_write( unsigned int nof_write_words, uint32_t  addr, uint32_t* data_ptr )
{
    int i;
	int return_code;
    socklen_t addr_len;
    unsigned int nof_32bit_word;
    unsigned int send_data;
  	int send_length;
 
	unsigned int udp_data_copy_to_buffer_index ;
	unsigned int nof_read_data_bytes ;
	int rest_length_byte ;
	unsigned int soft_packet_number;
	unsigned int packet_number;
	unsigned int packet_status;
	unsigned int packet_cmd;
	unsigned char* uchar_ptr;
	int receive_bytes;

	//unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);

	nof_32bit_word = nof_write_words ;
	if (nof_write_words == 0) {
		return 0 ;
		//nof_32bit_word = 1 ;
	}
	if (nof_write_words > UDP_MAX_NOF_WRITE_32bitWords) {
		nof_32bit_word = UDP_MAX_NOF_WRITE_32bitWords ;
	}
	
	// send Read Req Cmd
	this->udp_send_data[0] = 0x31 ; // send SIS3316 Fifo Write Req Cmd
    this->udp_send_data[1] = (unsigned char)  ((nof_32bit_word-1) & 0xff);           //  lower length
    this->udp_send_data[2] = (unsigned char) (((nof_32bit_word-1) >>  8) & 0xff);    //  upper length 

	this->udp_send_data[3] = (unsigned char)  (addr & 0xff) ;        // address(7 dwonto 0)
	this->udp_send_data[4] = (unsigned char) ((addr >>  8) & 0xff) ; // address(15 dwonto 8)
	this->udp_send_data[5] = (unsigned char) ((addr >> 16) & 0xff) ; // address(23 dwonto 16)
	this->udp_send_data[6] = (unsigned char) ((addr >> 24) & 0xff) ; // address(31 dwonto 24)


	send_length = 7 + (4*nof_32bit_word);
	for (i=0;i<nof_32bit_word;i++) {
		send_data = data_ptr[i] ;
		this->udp_send_data[7+(4*i)] = (unsigned char)  (send_data & 0xff) ;        // send_data(7 dwonto 0)
		this->udp_send_data[8+(4*i)] = (unsigned char) ((send_data >>  8) & 0xff) ; // send_data(15 dwonto 8)
		this->udp_send_data[9+(4*i)] = (unsigned char) ((send_data >> 16) & 0xff) ; // send_data(23 dwonto 16)
		this->udp_send_data[10+(4*i)] = (unsigned char) ((send_data >> 24) & 0xff) ; // send_data(31 dwonto 24)
	}

    return_code = sendto(this->udp_socket, udp_send_data, send_length, 0, (struct sockaddr *)&this->SIS3316_sock_addr_in, sizeof(struct sockaddr));
	if (return_code != send_length) {
		//printf("sendto: return_code = 0x%08x \n",return_code);
	}

	return_code = recvfrom(udp_socket, udp_recv_data, 512, 0,  (struct sockaddr *)&this->SIS3316_sock_addr_in, &addr_len );
    if(return_code != 2 ) {
		//printf("recvfrom: return_code = 0x%08x \n",return_code);
		return -1; 
    }

	packet_cmd    = (unsigned int) udp_recv_data[0]  ;
	packet_status = (unsigned int) (udp_recv_data[1] & 0x70) >> 4 ;
	if (packet_cmd != 0x31) {
		// packet cmd error !
		return -2; 
	}
	if (packet_status != 0x0) {
		return -3; 
		// packet status error !
	}

    return 0; 

}

int SISEthernetInterface::udp_sis3316_fifo_write( unsigned int nof_write_words, uint32_t addr, uint32_t* data_ptr, uint32_t* written_nof_words )
{
    int error;
	int return_code;
    unsigned int rest_length_words;
    unsigned int req_nof_words;
    unsigned int data_buffer_index;

	*written_nof_words = 0x0 ;
	if (nof_write_words == 0) {
		return 0 ;
	}
	
	error = 0 ;
	rest_length_words = nof_write_words ;
	data_buffer_index = 0 ;

			//printf("receive_bytes  %d   Ack = %2x  Status = %2x  data = %2x \n",receive_bytes, (unsigned char) udp_recv_data[0], (unsigned char) udp_recv_data[1], (unsigned char) udp_recv_data[2]);

	do {
		if (rest_length_words >= UDP_MAX_NOF_WRITE_32bitWords) {
			req_nof_words = UDP_MAX_NOF_WRITE_32bitWords ;
		}
		else {
			req_nof_words = rest_length_words ;
		}
		return_code = this->udp_sub_sis3316_fifo_write ( req_nof_words, addr, &data_ptr[data_buffer_index]) ;

		if (return_code == -1) { // Timeout
			error = -1 ;
		}

		data_buffer_index = data_buffer_index + req_nof_words ;
		rest_length_words = rest_length_words - req_nof_words ; 


	} while ((error == 0) && (rest_length_words>0)) ;

	*written_nof_words = data_buffer_index ;
	return_code = error ;

if (return_code != 0) {
	//printf("udp_sis3316_fifo_write: return_code = 0x%08x \n",return_code);
}

    return return_code;

}

/*************************************************************************************************************************************************/
/*************************************************************************************************************************************************/
/***                                                                                                                                           ***/
/***     "emulate" VME access routines                                                                                                         ***/
/***                                                                                                                                           ***/
/*************************************************************************************************************************************************/
/*************************************************************************************************************************************************/

int SISEthernetInterface::vme_A32D32_read( uint32_t addr, uint32_t* data )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr & 0x00ffffff ; // only sis3316 address range

	if (udp_address < 0x20) {
		return_code = this->udp_register_read(udp_address, data)  ;
		if(return_code == 0) {
			return_code = 0;
		}
		else {
			return_code = 0x211;
		}
	}
	else {
		return_code = this->udp_sis3316_register_read(1, &udp_address, data)  ;
		if(return_code == 0) {
			return_code = 0;
		}
		else {
			return_code = 0x211;
		}
	}
	return return_code;
}

int SISEthernetInterface::vme_A32DMA_D32_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32BLT32_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_read(request_nof_words, udp_address, data, got_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32MBLT64_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_read(request_nof_words, udp_address, data, got_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32_2EVME_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32_2ESST160_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32_2ESST267_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32_2ESST320_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32DMA_D32FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32BLT32FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_read(request_nof_words, udp_address, data, got_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32MBLT64FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_read(request_nof_words, udp_address, data, got_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32_2EVMEFIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32_2ESST160FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32_2ESST267FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32_2ESST320FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32D32_write( uint32_t addr, uint32_t data )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	if (udp_address < 0x20) {
		return_code = this->udp_register_write(udp_address, data)  ;
		if(return_code == 0) {
			return_code = 0;
		}
		else {
			return_code = 0x211;
		}
	}
	else {
		return_code = this->udp_sis3316_register_write(1, &udp_address, &data)  ;
		if(return_code == 0) {
			return_code = 0;
		}
		else {
			return_code = 0x211;
		}
	}
	return return_code;
}

int SISEthernetInterface::vme_A32DMA_D32_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words )
{
	return -1; // not implemented
}
/**************************************************************************************/

int SISEthernetInterface::vme_A32BLT32_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	//printf("vme_A32BLT32_write =  %d   \n",request_nof_words);

	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_write(request_nof_words, udp_address, data, written_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32MBLT64_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	//printf("vme_A32MBLT64_write =  %d   \n",request_nof_words);
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_write(request_nof_words, udp_address, data, written_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32DMA_D32FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_A32BLT32FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	//printf("vme_A32BLT32FIFO_write =  %d   \n",request_nof_words);
	udp_address = addr & 0x00ffffff ; // only sis3316 address range
	return_code = this->udp_sis3316_fifo_write(request_nof_words, udp_address, data, written_nof_words)  ;
	if(return_code == 0) {
		return_code = 0;
	}
	else {
		return_code = 0x211;
	}
	return return_code; // 
}

int SISEthernetInterface::vme_A32MBLT64FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words )
{
	return -1; // not implemented
}

int SISEthernetInterface::vme_IRQ_Status_read( uint32_t* data ) 
{
	return -1; // not implemented
}