#include "SISInterface.h"

SISInterface::SISInterface( VMEInterface* crate, unsigned int baseaddress )
{
	int return_code ;
	unsigned int data ;
	if(crate){
		this->i = crate;
	}
	this->baseaddress = baseaddress;

	this->get_frequency(0,freqSI570_calibrated_value_125MHz );

	// frequency presets setup
	freqPreset62_5MHz[0] = 0x23;
	freqPreset62_5MHz[1] = (0x3 << 6) + (freqSI570_calibrated_value_125MHz[1] & 0x3F);
	freqPreset62_5MHz[2] = freqSI570_calibrated_value_125MHz[2];
	freqPreset62_5MHz[3] = freqSI570_calibrated_value_125MHz[3];
	freqPreset62_5MHz[4] = freqSI570_calibrated_value_125MHz[4];
	freqPreset62_5MHz[5] = freqSI570_calibrated_value_125MHz[5];

	freqPreset125MHz[0] = 0x21;
	freqPreset125MHz[1] = (0x3 << 6) + (freqSI570_calibrated_value_125MHz[1] & 0x3F);
	freqPreset125MHz[2] = freqSI570_calibrated_value_125MHz[2];
	freqPreset125MHz[3] = freqSI570_calibrated_value_125MHz[3];
	freqPreset125MHz[4] = freqSI570_calibrated_value_125MHz[4];
	freqPreset125MHz[5] = freqSI570_calibrated_value_125MHz[5];

	freqPreset250MHz[0] = 0x20;
	freqPreset250MHz[1] = (0x3 << 6) + (freqSI570_calibrated_value_125MHz[1] & 0x3F);
	freqPreset250MHz[2] = freqSI570_calibrated_value_125MHz[2];
	freqPreset250MHz[3] = freqSI570_calibrated_value_125MHz[3];
	freqPreset250MHz[4] = freqSI570_calibrated_value_125MHz[4];
	freqPreset250MHz[5] = freqSI570_calibrated_value_125MHz[5];


	this->adc_125MHz_flag = 0;
	return_code=this->adc_spi_setup() ;
	if (return_code != 0) {
		printf("Error adc_spi_setup: return_code = 0x%08x \n", return_code);
	}
	//printf("adc_spi_setup: adc_125MHz_flag = %d \n", this->adc_125MHz_flag);

#ifdef NOT_USED
	this->adc_spi_read( 0, 0, 1, &data);
	if ((data&0xff) == 0x32) {
		this->adc_125MHz_flag = 1;
	}
#endif
	//this->adc_spi_read( 0, 1, 1, &data);
	//this->adc_spi_read( unsigned int adc_fpga_group, unsigned int adc_chip, unsigned int spi_addr, unsigned int* spi_data);

}

int SISInterface::register_read( uint32_t addr, uint32_t* data )
{
int return_code ;
	return_code = this->i->vme_A32D32_read (this->baseaddress+addr, data);	
	return return_code;
}

int SISInterface::register_write( uint32_t addr, uint32_t data)
{
int return_code ;
	return_code = this->i->vme_A32D32_write (this->baseaddress+addr, data);	
	return return_code;
}

int SISInterface::update_firmware( char* path, int offset, void (*cb)(int percentage) )
{
	int rc;
	FILE *fp;
	char *buf;
	int fileSize;
	int percent, percent_old;

	if(path == NULL){
		return -100;
	}

	fp = fopen(path, "rb");
	if(fp == NULL){
		return -101;
	}

	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	rewind(fp);

	buf = (char *)malloc(fileSize);
	if(buf == NULL){
		return -102;
	}

	rc = fread(buf, 1, fileSize, fp);
	if(rc != fileSize){
		return -103;
	}

	fclose(fp);

	percent = percent_old = 0;
	if(cb){
		(cb)(percent);
	}

	int written = 0;
	int pageProgramSize;

	this->FlashEnableProg();

	while(written < fileSize){
		// erase
		if((written & (SIS3316_FLASH_ERASE_BLOCKSIZE - 1)) == 0){
			rc = this->FlashEraseBlock((offset + written) & 0xFFFF0000);
		}

		if(fileSize >= (written + SIS3316_FLASH_PROGRAM_PAGESIZE)){
			pageProgramSize = SIS3316_FLASH_PROGRAM_PAGESIZE;
		}else{
			pageProgramSize = fileSize - written;
		}

		rc = this->FlashProgramPage(offset + written, buf + written, pageProgramSize);

		written += pageProgramSize;

		if(cb){
			percent = written * 100 / fileSize;
			if(percent != percent_old){
				(cb)(percent);
				percent_old = percent;
			}
		}
	}
	
	this->FlashDisableProg();

	free(buf);

	return 0;
}

int SISInterface::FlashEnableProg( )
{
	uint32_t tmp;
	int rc;

	rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_CSR, &tmp);
	if(rc){
		return rc;
	}

	tmp |= (1<<ENABLE_SPI_PROG);

	rc = i->vme_A32D32_write(this->baseaddress + SIS3316_SPI_FLASH_CSR, tmp);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashDisableProg( )
{
	uint32_t tmp;
	int rc;

	rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_CSR, &tmp);
	if(rc){
		return rc;
	}

	tmp &= ~(1<<ENABLE_SPI_PROG);

	rc = i->vme_A32D32_write(this->baseaddress + SIS3316_SPI_FLASH_CSR, tmp);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashEnableCS( int chip )
{
	uint32_t tmp;
	int rc;

	rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_CSR, &tmp);
	if(rc){
		return rc;
	}

	switch(chip){
	case 0:
		tmp |= (1<<CHIPSELECT_1);
		break;
	case 1:
		tmp |= (1<<CHIPSELECT_2);
		break;
	default:
		return -100;
	}

	rc = i->vme_A32D32_write(this->baseaddress + SIS3316_SPI_FLASH_CSR, tmp);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashDisableCS( int chip )
{
	uint32_t tmp;
	int rc;

	rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_CSR, &tmp);
	if(rc){
		return rc;
	}

	switch(chip){
	case 0:
		tmp &= ~(1<<CHIPSELECT_1);
		break;
	case 1:
		tmp &= ~(1<<CHIPSELECT_2);
		break;
	default:
		return -100;
	}

	rc = i->vme_A32D32_write(this->baseaddress + SIS3316_SPI_FLASH_CSR, tmp);
	if(rc){
		return rc;
	}

	return 0;
}

/***************************************************************/


int SISInterface::FlashWriteEnable( )
{
	int rc;
	
	rc = this->FlashEnableCS(0);
	if(rc){
		return rc;
	}
	
	rc = this->FlashXfer(0x06, NULL); // Write Enable command
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashWriteDisable( )
{
	int rc;
	
	rc = this->FlashEnableCS(0);
	if(rc){
		return rc;
	}
	
	rc = this->FlashXfer(0x04, NULL); // Write Enable command
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashRead( int address, char* data, int len )
{
	int rc;
	char tmp;

	if(data == NULL){
		return -100;
	}
	
	//rc = this->FlashWriteEnable();
	//if(rc){
	//	return rc;
	//}

	// program command

	//this->FlashDisableCS(0);
	//this->FlashEnableProg();

	this->FlashWriteDisable();

	// program command
	rc = this->FlashEnableCS(0);


	rc = this->FlashEnableCS(0);
	if(rc){
		return rc;
	}
	
	rc = this->FlashXfer(0x03, NULL); // Read program command
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

 
	rc = this->FlashXfer((char)(address >> 16), NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)(address >> 8), NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)address, NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

 

	char char_temp ;
	char_temp=0;
	for(int i = 0;i < len && i < SIS3316_FLASH_PROGRAM_PAGESIZE;i++){
		rc = this->FlashXfer(char_temp, &tmp);
		data[i] = tmp ;
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}
	}
 
	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashProgramPage( int address, char* data, int len )
{
	int rc;
	char tmp;
	uint32_t utmp;
	uint32_t dmabuf[SIS3316_FLASH_PROGRAM_PAGESIZE];
	uint32_t putWords;

	if(data == NULL){
		return -100;
	}
	
	rc = this->FlashWriteEnable();
	if(rc){
		return rc;
	}

	// program command
	rc = this->FlashEnableCS(0);
	if(rc){
		return rc;
	}
	
	rc = this->FlashXfer(0x02, NULL); // Page program command
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)(address >> 16), NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)(address >> 8), NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)address, NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	// dma d32
	for(int k = 0;k < SIS3316_FLASH_PROGRAM_PAGESIZE;k++){
		dmabuf[k] = (uint32_t)*(data + k);
	}
	rc = i->vme_A32DMA_D32FIFO_write(this->baseaddress + SIS3316_SPI_FLASH_DATA, dmabuf, SIS3316_FLASH_PROGRAM_PAGESIZE, &putWords);
	if(rc || putWords != SIS3316_FLASH_PROGRAM_PAGESIZE){
		return -101;
	}

	// busy polling
	do{
		rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_CSR, &utmp);
		if(rc){
			return rc;
		}

		utmp &= (1<<FLASH_LOGIC_BUSY^1<<FIFO_NOT_EMPTY);

		usleep(1000); // testing

	}while(utmp != 0);

	// single cycles
#if 0
	for(int i = 0;i < len && i < SIS3316_FLASH_PROGRAM_PAGESIZE;i++){
		rc = this->FlashXfer(*(data + i), NULL);
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}
	}
#endif

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	usleep(1000); // testing
	// busy polling
	do{
		rc = this->FlashEnableCS(0);
		if(rc){
			return rc;
		}
	
		rc = this->FlashXfer(0x05, NULL); // read status register 1 command
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}
		rc = this->FlashXfer(0, &tmp);
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}

		tmp &= 1;

		rc = this->FlashDisableCS(0);
		if(rc){
			return rc;
		}
		usleep(1000); // testing
	}while(tmp);

	return 0;
}

int SISInterface::FlashEraseBlock( int address )
{
	int rc;
	char tmp;
	
	rc = this->FlashWriteEnable();
	if(rc){
		return rc;
	}

	// erase command
	rc = this->FlashEnableCS(0);
	if(rc){
		return rc;
	}
	
	rc = this->FlashXfer((char)0xD8, NULL); // 64kB Block erase command
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)(address >> 16), NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)(address >> 8), NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer((char)address, NULL);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}
	usleep(1000); // testing
	// busy polling
	do{
		rc = this->FlashEnableCS(0);
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}
	
		rc = this->FlashXfer(0x05, NULL); // read status register 1 command
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}
		rc = this->FlashXfer(0, &tmp);
		if(rc){
			this->FlashDisableCS(0);
			return rc;
		}

		tmp &= 1;

		rc = this->FlashDisableCS(0);
		if(rc){
			return rc;
		}
		usleep(1000); // testing
	}while(tmp);

	return 0;
}

int SISInterface::FlashXfer( char in, char* out )
{
	uint32_t tmp;
	char ctmp;
	int rc;

	tmp = in;
	rc = i->vme_A32D32_write(this->baseaddress + SIS3316_SPI_FLASH_DATA, tmp);
	if(rc){
		return rc;
	}
 
	do{
		rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_CSR, &tmp);
		if(rc){
			return rc;
		}

		tmp &= (1<<FLASH_LOGIC_BUSY^1<<FIFO_NOT_EMPTY);

		//usleep(1000); // testing

	}while(tmp != 0);
 
	rc = i->vme_A32D32_read(this->baseaddress + SIS3316_SPI_FLASH_DATA, &tmp);
	if(rc){
		return rc;
	}

	ctmp = tmp & 0xFF;
	if(out){
		*out = ctmp;
	}

	return 0;
}

int SISInterface::FlashGetId( char* id )
{
	int rc;

	if(id == NULL){
		return -100;
	}

	rc = this->FlashEnableCS(0);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	
	rc = this->FlashXfer((char)0x9F, NULL); // JEDEC ID command
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer(0, id);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer(0, id+1);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer(0, id+2);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashReadStatus1( char* status )
{
	int rc;

	if(status == NULL){
		return -100;
	}

	rc = this->FlashEnableCS(0);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	
	rc = this->FlashXfer((char)0x05, NULL); //  
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer(0, status);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::FlashReadStatus2( char* status )
{
	int rc;

	if(status == NULL){
		return -100;
	}

	rc = this->FlashEnableCS(0);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	
	rc = this->FlashXfer((char)0x35, NULL); //  
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}
	rc = this->FlashXfer(0, status);
	if(rc){
		this->FlashDisableCS(0);
		return rc;
	}

	rc = this->FlashDisableCS(0);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::I2cStart( int osc )
{
	int rc;
	int i;
	uint32_t tmp;

	if(osc > 3){
		return -101;
	}

	// start
	rc = this->i->vme_A32D32_write(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), 1<<I2C_START);
	if(rc){
		return rc;
	}

	i = 0;
	do{
		// poll i2c fsm busy
		rc = this->i->vme_A32D32_read(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), &tmp);
		if(rc){
			return rc;
		}
		i++;
	}while((tmp & (1<<I2C_BUSY)) && (i < 1000));

	// register access problem
	if(i == 1000){
		return -100;
	}

	return 0;
}

int SISInterface::I2cStop( int osc )
{
	int rc;
	int i;
	uint32_t tmp;

	if(osc > 3){
		return -101;
	}

	// stop
	rc = this->i->vme_A32D32_write(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), 1<<I2C_STOP);
	if(rc){
		return rc;
	}
	
	i = 0;
	do{
		// poll i2c fsm busy
		rc = this->i->vme_A32D32_read(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), &tmp);
		if(rc){
			return rc;
		}
		i++;
	}while((tmp & (1<<I2C_BUSY)) && (i < 1000));

	// register access problem
	if(i == 1000){
		return -100;
	}

	return 0;
}

int SISInterface::I2cWriteByte( int osc, unsigned char data, char* ack )
{
	int rc;
	int i;
	uint32_t tmp;
	
	if(osc > 3){
		return -101;
	}

	// write byte, receive ack
	rc = this->i->vme_A32D32_write(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), 1<<I2C_WRITE ^ data);
	if(rc){
		return rc;
	}
	
	i = 0;
	do{
		// poll i2c fsm busy
		rc = this->i->vme_A32D32_read(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), &tmp);
		if(rc){
			return rc;
		}
		i++;
	}while((tmp & (1<<I2C_BUSY)) && (i < 1000));

	// register access problem
	if(i == 1000){
		return -100;
	}

	// return ack value?
	if(ack){
		// yup
		*ack = tmp & 1<<I2C_ACK ? 1 : 0;
	}

	return 0;
}

int SISInterface::I2cReadByte( int osc, unsigned char* data, char ack )
{
	int rc;
	int i;
	uint32_t tmp;
	unsigned char char_tmp;	
	if(osc > 3){
		return -101;
	}

	// read byte, put ack
	tmp = 1<<I2C_READ;
	tmp |= ack ? 1<<I2C_ACK : 0;
	rc = this->i->vme_A32D32_write(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), tmp);
	if(rc){
		return rc;
	}
	
	i = 0;
	do{
		// poll i2c fsm busy
		rc = this->i->vme_A32D32_read(this->baseaddress + SIS3316_ADC_CLK_OSC_I2C_REG + (4 * osc), &tmp);
		if(rc){
			return rc;
		}
		i++;
	}while((tmp & (1<<I2C_BUSY)) && (i < 1000));

	// register access problem
	if(i == 1000){
		return -100;
	}
	char_tmp = (unsigned char) (tmp & 0xff) ;
	*data = char_tmp ;
	return 0;
}

int SISInterface::Si570FreezeDCO( int osc )
{
	int rc;
	char ack;

	// start
	rc = this->I2cStart(osc);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	// address
	rc = this->I2cWriteByte(osc, OSC_ADR<<1, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// register offset
	rc = this->I2cWriteByte(osc, 0x89, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// write data
	rc = this->I2cWriteByte(osc, 0x10, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// stop
	rc = this->I2cStop(osc);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::Si570ReadDivider( int osc, unsigned char* data )
{
	int rc;
	char ack;
	int i;

	// start
	rc = this->I2cStart(osc);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	// address
	rc = this->I2cWriteByte(osc, OSC_ADR<<1, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// register offset
	rc = this->I2cWriteByte(osc, 0x0D, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}


	rc = this->I2cStart(osc);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	// address + 1
	rc = this->I2cWriteByte(osc, (OSC_ADR<<1) + 1, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}


	// read data
	for(i = 0;i < 6;i++){
		ack = 1 ;
		if (i==5) {ack = 0;}
		rc = this->I2cReadByte(osc, &data[i], ack);
		if(rc){
			this->I2cStop(osc);
			return rc;
		}

	}

	// stop
	rc = this->I2cStop(osc);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::Si570Divider( int osc, unsigned char* data )
{
	int rc;
	char ack;
	int i;

	// start
	rc = this->I2cStart(osc);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	// address
	rc = this->I2cWriteByte(osc, OSC_ADR<<1, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// register offset
	rc = this->I2cWriteByte(osc, 0x0D, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// write data
	for(i = 0;i < 2;i++){
		rc = this->I2cWriteByte(osc, data[i], &ack);
		if(rc){
			this->I2cStop(osc);
			return rc;
		}

		if(!ack){
			this->I2cStop(osc);
			return -101;
		}
	}

	// stop
	rc = this->I2cStop(osc);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::Si570UnfreezeDCO( int osc )
{
	int rc;
	char ack;

	// start
	rc = this->I2cStart(osc);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	// address
	rc = this->I2cWriteByte(osc, OSC_ADR<<1, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// register offset
	rc = this->I2cWriteByte(osc, 0x89, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// write data
	rc = this->I2cWriteByte(osc, 0x00, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// stop
	rc = this->I2cStop(osc);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::Si570NewFreq( int osc )
{
	int rc;
	char ack;
	// start
	rc = this->I2cStart(osc);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	// address
	rc = this->I2cWriteByte(osc, OSC_ADR<<1, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}
	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// register offset
	rc = this->I2cWriteByte(osc, 0x87, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// write data
	rc = this->I2cWriteByte(osc, 0x40, &ack);
	if(rc){
		this->I2cStop(osc);
		return rc;
	}

	if(!ack){
		this->I2cStop(osc);
		return -101;
	}

	// stop
	rc = this->I2cStop(osc);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::set_frequency( int osc, unsigned char* values )
{
	int rc;

	if(values == NULL){
		return -100;
	}
	if(osc > 3 || osc < 0){
		return -100;
	}

	rc = this->Si570FreezeDCO(osc);
	if(rc){
		return rc;
	}

	rc = this->Si570Divider(osc, values);
	if(rc){
		return rc;
	}

	rc = this->Si570UnfreezeDCO(osc);
	if(rc){
		return rc;
	}

	rc = this->Si570NewFreq(osc);
	if(rc){
		return rc;
	}

// min. 10ms wait
	usleep(20000);

	// DCM Reset
	rc = this->i->vme_A32D32_write(this->baseaddress + 0x438 , 0);
	if(rc){
		return rc;
	}


	return 0;
}

/****************************************************************************************************/
/*                                                                                                  */
/*     change_frequency_HSdiv_N1div                                                                 */
/*     hs_div_val: allowed values are [4, 5, 6, 7, 9, 11]                                           */
/*     n1_div_val: allowed values are [2, 4, 6, .... 124, 126]                                      */
/*     Sample Frequence =  5 GHz / (hs_div_val * n1_div_val)                                        */
/*                                                                                                  */
/*     example:                                                                                     */
/*     hs_div_val = 5                                                                               */
/*     n1_div_val = 4                                                                               */
/*     Sample Frequence =  5 GHz / 20 = 250 MHz                                                     */
/*                                                                                                  */
/****************************************************************************************************/

int SISInterface::change_frequency_HSdiv_N1div( int osc, unsigned hs_div_val, unsigned n1_div_val )
{
	int rc;
	unsigned i ;
	unsigned N1div ;
	unsigned HSdiv ;
	unsigned HSdiv_reg[6] ;
	unsigned HSdiv_val[6] ;
	unsigned char freqSI570_high_speed_rd_value[6];
	unsigned char freqSI570_high_speed_wr_value[6];
	
	if(osc > 3 || osc < 0){
		return -100;
	}

	HSdiv_reg[0] =  0 ;
	HSdiv_val[0] =  4 ;

	HSdiv_reg[1] =  1 ;
	HSdiv_val[1] =  5 ;

	HSdiv_reg[2] =  2 ;
	HSdiv_val[2] =  6 ;

	HSdiv_reg[3] =  3 ;
	HSdiv_val[3] =  7 ;

	HSdiv_reg[4] =  5 ;
	HSdiv_val[4] =  9 ;

	HSdiv_reg[5] =  7 ;
	HSdiv_val[5] =  11 ;

	HSdiv = 0xff ;
	for (i=0;i<6;i++){
		if (HSdiv_val[i] == hs_div_val) {
			HSdiv = HSdiv_reg[i] ;
		}
	}
	if (HSdiv > 11) {
		return -101;
	}

	// gt than 127 or odd then return
	if((n1_div_val > 127) || ((n1_div_val & 0x1) == 1) || (n1_div_val == 0) ) {
		return -102;
	}
	N1div = n1_div_val - 1 ;

	rc = this->Si570ReadDivider(osc, freqSI570_high_speed_rd_value);
	if(rc){
		printf("Si570ReadDivider = %d \n",rc);
		return rc;
	}
	freqSI570_high_speed_wr_value[0] = ((HSdiv & 0x7) << 5) + ((N1div & 0x7c) >> 2);
	freqSI570_high_speed_wr_value[1] = ((N1div & 0x3) << 6) + (freqSI570_high_speed_rd_value[1] & 0x3F);
	freqSI570_high_speed_wr_value[2] = freqSI570_high_speed_rd_value[2];
	freqSI570_high_speed_wr_value[3] = freqSI570_high_speed_rd_value[3];
	freqSI570_high_speed_wr_value[4] = freqSI570_high_speed_rd_value[4];
	freqSI570_high_speed_wr_value[5] = freqSI570_high_speed_rd_value[5];


	rc = this->set_frequency(osc, freqSI570_high_speed_wr_value);
	if(rc){
		printf("set_frequency = %d \n",rc);
		return rc;
	}

	return 0;
}

int SISInterface::get_frequency(int osc, unsigned char *values)
{
	int rc;

	if(values == NULL){
		return -100;
	}
	if(osc > 3 || osc < 0){
		return -100;
	}
	rc = this->Si570ReadDivider(osc, values);
	if(rc){
		return rc;
	}

	return 0;
}

int SISInterface::si5325_clk_muliplier_write( unsigned int addr, unsigned int data ) 
{
	unsigned int return_code ;
	unsigned int write_data, read_data ;
	unsigned int poll_counter   ;
// write address
	write_data = 0x0000 + (addr & 0xff) ; // write ADDR Instruction + register addr
	return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
	if (return_code != 0) {	return -1 ;	}
		usleep(10000) ;

	poll_counter = 0 ;
	do {
		poll_counter++;
		this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
	} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
	if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}
		usleep(10000) ;

// write data
	write_data = 0x4000 + (data & 0xff) ; // write Instruction + data
	return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
	if (return_code != 0) {	return -1 ;	}
		usleep(10000) ;

	poll_counter = 0 ;
	do {
		poll_counter++;
		this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
	} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
	if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}
	
	return 0 ;
}

int SISInterface::si5325_clk_muliplier_read(unsigned int addr, unsigned int* data)   
{
	unsigned int return_code ;
	unsigned int write_data, read_data ;
	unsigned int poll_counter   ;
// read address
	write_data = 0x0000 + (addr & 0xff) ; // read ADDR Instruction + register addr
	return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
	if (return_code != 0) {	return -1 ;	}

	poll_counter = 0 ;
	do {
		poll_counter++;
		this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
	} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
	if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}
		usleep(10000) ;

// read data
	write_data = 0x8000  ; // read Instruction + data
	return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
	if (return_code != 0) {	return -1 ;	}
		usleep(10000) ;

	poll_counter = 0 ;
	do {
		poll_counter++;
		this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
	} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
	if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}
	//*data = (read_data & 0xff) ;
	*data = (read_data ) ;
	return (0) ;
}

int SISInterface::si5325_clk_muliplier_internal_calibration_cmd( void )  
{
	unsigned int return_code ;
	unsigned int write_data, read_data ;
	unsigned int poll_counter, cal_poll_counter  ;
// write address
	write_data = 0x0000 + 136 ; // write ADDR Instruction + register addr
	return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
	if (return_code != 0) {	return -1 ;	}

	poll_counter = 0 ;
	do {
		poll_counter++;
		this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
	} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
	if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}

// write data
	write_data = 0x4000 + 0x40 ; // write Instruction + data
	return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
	if (return_code != 0) {	return -1 ;	}

	poll_counter = 0 ;
	do {
		poll_counter++;
		this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
	} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
	if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}

// poll until Calibration is ready 
	cal_poll_counter = 0 ;
	do {
		cal_poll_counter++;
		// read data
		write_data = 0x8000  ; // read Instruction + data
		return_code = this->i->vme_A32D32_write(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, write_data ); //  
		if (return_code != 0) {	return -1 ;	}

		poll_counter = 0 ;
		do {
			poll_counter++;
			this->i->vme_A32D32_read(this->baseaddress + SIS3316_NIM_CLK_MULTIPLIER_SPI_REG, &read_data ); //  
		} while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5325_SPI_POLL_COUNTER_MAX)) ;
		if (poll_counter == SI5325_SPI_POLL_COUNTER_MAX) {	return -2 ;	}

	} while (((read_data & 0x40) == 0x40) && (cal_poll_counter < SI5325_SPI_CALIBRATION_READY_POLL_COUNTER_MAX)) ;
	if (cal_poll_counter == SI5325_SPI_CALIBRATION_READY_POLL_COUNTER_MAX) {	return -3 ;	}

	return 0 ;
}

int SISInterface::get_status_external_clock_multiplier( unsigned int *status )
{
	int rc;
	unsigned int data ;
	rc = si5325_clk_muliplier_read( 128, &data); //  
	*status = data & 0x1 ;
	rc = si5325_clk_muliplier_read( 129, &data); //  
	*status = *status + (data & 0x2) ;
	return rc ;
}

int SISInterface::bypass_external_clock_multiplier(void )
{
	int rc;
	rc = si5325_clk_muliplier_write( 0, 0x2); // Bypass 
	rc = si5325_clk_muliplier_write(11, 0x02); //  PowerDown clk2 
	return rc ;
}

int SISInterface::set_external_clock_multiplier( unsigned int bw_sel, unsigned int n1_hs, unsigned int n1_clk1, unsigned int n1_clk2, unsigned int n2, unsigned int n3, unsigned int clkin1_mhz )
{
	volatile unsigned int n1_val ;
	volatile unsigned int n1_hs_val ;
	volatile unsigned int n2_val ;
	volatile unsigned int n3_val ;

	// input frequency
	if ((clkin1_mhz < 10) || (clkin1_mhz > 250)) {
		return -2 ;
	}
	// bw_sel : see DSPLLsinm for setting
	if (bw_sel > 15) {
		return -3 ;
	}
	// n1_hs
	if ((n1_hs < 4) || (n1_hs > 11)) {
		return -4 ;
	}


	// n1_clk1
	if (n1_clk1 == 0)  {
		return -5 ;
	}
	else {
		if ((((n1_clk1) & 0x1) == 1) && (n1_clk1 != 1) ) { // odd but not 1
			return -5 ;
		}
		if ((n1_clk1 & 0xfff00000) != 0)  { // > 2**20
			return -5 ;
		}
	}

	// n1_clk2
	if (n1_clk2 == 0)  {
		return -6 ;
	}
	else {
		if ((((n1_clk2) & 0x1) == 1) && (n1_clk2 != 1) ) { // odd but not 1
			return -6 ;
		}
		if ((n1_clk2 & 0xfff00000) != 0)  { // > 2**20
			return -6 ;
		}
	}


	// n2
	if ((n2 < 32) || (n2 > 512)) {
		return -7 ;
	}
	else {
		if ((n2 & 0x1) == 1)  { // odd
			return -7 ;
		}
	}

	// n3
	if (n3 == 0)  {
		return -8 ;
	}
	else {
		if ((n3 & 0xfff80000) != 0)  { // > 2**19
			return -8 ;
		}
	}


	si5325_clk_muliplier_write( 0, 0x0); // No Bypass 
	si5325_clk_muliplier_write(11, 0x02); //  PowerDown clk2 

	// N3 = 1
	n3_val = n3 - 1 ;
	si5325_clk_muliplier_write(43, ((n3_val >> 16) & 0x7) );       //  N3 bits 18:16
	si5325_clk_muliplier_write(44, ((n3_val >> 8) & 0xff));       //  N3 bits 15:8
	si5325_clk_muliplier_write(45, (n3_val  & 0xff) );       //  N3 bits 7:0

	n2_val = n2  ;
	si5325_clk_muliplier_write(40, 0x00);						//    N2_LS bits 19:16
	si5325_clk_muliplier_write(41, ((n2_val >> 8) & 0xff));     //  N2_LS bits 15:8
	si5325_clk_muliplier_write(42, (n2_val  & 0xff));			//  N2_LS bits 7:0

	n1_hs_val = n1_hs - 4 ;
	si5325_clk_muliplier_write(25, (n1_hs_val << 5)); //   

	n1_val = n1_clk1 - 1 ;
	si5325_clk_muliplier_write(31, ((n1_val >> 16) & 0xf));       //  NC1_LS (low speed divider) bits 19:16
	si5325_clk_muliplier_write(32, ((n1_val >> 8) & 0xff));       //  NC1_LS (low speed divider) bits 15:8
	si5325_clk_muliplier_write(33, (n1_val  & 0xff) );       //  NC1_LS (low speed divider) bits 7:0

	n1_val = n1_clk2 - 1 ;
	si5325_clk_muliplier_write(34, ((n1_val >> 16) & 0xf));       //  NC2_LS (low speed divider) bits 19:16
	si5325_clk_muliplier_write(35, ((n1_val >> 8) & 0xff));       //  NC2_LS (low speed divider) bits 15:8
	si5325_clk_muliplier_write(36, (n1_val  & 0xff));       //  NC2_LS (low speed divider) bits 7:0


	si5325_clk_muliplier_write( 2, (bw_sel << 5) ); //BWSEL_REG 

	si5325_clk_muliplier_internal_calibration_cmd();

	return 0;
}
 
// adc_fpga_group: 0,1,2,3
// adc_chip: 0 or 1
//				-1 : not all adc chips have the same chip ID
//				>0 : VME Error Code

int SISInterface::adc_spi_setup( void )
{
	int return_code;
	unsigned int adc_chip_id;
	unsigned int addr, data;
	unsigned i_adc_fpga_group; 
	unsigned i_adc_chip; 

// reset 
	for (i_adc_fpga_group = 0; i_adc_fpga_group < 4; i_adc_fpga_group++) {
		for (i_adc_chip = 0; i_adc_chip < 2; i_adc_chip++) {
			this->adc_spi_write( i_adc_fpga_group, i_adc_chip, 0x0, 0x24); // soft reset
		}
	}

	this->adc_spi_read( 0, 0, 1, &adc_chip_id); // read chip Id from adc chips ch1/2
	this->adc_spi_read( 0, 0, 1, &adc_chip_id); // read chip Id from adc chips ch1/2
	this->adc_spi_read( 0, 0, 1, &adc_chip_id); // read chip Id from adc chips ch1/2

	for (i_adc_fpga_group = 0; i_adc_fpga_group < 4; i_adc_fpga_group++) {
		for (i_adc_chip = 0; i_adc_chip < 2; i_adc_chip++) {
			this->adc_spi_read( i_adc_fpga_group, i_adc_chip, 1, &data);
			printf("i_adc_fpga_group = %d   i_adc_chip = %d    data = 0x%08x     adc_chip_id = 0x%08x     \n", i_adc_fpga_group, i_adc_chip, data, adc_chip_id);
			if (data != adc_chip_id) {
				printf("i_adc_fpga_group = %d   i_adc_chip = %d    data = 0x%08x     adc_chip_id = 0x%08x     \n", i_adc_fpga_group, i_adc_chip, data, adc_chip_id);
				return -1 ;
			}
		}
	}

	this->adc_125MHz_flag = 0;
	if ((adc_chip_id&0xff) == 0x32) {
		this->adc_125MHz_flag = 1;
	}

// reg 0x14 : Output mode
	if (this->adc_125MHz_flag == 0) { // 250 MHz chip AD9643
		data = 0x04 ; 	//  Output inverted (bit2 = 1)
	}
	else { // 125 MHz chip AD9268
		data = 0x40 ; 	// Output type LVDS (bit6 = 1), Output inverted (bit2 = 0) !
	}
	for (i_adc_fpga_group = 0; i_adc_fpga_group < 4; i_adc_fpga_group++) {
		for (i_adc_chip = 0; i_adc_chip < 2; i_adc_chip++) {
			this->adc_spi_write( i_adc_fpga_group, i_adc_chip, 0x14, data); 
		}
	}


// reg 0x18 : Reference Voltage / Input Span
	if (this->adc_125MHz_flag == 0) { // 250 MHz chip AD9643
		data = 0x0 ; 	//  1.75V
	}
	else { // 125 MHz chip AD9268
		//data = 0x8 ; 	//  1.75V
		data = 0xC0 ; 	//  2.0V
	}
	for (i_adc_fpga_group = 0; i_adc_fpga_group < 4; i_adc_fpga_group++) {
		for (i_adc_chip = 0; i_adc_chip < 2; i_adc_chip++) {
			this->adc_spi_write( i_adc_fpga_group, i_adc_chip, 0x18, data); 
		}
	}




// reg 0xff : register update
	data = 0x01 ; 	// update
	for (i_adc_fpga_group = 0; i_adc_fpga_group < 4; i_adc_fpga_group++) {
		for (i_adc_chip = 0; i_adc_chip < 2; i_adc_chip++) {
			this->adc_spi_write( i_adc_fpga_group, i_adc_chip, 0xff, data); 
		}
	}


	// enable ADC output
	for (i_adc_fpga_group = 0; i_adc_fpga_group < 4; i_adc_fpga_group++) {
		addr = SIS3316_ADC_SPI_CTRL_REG +  ((i_adc_fpga_group & 0x3) * SIS3316_FPGA_ADC_REG_OFFSET) ;
		return_code = i->vme_A32D32_write(this->baseaddress + addr, 0x1000000); //  set bit 24
		if (return_code != 0) {return return_code ; }
	}
	return 0 ;
}

// adc_fpga_group: 0,1,2,3
// adc_chip: 0 or 1
// return codes
//				-1 : invalid parameter
//				-2 : timeout
//				>0 : VME Error Code

int SISInterface::adc_spi_read( unsigned int adc_fpga_group, unsigned int adc_chip, unsigned int spi_addr, unsigned int* spi_data )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int uint_adc_mux_select;
	unsigned int pollcounter;

	pollcounter = 1000;

	if (adc_fpga_group > 4) {return -1;}
	if (adc_chip > 2) {return -1;}
	if (spi_addr > 0xffff) {return -1;}

	if (adc_chip == 0) {
		uint_adc_mux_select = 0 ;	// adc chip ch1/ch2	
	}
	else {
		uint_adc_mux_select = 0x400000 ; // adc chip ch3/ch4 		
	}

	// read register to get the information of bit 24 (adc output enabled)
	addr = SIS3316_ADC_SPI_CTRL_REG +  ((adc_fpga_group & 0x3) * SIS3316_FPGA_ADC_REG_OFFSET) ;
	return_code = i->vme_A32D32_read(this->baseaddress + addr, &data); //  
	if (return_code != 0) {
		return return_code ; 
	}
	data = data & 0x01000000 ; // save bit 24


	data =  data + 0xC0000000 + uint_adc_mux_select + ((spi_addr & 0xffff) << 8)  ;
	addr = SIS3316_ADC_SPI_CTRL_REG +  ((adc_fpga_group & 0x3) * SIS3316_FPGA_ADC_REG_OFFSET) ;
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data); //  

	addr = SIS3316_ADC_FPGA_SPI_BUSY_STATUS_REG ;
	do { // the logic is appr. 7us busy  (20us)
		return_code = i->vme_A32D32_read(this->baseaddress + addr, &data); //  
		pollcounter--;
	} while (((data & 0x80000000) == 0x80000000) && (pollcounter > 0) && (return_code == 0));
	if (return_code != 0) {return return_code ; }
	if (pollcounter == 0) {return -2 ; }

	usleep(1000) ; //

	addr = SIS3316_ADC_SPI_READBACK_REG ;
	return_code = i->vme_A32D32_read(this->baseaddress + addr, &data); //  
	if (return_code != 0) {return return_code ; }

	*spi_data = data & 0xff ;
	return 0 ;
}


int SISInterface::adc_spi_write( unsigned int adc_fpga_group, unsigned int adc_chip, unsigned int spi_addr, unsigned int spi_data )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int uint_adc_mux_select;
	unsigned int pollcounter;
	pollcounter = 1000;

	if (adc_fpga_group > 4) {return -1;}
	if (adc_chip > 2) {return -1;}
	if (spi_addr > 0xffff) {return -1;}

	if (adc_chip == 0) {
		uint_adc_mux_select = 0 ;	// adc chip ch1/ch2	
	}
	else {
		uint_adc_mux_select = 0x400000 ; // adc chip ch3/ch4 		
	}

	// read register to get the information of bit 24 (adc output enabled)
	addr = SIS3316_ADC_SPI_CTRL_REG +  ((adc_fpga_group & 0x3) * SIS3316_FPGA_ADC_REG_OFFSET) ;
	return_code = i->vme_A32D32_read(this->baseaddress + addr, &data); //  
	if (return_code != 0) {return return_code ; }
	data = data & 0x01000000 ; // save bit 24

	data =  data + 0x80000000 + uint_adc_mux_select + ((spi_addr & 0xffff) << 8) + (spi_data & 0xff) ;
	addr = SIS3316_ADC_SPI_CTRL_REG +  ((adc_fpga_group & 0x3) * SIS3316_FPGA_ADC_REG_OFFSET) ;
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data); //  

	addr = SIS3316_ADC_FPGA_SPI_BUSY_STATUS_REG ;
	do { // the logic is appr. 7us busy
		return_code = i->vme_A32D32_read(this->baseaddress + addr, &data); //  
		pollcounter--;
	} while (((data & 0x80000000) == 0x80000000) && (pollcounter > 0) && (return_code == 0));
	if (return_code != 0) {return return_code ; }
	if (pollcounter == 0) {return -2 ; }
	return 0 ;
}

int SISInterface::read_MBLT64_Channel_PreviousBankDataBuffer( unsigned int bank2_read_flag, unsigned int channel_no, unsigned int*  dma_got_no_of_words, unsigned int* uint_adc_buffer )
{
	int return_code;
	unsigned int  max_read_nof_words;
	max_read_nof_words = 0xffffff ;
	return_code = this->read_DMA_Channel_PreviousBankDataBuffer(bank2_read_flag, channel_no, max_read_nof_words, dma_got_no_of_words, uint_adc_buffer ) ;

	return 0 ;
}

int SISInterface::read_DMA_Channel_PreviousBankDataBuffer( unsigned int bank2_read_flag, unsigned int channel_no, unsigned int  max_read_nof_words, unsigned int*  dma_got_no_of_words, unsigned int* uint_adc_buffer )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int previous_bank_addr_value ;
	unsigned int req_nof_32bit_words ;
	unsigned int got_nof_32bit_words ;
	unsigned int memory_bank_offset_addr ;
	unsigned int max_poll_counter ;
	
	// read previous Bank sample address
	// poll until it is valid.
	addr = SIS3316_ADC_PREVIOUS_BANK_SAMPLE_ADDRESS_REG + ((channel_no & 0x3) * 4) + (((channel_no >> 2) & 0x3) * SIS3316_FPGA_ADC_REG_OFFSET) ; // Previous Bank Sample address register for the channel ch_no
	max_poll_counter = 1000;
	*dma_got_no_of_words = 0;
	do {
		return_code = i->vme_A32D32_read(this->baseaddress + addr, &previous_bank_addr_value); //  
		return_code = i->vme_A32D32_read(this->baseaddress + addr, &previous_bank_addr_value); //  only if version VME _V0002
		if(return_code != 0) { return return_code;}
		max_poll_counter--;
		if (max_poll_counter == 0) { 
			printf("max_poll_counter e' arrivato a zero\n");
			return 0x900;
		}
	} while (((previous_bank_addr_value & 0x1000000) >> 24 )  != bank2_read_flag) ; // previous Bank sample address is valid if bit 24 is equal bank2_read_flag 

	printf("previous_bank_addr_value 24bit per il canale %d e' uguale a %d\n",channel_no+1,(previous_bank_addr_value & 0x1000000) >> 24);

	if((previous_bank_addr_value & 0xffffff) == 0) { // no data sampled !
		*dma_got_no_of_words = 0;
		printf("(previous_bank_addr_value & 0xffffff) == 0 therefore it does not read any data\n");
		return 0;	
	} 

	printf(" numero di roba da leggere = %d\n",(previous_bank_addr_value & 0xffffff)); // mod Anto

	// start Readout FSM
	if (bank2_read_flag == 1) {		
		memory_bank_offset_addr = 0x01000000; // Bank2 offset
	}
	else {
		memory_bank_offset_addr = 0x00000000; // Bank1 offset
	}

	if ((channel_no & 0x1) != 0x1) { // 0,1 	
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 1 , 3, ..... 15
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x02000000; // channel 2 , 4, ..... 16
	}

	if ((channel_no & 0x2) != 0x2) { // 0,2 	
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 0,1 , 4,5, .....
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x10000000; // channel 2,3 , 6,7 ..... 
	}

	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x80000000 + memory_bank_offset_addr ;
	printf("data = 0x%08x e memory_bank_offser_addr = 0x%08x dopo aver fatto il write de data_transfer_Ch1_4_ctrl_reg\n",data,memory_bank_offset_addr); //mod Anto
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	//if(return_code != 0) { return return_code;} // old version
	if(return_code != 0) { // modificato Anto
		printf("il comando i->vme_A32D32_write(this->baseaddress + addr, data) non va a buon fine e ritorna qualcosa diverso da zero\n");
		return return_code;
	} // fine mod Anto

	// readout 
	addr = SIS3316_FPGA_ADC_MEM_BASE + (((channel_no >> 2) & 0x3 )* SIS3316_FPGA_ADC_MEM_OFFSET)  ;
	printf("addr per leggere la FIFO = 0x%08x \n",addr); //mod Anto
	req_nof_32bit_words = previous_bank_addr_value & 0xffffff ;
	if (req_nof_32bit_words > max_read_nof_words){
		req_nof_32bit_words = max_read_nof_words ;
	}
	printf("got_nof_32bit_words = 0x%08x\n",got_nof_32bit_words); //mod Anto
	return_code = i->vme_A32MBLT64FIFO_read(this->baseaddress + addr, uint_adc_buffer, ((req_nof_32bit_words + 1) & 0xfffffE) , &got_nof_32bit_words); // N * 8-byte length  !!! 
	printf("got_nof_32bit_words = 0x%08x\n",got_nof_32bit_words); //mod Anto
	//if(return_code != 0) { return return_code;} // old version
	if(return_code != 0) { // mod Anto
		printf("la funzione i->vme_A32MBLT64FIFO_read(this->baseaddress + addr, uint_adc_buffer, ((req_nof_32bit_words + 1) & 0xfffffE) , &got_nof_32bit_words) non � andata a buon fine!!!!\n");
		return return_code;
	} // fine mod Anto

	*dma_got_no_of_words = req_nof_32bit_words ;

// new
	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x00000000  ; // Reset
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	//if(return_code != 0) { return return_code;} // old version
	if(return_code != 0) { printf("la funzione i->vme_A32D32_write(this->baseaddress + addr, data) nella seconda volta che la chiama non va a buon fine\n"); return return_code;} //mod Anto

	return 0 ;
}

int SISInterface::read_Channel_EnergyHistogramBuffer( unsigned int channel_no, unsigned int lenght, unsigned int* uint_adc_buffer )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int req_nof_32bit_words ;
	unsigned int got_nof_32bit_words ;
	unsigned int memory_bank_offset_addr ;
//	unsigned int max_poll_counter ;
	
	memory_bank_offset_addr = 0x00FF0000; // Histogram offset

	if ((channel_no & 0x1) != 0x1) { // 0,1 	
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 1 , 3, ..... 15
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x02000000; // channel 2 , 4, ..... 16
	}

	if ((channel_no & 0x2) != 0x2) { // 0,2 	
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 0,1 , 4,5, .....
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x10000000; // channel 2,3 , 6,7 ..... 
	}

	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x80000000 + memory_bank_offset_addr ;
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}

	// readout 
	addr = SIS3316_FPGA_ADC_MEM_BASE + (((channel_no >> 2) & 0x3 )* SIS3316_FPGA_ADC_MEM_OFFSET)  ;
	req_nof_32bit_words = lenght   ;
	return_code = i->vme_A32MBLT64FIFO_read(this->baseaddress + addr, uint_adc_buffer, ((req_nof_32bit_words + 1) & 0xfffffE) , &got_nof_32bit_words); // N * 8-byte length  !!! 
	if(return_code != 0) { return return_code;}


// new
	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x00000000  ; // Reset
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}


	return 0 ;
}
	
int SISInterface::read_Channel_TofHistogramBuffer( unsigned int channel_no, int histogram_index, unsigned int lenght, unsigned int* uint_buffer )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int req_nof_32bit_words ;
	unsigned int got_nof_32bit_words ;
	unsigned int memory_bank_offset_addr ;
//	unsigned int max_poll_counter ;

	if(histogram_index == 0) {
		memory_bank_offset_addr = 0x00C00000; // Histogram offset Beam On
	}
	else {
		memory_bank_offset_addr = 0x00D00000; // Histogram offset Beam OFF
	}

	if ((channel_no & 0x1) != 0x1) { // 0,1
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 1 , 3, ..... 15
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x02000000; // channel 2 , 4, ..... 16
	}

	if ((channel_no & 0x2) != 0x2) { // 0,2
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 0,1 , 4,5, .....
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x10000000; // channel 2,3 , 6,7 .....
	}

	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x80000000 + memory_bank_offset_addr ;
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}

	// readout
	addr = SIS3316_FPGA_ADC_MEM_BASE + (((channel_no >> 2) & 0x3 )* SIS3316_FPGA_ADC_MEM_OFFSET)  ;
	req_nof_32bit_words = lenght   ;
	return_code = i->vme_A32MBLT64FIFO_read(this->baseaddress + addr, uint_buffer, ((req_nof_32bit_words + 1) & 0xfffffE) , &got_nof_32bit_words); // N * 8-byte length  !!!
	if(return_code != 0) { return return_code;}


// new
	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x00000000  ; // Reset
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}


	return 0 ;
}

int SISInterface::read_Channel_ShapeHistogramBuffer( unsigned int channel_no, int histogram_index, unsigned int lenght, unsigned int* uint_buffer )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int req_nof_32bit_words ;
	unsigned int got_nof_32bit_words ;
	unsigned int memory_bank_offset_addr ;
//	unsigned int max_poll_counter ;

	if(histogram_index == 0) {
		memory_bank_offset_addr = 0x00E00000; // Histogram offset Beam On
	}
	else {
		memory_bank_offset_addr = 0x00E40000; // Histogram offset Beam OFF
	}

	if ((channel_no & 0x1) != 0x1) { // 0,1
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 1 , 3, ..... 15
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x02000000; // channel 2 , 4, ..... 16
	}

	if ((channel_no & 0x2) != 0x2) { // 0,2
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 0,1 , 4,5, .....
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x10000000; // channel 2,3 , 6,7 .....
	}

	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x80000000 + memory_bank_offset_addr ;
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}

	// readout
	addr = SIS3316_FPGA_ADC_MEM_BASE + (((channel_no >> 2) & 0x3 )* SIS3316_FPGA_ADC_MEM_OFFSET)  ;
	req_nof_32bit_words = lenght   ;
	//printf("req_nof_32bit_words    = %d     \n", req_nof_32bit_words);

	return_code = i->vme_A32MBLT64FIFO_read(this->baseaddress + addr, uint_buffer, ((req_nof_32bit_words + 1) & 0xfffffE) , &got_nof_32bit_words); // N * 8-byte length  !!!
	if(return_code != 0) { return return_code;}


// new
	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x00000000  ; // Reset
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}

	return 0 ;
}

int SISInterface::read_Channel_ChargeHistogramBuffer( unsigned int channel_no, int histogram_index, unsigned int lenght, unsigned int* uint_buffer )
{
	int return_code;
	unsigned int data ;
	unsigned int addr ;
	unsigned int req_nof_32bit_words ;
	unsigned int got_nof_32bit_words ;
	unsigned int memory_bank_offset_addr ;
//	unsigned int max_poll_counter ;

	if(histogram_index == 0) {
		memory_bank_offset_addr = 0x00F00000; // Histogram offset Beam On
	}
	else {
		memory_bank_offset_addr = 0x00F10000; // Histogram offset Beam OFF
	}

	if ((channel_no & 0x1) != 0x1) { // 0,1
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 1 , 3, ..... 15
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x02000000; // channel 2 , 4, ..... 16
	}

	if ((channel_no & 0x2) != 0x2) { // 0,2
		memory_bank_offset_addr = memory_bank_offset_addr + 0x00000000; // channel 0,1 , 4,5, .....
	}
	else {
		memory_bank_offset_addr = memory_bank_offset_addr + 0x10000000; // channel 2,3 , 6,7 .....
	}

	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x80000000 + memory_bank_offset_addr ;
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}

	// readout
	addr = SIS3316_FPGA_ADC_MEM_BASE + (((channel_no >> 2) & 0x3 )* SIS3316_FPGA_ADC_MEM_OFFSET)  ;
	req_nof_32bit_words = lenght   ;
	return_code = i->vme_A32MBLT64FIFO_read(this->baseaddress + addr, uint_buffer, ((req_nof_32bit_words + 1) & 0xfffffE) , &got_nof_32bit_words); // N * 8-byte length  !!!
	if(return_code != 0) { return return_code;}


// new
	addr = SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG + (((channel_no >> 2) & 0x3) * 4) ;
	data = 0x00000000  ; // Reset
	return_code = i->vme_A32D32_write(this->baseaddress + addr, data);
	if(return_code != 0) { return return_code;}


	return 0 ;
}

SISInterface::~SISInterface(void)
{
	
}