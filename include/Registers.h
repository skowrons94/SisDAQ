#define SIS3316_CONTROL_STATUS                       					0x0			/* read/write; D32 */
#define SIS3316_MODID                                					0x4			/* read only; D32 */
#define SIS3316_IRQ_CONFIG                           					0x8			/* read/write; D32 */
#define SIS3316_IRQ_CONTROL                          					0xC			/* read/write; D32 */

#define SIS3316_INTERFACE_ACCESS_ARBITRATION_CONTROL   					0x10		/* read/write; D32 */
#define SIS3316_CBLT_BROADCAST            								0x14		/* read/write; D32 */
#define SIS3316_HARDWARE_VERSION                       					0x1C			/* read only; D32 */

#define SIS3316_UDP_PROT_CONFIGURATION                 					0x8			/* read/write; D32 */

#define SIS3316_INTERNAL_TEMPERATURE_REG		      	    			0x20      /* read/write; D32 */
#define SIS3316_ONE_WIRE_CONTROL_REG       		   		    			0x24      /* read/write; D32 */
#define SIS3316_SERIAL_NUMBER_REG       		   		    			0x28      /* read only; D32 */

#define SIS3316_ADC_CLK_OSC_I2C_REG				      	    			0x40      /* read/write; D32 */
#define SIS3316_MGT1_OSC_I2C_REG				      	    			0x44      /* read/write; D32 */

#define SIS3316_SAMPLE_CLOCK_DISTRIBUTION_CONTROL      	    			0x50      /* read/write; D32 */
#define SIS3316_NIM_CLK_MULTIPLIER_SPI_REG      	    				0x54      /* read/write; D32 */
#define SIS3316_FP_LVDS_BUS_CONTROL             		    			0x58      /* read/write; D32 */
#define SIS3316_NIM_INPUT_CONTROL_REG  									0x5C      /* read/write; D32 */

#define SIS3316_ACQUISITION_CONTROL_STATUS         		    			0x60      /* read/write; D32 */

#define SIS3316_LEMO_OUT_CO_SELECT_REG  								0x70      /* read/write; D32 */
#define SIS3316_LEMO_OUT_TO_SELECT_REG  								0x74      /* read/write; D32 */
#define SIS3316_LEMO_OUT_UO_SELECT_REG  								0x78      /* read/write; D32 */

#define SIS3316_DATA_TRANSFER_CH1_4_CTRL_REG							0x80  /* r/w; D32 */
#define SIS3316_DATA_TRANSFER_CH5_8_CTRL_REG  							0x84  /* r/w; D32 */
#define SIS3316_DATA_TRANSFER_CH9_12_CTRL_REG 							0x88  /* r/w; D32 */
#define SIS3316_DATA_TRANSFER_CH12_16_CTRL_REG  						0x8C  /* r/w; D32 */

#define SIS3316_DATA_TRANSFER_ADC1_4_STATUS_REG							0x90  /* read; D32 */
#define SIS3316_DATA_TRANSFER_ADC5_8_STATUS_REG  						0x94  /* read; D32 */
#define SIS3316_DATA_TRANSFER_ADC9_12_STATUS_REG 						0x98  /* read; D32 */
#define SIS3316_DATA_TRANSFER_ADC12_16_STATUS_REG  						0x9C  /* read; D32 */

#define SIS3316_VME_FPGA_LINK_ADC_PROT_STATUS      		    			0xA0      /* read/write; D32 */

#define SIS3316_ADC_FPGA_SPI_BUSY_STATUS_REG 							0xA4                  

#define SIS3316_KEY_RESET                           					0x400	  /* write only; D32 */
#define SIS3316_KEY_USER_FUNCTION                          				0x404	  /* write only; D32 */
#define SIS3316_KEY_HISTOGRAMS_CLEAR                       				0x404	  /* write only; D32 */

//#define SIS3316_KEY_ARM                         						0x410	  /* write only; D32 */
#define SIS3316_KEY_DISARM                         						0x414	  /* write only; D32 */
#define SIS3316_KEY_TRIGGER                         					0x418	  /* write only; D32 */
#define SIS3316_KEY_TIMESTAMP_CLEAR                 					0x41C	  /* write only; D32 */
#define SIS3316_KEY_DISARM_AND_ARM_BANK1           						0x420	  /* write only; D32 */
#define SIS3316_KEY_DISARM_AND_ARM_BANK2           						0x424	  /* write only; D32 */
#define SIS3316_KEY_ENABLE_SAMPLE_BANK_SWAP_CONTROL_WITH_NIM_INPUT		0x428
#define SIS3316_KEY_ADC_FPGA_RESET           							0x434	  /* write only; D32 */
#define SIS3316_KEY_ADC_CLOCK_DCM_RESET           						0x438	  /* write only; D32 */

/* ***************************************************************************** */

/* ADC FPGA Read/Write registers                                                 */

#define SIS3316_ADC_INPUT_TAP_DELAY_REG     						0x1000	    
#define SIS3316_ADC_ANALOG_CTRL_REG     							0x1004	   
#define SIS3316_ADC_DAC_OFFSET_CTRL_REG 							0x1008	    
#define SIS3316_ADC_SPI_CTRL_REG 									0x100C	  
#define SIS3316_ADC_EVENT_CONFIG_REG 								0x1010	    
#define SIS3316_ADC_CHANNEL_HEADER_REG 							    0x1014	  
#define SIS3316_ADC_ADDRESS_THRESHOLD_REG 						    0x1018	  
#define SIS3316_ADC_TRIGGER_GATE_WINDOW_LENGTH_REG 				    0x101C	    
#define SIS3316_ADC_RAW_DATA_BUFFER_CONFIG_REG 					    0x1020	   
#define SIS3316_ADC_PILEUP_CONFIG_REG 							    0x1024	  	  
#define SIS3316_ADC_PRE_TRIGGER_DELAY_REG 						    0x1028	    
#define SIS3316_ADC_AVERAGE_CONFIGURATION_REG						0x102C
#define SIS3316_ADC_DATAFORMAT_CONFIG_REG 						    0x1030	    
#define SIS3316_ADC_MAW_TEST_BUFFER_CONFIG_REG 					    0x1034	    
#define SIS3316_ADC_INTERNAL_TRIGGER_DELAY_CONFIG_REG 			    0x1038	   
#define SIS3316_ADC_INTERNAL_GATE_LENGTH_CONFIG_REG 				0x103C	   
#define SIS3316_ADC_FIR_TRIGGER_SETUP_REG					        0x1040 // add 0x0010 for each channel
#define SIS3316_ADC_SUM_FIR_TRIGGER_SETUP_REG						0x1080 
#define SIS3316_ADC_FIR_TRIGGER_THRESHOLD_REG						0x1044 // add 0x0010 for each channel
#define SIS3316_ADC_SUM_FIR_TRIGGER_THRESHOLD_REG					0x1084
#define SIS3316_ADC_PEAK_CHARGE_CONFIGURATION_REG                   0x1090
#define SIS3316_ADC_FIR_HIGH_ENERGY_THRESHOLD_REG					0x1048 // add 0x0010 for each channel
#define SIS3316_ADC_SUM_FIR_HIGH_ENERGY_THRESHOLD_REG				0x1088 
#define SIS3316_ADC_TRIGGER_STATISTIC_COUNTER_MODE_REG			    0x1090 
#define SIS3316_ADC_ACCUMULATOR_GATE_CONFIG_REG 					0x10A0 // add 0x0010 for each channel	        
#define SIS3316_ADC_FIR_ENERGY_SETUP_REG							0x10C0 // add 0x0010 for each channel
#define SIS3316_ADC_HISTOGRAM_CONF_REG								0x10D0 // add 0x0010 for each channel

/* ***************************************************************************** */

/* ADC FPGA Read registers                                                       */

#define SIS3316_ADC_FIRMWARE_REG     								0x1100 /* rd only */  
#define SIS3316_ADC_STATUS_REG     								    0x1104 /* rd only */  
#define SIS3316_ADC_DAC_OFFSET_READBACK_REG 						0x1108	   
#define SIS3316_ADC_SPI_READBACK_REG 								0x110C	    
#define SIS3316_ADC_ACTUAL_SAMPLE_ADDRESS_REG						0x1110 /* rd only */  // add 0x0010 for each channel
#define SIS3316_ADC_PREVIOUS_BANK_SAMPLE_ADDRESS_REG				0x1120 // add 0x0010 for each channel   

/* ***************************************************************************** */

#define SIS3316_FPGA_ADC_REG_BASE					  				0x1000	    
#define SIS3316_FPGA_ADC_REG_OFFSET					  				0x1000	  
#define SIS3316_FPGA_ADC_MEM_BASE					  				0x100000	  	  
#define SIS3316_FPGA_ADC_MEM_OFFSET					  				0x100000	  

/* ***************************************************************************** */

#define FIR_ENERGY_MAX_PEAKING			0x7fe
#define FIR_ENERGY_MIN_PEAKING			2

#define FIR_ENERGY_MAX_GAP				0x1fe
#define FIR_ENERGY_MIN_GAP				2

#define FIR_ENERGY_MAX_TAU_TABLE		0x3
#define FIR_ENERGY_MAX_TAU_FACTOR		0x3f
#define FIR_ENERGY_MAX_ADD_AVERAGE		3
