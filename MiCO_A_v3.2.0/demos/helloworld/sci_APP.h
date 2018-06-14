/*
 * sci_APP.h
 *
 *  Created on: 2018Äê6ÔÂ12ÈÕ
 *      Author: Administrator
 */

#include "common.h"

#ifndef DEMOS_HELLOWORLD_SCI_APP_H_
#define DEMOS_HELLOWORLD_SCI_APP_H_


OSStatus app_sci_init();
void uartRecv_thread(uint32_t inContext);
size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen);
void tcp_client_tx_thread( mico_thread_arg_t arg );


#endif /* DEMOS_HELLOWORLD_SCI_APP_H_ */
