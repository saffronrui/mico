/*
 * vfd.c
 *
 *  Created on: 2018Äê6ÔÂ14ÈÕ
 *      Author: Administrator
 */
#include "mico.h"
#include "platform.h"
#include "common.h"
#include "vfd.h"

char vfd_addr[vfd_char_num] = {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
							   0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf};

const mico_spi_device_t mico_vfd =
{
  .port        = MICO_SPI_1,
  .chip_select = Arduino_CS,
  .speed       = 5000000,
  .mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_USE_DMA | SPI_MSB_FIRST ),
  .bits        = 8
};


platform_spi_message_segment_t SPI_msg;
void mico_vfd_thread( mico_thread_arg_t arg ){
	char temp_cont = 0x30;
	char temp_addr = 0x80;
	char i = 0;

	vfd_clear();		// clear vfd_display after system reset

	while(1){

		vfd_dis_char(vfd_addr[i++], temp_cont++);
		if(temp_cont > 0x3a)
			temp_cont = 0x30;
		if(i >= 32)
			i = 0;

		mico_thread_msleep(1000);
	}
}

/*
 * clear the vfd_display function
 * para: none
 * return: none
 */
void 	vfd_clear(void){

	char	temp_cmd1[2] = {vfd_cmd, vfd_cmd_clear};
	char	temp_cmd2[2] = {vfd_cmd, 0x02};
	char	temp_cmd3[2] = {vfd_cmd, 0x08};

	SPI_msg.tx_buffer = temp_cmd1;
	SPI_msg.length = 2;
	MicoSpiTransfer( &mico_vfd, &SPI_msg, 1 );			// clear the dispay

	SPI_msg.tx_buffer = temp_cmd2;
	SPI_msg.length = 2;
	MicoSpiTransfer( &mico_vfd, &SPI_msg, 1 );			// reset the counter

	SPI_msg.tx_buffer = temp_cmd3;
	SPI_msg.length = 2;
	MicoSpiTransfer( &mico_vfd, &SPI_msg, 1 );			// turn off the display

}


void	vfd_dis_char(char	addr, char	cont){

	char temp_cmd1[2] = {vfd_cmd,  addr};
	char temp_cmd2[2] = {vfd_data, cont};
	char temp_cmd3[2] = {vfd_cmd,  0x0f};

	SPI_msg.tx_buffer = temp_cmd1;
	SPI_msg.length = 2;
	MicoSpiTransfer( &mico_vfd, &SPI_msg, 1 );
	SPI_msg.tx_buffer = temp_cmd2;
	SPI_msg.length = 2;
	MicoSpiTransfer( &mico_vfd, &SPI_msg, 1 );
	SPI_msg.tx_buffer = temp_cmd3;
	SPI_msg.length = 2;
	MicoSpiTransfer( &mico_vfd, &SPI_msg, 1 );

}

int mico_vfd_init(){

	OSStatus err = kNoErr;

	MicoSpiInitialize( &mico_vfd );
//	require_noerr_action( err, exit, os_helloworld_log("ERROR: Unable to init the VFD.") );

	err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "VFD_update", mico_vfd_thread,
		                                 0x800,
		                                0 );
	exit:
	return -1;
}

