#ifndef VMEINTERFACE_H
#define VMEINTERFACE_H

#include <stdint.h>

#define DEF_VME_READ_MODE_D32				0  
#define DEF_VME_READ_MODE_D32_DMA			1  
#define DEF_VME_READ_MODE_BLT32				2  
#define DEF_VME_READ_MODE_MBLT64			3  
#define DEF_VME_READ_MODE_2EVME				4  
#define DEF_VME_READ_MODE_2ESST160			5  
#define DEF_VME_READ_MODE_2ESST267			6  
#define DEF_VME_READ_MODE_2ESST320			7  

class VMEInterface {

public:
	virtual int vmeopen( void ) = 0;
	virtual int vmeclose( void ) = 0;

	virtual int get_vmeopen_messages( char* messages, uint32_t* nof_found_devices ) = 0;

	virtual int vme_A32D32_read( uint32_t addr, uint32_t* data ) = 0;

	virtual int vme_A32DMA_D32_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32BLT32_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32MBLT64_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2EVME_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2ESST160_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2ESST267_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2ESST320_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;

	virtual int vme_A32DMA_D32FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32BLT32FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32MBLT64FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2EVMEFIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2ESST160FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2ESST267FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;
	virtual int vme_A32_2ESST320FIFO_read( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* got_nof_words ) = 0;


	virtual int vme_A32D32_write( uint32_t addr, uint32_t data ) = 0;
	virtual int vme_A32DMA_D32_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words ) = 0;
	virtual int vme_A32BLT32_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words ) = 0;
	virtual int vme_A32MBLT64_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words ) = 0;

	virtual int vme_A32DMA_D32FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words ) = 0;
	virtual int vme_A32BLT32FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words ) = 0;
	virtual int vme_A32MBLT64FIFO_write( uint32_t addr, uint32_t* data, uint32_t request_nof_words, uint32_t* written_nof_words ) = 0;

	virtual int vme_IRQ_Status_read( uint32_t* data ) = 0;

//	virtual int read( uint32_t *buf, uint32_t len ) = 0;
//	virtual int write( uint32_t *buf, uint32_t len ) = 0;

};

#endif // VMEINTERFACE_H_INCLUDED

