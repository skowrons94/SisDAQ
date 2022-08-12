#ifndef SISINTERFACE_H
#define SISINTERFACE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "Registers.h"

#include "VMEInterface.h"

#define SIS3316_ADC_FPGA_BOOT_CSR		0x30
#define SIS3316_SPI_FLASH_CSR			0x34
#define ENABLE_SPI_PROG	                0
#define CHIPSELECT_1		            1
#define CHIPSELECT_2		            2
#define FIFO_NOT_EMPTY		            14
#define FLASH_LOGIC_BUSY	            31

#define SIS3316_SPI_FLASH_DATA			0x38
#define SIS3316_FLASH_PROGRAM_PAGESIZE	256
#define SIS3316_FLASH_ERASE_BLOCKSIZE	65536

#define SIS3316_ADC_CLK_OSC_I2C_REG		0x40
#define I2C_ACK			                8
#define I2C_START			            9
#define I2C_REP_START		            10
#define I2C_STOP			            11
#define I2C_WRITE			            12
#define I2C_READ			            13
#define I2C_BUSY			            31

#define OSC_ADR	                        0x55

#define SI5325_SPI_POLL_COUNTER_MAX							100
#define SI5325_SPI_CALIBRATION_READY_POLL_COUNTER_MAX		1000


class SISInterface
{
	VMEInterface* i;
	unsigned int  baseaddress;

private:
	int FlashEnableCS( int chip );
	int FlashDisableCS( int chip );
	int FlashWriteEnable( void );
	int FlashWriteDisable( void );
	int FlashProgramPage( int address, char* data, int len );
	int FlashEraseBlock( int address );
	int FlashXfer( char in, char* out );
	
	int I2cStart( int osc );
	int I2cStop( int osc );
	int I2cWriteByte( int osc, unsigned char data, char* ack );
	int I2cReadByte( int osc, unsigned char* data, char ack );
	int Si570FreezeDCO( int osc );
	int Si570Divider( int osc, unsigned char* data );
	int Si570UnfreezeDCO( int osc );
	int Si570NewFreq( int osc );
	int Si570ReadDivider( int osc, unsigned char* data );

	int si5325_clk_muliplier_write( unsigned int addr, unsigned int data );
	int si5325_clk_muliplier_read( unsigned int addr, unsigned int *data ); 
	int si5325_clk_muliplier_internal_calibration_cmd( void );

public:
	unsigned char freqSI570_calibrated_value_125MHz[6];
	unsigned char freqPreset62_5MHz[6];
	unsigned char freqPreset125MHz[6];
	unsigned char freqPreset250MHz[6];

	unsigned int adc_125MHz_flag;

public:
	SISInterface( VMEInterface* crate, unsigned int baseaddress );

	int register_read( uint32_t addr, uint32_t* data );
	int register_write( uint32_t addr, uint32_t data );

	int update_firmware( char* path, int offset, void (*cb)(int percentage) );
	int FlashRead( int address, char* data, int len );
	int FlashReadStatus1( char* status );
	int FlashReadStatus2( char* status );
	int FlashEnableProg( void );
	int FlashDisableProg( void );

	int change_frequency_HSdiv_N1div( int osc, unsigned hs_div_val, unsigned n1_div_val );
	int get_frequency( int osc, unsigned char* values );
	int set_frequency( int osc, unsigned char* values );
	int set_external_clock_multiplier( unsigned int bw_sel, unsigned int n1_hs, unsigned int n1_clk1, unsigned int n1_clk2, unsigned int n2, unsigned int n3, unsigned int clkin1_mhz );
	int bypass_external_clock_multiplier( void );
	int get_status_external_clock_multiplier( unsigned int* status );

	int FlashGetId( char* id );

	int adc_spi_setup( void );
	int adc_spi_read( unsigned int adc_fpga_group, unsigned int adc_chip, unsigned int spi_addr, unsigned int* spi_data );
	int adc_spi_write( unsigned int adc_fpga_group, unsigned int adc_chip, unsigned int spi_addr, unsigned int spi_data );

	int read_DMA_Channel_PreviousBankDataBuffer( unsigned int bank2_read_flag, unsigned int channel_no,
			unsigned int  max_read_nof_words, unsigned int*  dma_got_no_of_words, unsigned int* uint_adc_buffer );

	int read_MBLT64_Channel_PreviousBankDataBuffer( unsigned int bank2_read_flag, unsigned int channel_no, 
							 unsigned int*  dma_got_no_of_words, unsigned int* uint_adc_buffer );

	int read_Channel_EnergyHistogramBuffer( unsigned int channel_no, unsigned int lenght, unsigned int* uint_adc_buffer );

	int read_Channel_TofHistogramBuffer( unsigned int channel_no, int histogram_index, unsigned int lenght, unsigned int* uint_buffer );
	int read_Channel_ShapeHistogramBuffer( unsigned int channel_no, int histogram_index, unsigned int lenght, unsigned int* uint_buffer );
	int read_Channel_ChargeHistogramBuffer( unsigned int channel_no, int histogram_index, unsigned int lenght, unsigned int* uint_buffer );

	~SISInterface( void );
};

#endif // SISINTERFACE_H_INCLUDED
