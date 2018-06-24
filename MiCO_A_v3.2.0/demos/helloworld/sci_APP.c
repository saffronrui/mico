#include "mico.h"
#include "MICOAppDefine.h"
#include "sem_os_APP.h"

extern uint8_t *inDataBuffer;
extern int recvlen;
bool data_ready = 0;
int semphr_fd;


void tcp_client_tx_thread( mico_thread_arg_t arg )
{
    OSStatus err = kNoErr;
    int fd = (int) arg;
    int len = 0;
    fd_set readfds;
    char *buf = NULL;
    struct timeval t;

    fd_set readfds_sem;
    char *buf1 = "123abcd";

    while ( 1 )
    {

        FD_ZERO( &readfds_sem );
        FD_SET( semphr_fd, &readfds_sem );
        select( 24, &readfds_sem, NULL, NULL, NULL );


        if ( FD_ISSET( semphr_fd, &readfds_sem ) )
        {
            mico_rtos_get_semaphore( &os_sem, MICO_WAIT_FOREVER ); //wait until get semaphore
            MicoGpioOutputTrigger( MICO_SYS_LED);
        	len = send( fd, inDataBuffer, recvlen, 0 );
            os_sem_log( "get semaphore" );
        }

    }

	exit:
    if ( err != kNoErr ) tcp_server_log( "TCP client thread exit with err: %d", err );
    if ( buf != NULL ) free( buf );
    SocketClose( &fd );

    if ( os_sem != NULL )
	  {
		  mico_rtos_deinit_semaphore( &os_sem );
	  }

	  if( semphr_fd != -1 )
	  {
		  mico_rtos_deinit_event_fd( semphr_fd );
	  }

    mico_rtos_delete_thread( NULL );
}


size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen)
{
	  uart_recv_log_trace();

	  int datalen;

	  while(1) {
	    if( MicoUartRecv( UART_FOR_APP, inBuf, inBufLen, UART_RECV_TIMEOUT) == kNoErr){
	      return inBufLen;
	    }
	   else{
	     datalen = MicoUartGetLengthInBuffer( UART_FOR_APP );
	     if(datalen){
	       MicoUartRecv(UART_FOR_APP, inBuf, datalen, UART_RECV_TIMEOUT);
//	   	   MicoGpioOutputTrigger( MICO_RF_LED);
	       return datalen;
	     }
	   }
	  }
}

void uartRecv_thread(uint32_t inContext)
{
	  uart_recv_log_trace();

	  inDataBuffer = malloc(UART_ONE_PACKAGE_LENGTH);
	  require(inDataBuffer, exit);

	  while(1) {
	    recvlen = _uart_get_one_packet(inDataBuffer, UART_ONE_PACKAGE_LENGTH);
	    if (recvlen <= 0)
	      continue;
	    os_sem_log( "release semaphore" );
//	    data_ready = true;
        mico_rtos_set_semaphore( &os_sem );
//	   	MicoUartSend(UART_FOR_APP, inDataBuffer, recvlen);
	  }

	exit:
	  if(inDataBuffer) free(inDataBuffer);
	  mico_rtos_delete_thread(NULL);
}

volatile ring_buffer_t rx_buffer;
volatile uint8_t rx_data[UART_BUFFER_LENGTH];

OSStatus app_sci_init(){

	OSStatus err = kNoErr;
	mico_uart_config_t uart_config;
    app_context_t* app_context;

    err = mico_rtos_init_semaphore( &os_sem, 1 ); //0/1 binary semaphore || 0/N semaphore
//    require_noerr( err, exit );
    semphr_fd = mico_create_event_fd( os_sem );

	uart_config.baud_rate = 115200;
	uart_config.data_width = DATA_WIDTH_8BIT;
	uart_config.parity = NO_PARITY;
	uart_config.stop_bits = STOP_BITS_1;
	uart_config.flow_control = FLOW_CONTROL_DISABLED;
	uart_config.flags = UART_WAKEUP_DISABLE;

	ring_buffer_init( (ring_buffer_t *) &rx_buffer, (uint8_t *) rx_data, UART_BUFFER_LENGTH );
	MicoUartInitialize( UART_FOR_APP, &uart_config, (ring_buffer_t *) &rx_buffer );

	err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uartRecv_thread,
	                                 STACK_SIZE_UART_RECV_THREAD, (mico_thread_arg_t)app_context );

	return err;
}



