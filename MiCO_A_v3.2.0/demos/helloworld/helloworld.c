/**
 ******************************************************************************
 * @file    hello_world.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   First MiCO application to say hello world!
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include "mico.h"
#include "MICOAppDefine.h"

#define os_helloworld_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)
#define tcp_server_log(M, ...) custom_log("TCP", M, ##__VA_ARGS__)
#define os_sem_log(M, ...) custom_log("OS", M, ##__VA_ARGS__)
#define uart_recv_log_trace() custom_log_trace("UART RECV")
#define udp_unicast_log(M, ...) custom_log("UDP", M, ##__VA_ARGS__)
#define udp_broadcast_log(M, ...) custom_log("UDP", M, ##__VA_ARGS__)

#define	ELE_LEN			100
#define	FCC_LEN			100

static mico_semaphore_t os_sem = NULL;
int semphr_fd = -1;

uint8_t *inDataBuffer;
int recvlen;
bool isReady;

static char *ap_ssid = "mxchip_zfw";
static char *ap_key = "12345678";
#define SERVER_PORT 20000 /*set up a tcp server,port at 20000*/

#define LOCAL_UDP_PORT 20000
#define REMOTE_UDP_PORT 20001

char* data = "UDP broadcast data";


void micoNotify_WifiStatusHandler( WiFiEvent event, void* const inContext )
{
    IPStatusTypedef para;
    switch ( event )
    {
        case NOTIFY_STATION_UP:
            micoWlanGetIPStatus( &para, Station );
            tcp_server_log("Server established at ip: %s port: %d",para.ip, SERVER_PORT);
            break;
        case NOTIFY_STATION_DOWN:
            case NOTIFY_AP_UP:
            case NOTIFY_AP_DOWN:
            break;
    }
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
		   isReady = 1;
	       return datalen;
	     }
	   }
	  }
}

void tcp_client_tx_thread( mico_thread_arg_t arg )
{
    OSStatus err = kNoErr;
    int fd = (int) arg;
    int len = 0;
    fd_set readfds;
    char *buf = NULL;
    struct timeval t;

    fd_set readfds_sem;


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


void tcp_client_thread( mico_thread_arg_t arg )
{
    OSStatus err = kNoErr;
    int fd = (int) arg;
    int len = 0;
    fd_set readfds;
    char *buf = NULL;
    struct timeval t;

    fd_set readfds_sem;

    buf = (char*) malloc( 1024 );
   // require_action( buf, exit, err = kNoMemoryErr );

    t.tv_sec = 5;
    t.tv_usec = 0;

    char *tempstr = "liuruirui12345";

    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( fd, &readfds );
/*
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
*/

       require_action( select( fd+1, &readfds, NULL, NULL, &t) >= 0, exit, err = kConnectionErr );

        if ( FD_ISSET( fd, &readfds ) ) //one client has data
        {
        	len = recv( fd, buf, 1024, 0 );
            require_action( len >= 0, exit, err = kConnectionErr );

            if ( len == 0 )
            {
                tcp_server_log( "TCP Client is disconnected, fd: %d", fd );
                goto exit;
            }

            tcp_server_log("fd: %d, recv data %d from client", fd, len);
            MicoUartSend(UART_FOR_APP, buf, len);
        }

    }

	exit:
    if ( err != kNoErr ) tcp_server_log( "TCP client thread exit with err: %d", err );
    if ( buf != NULL ) free( buf );
    SocketClose( &fd );
/*
    if ( os_sem != NULL )
	  {
		  mico_rtos_deinit_semaphore( &os_sem );
	  }

	  if( semphr_fd != -1 )
	  {
		  mico_rtos_deinit_event_fd( semphr_fd );
	  }
*/
    mico_rtos_delete_thread( NULL );
}

/* TCP server listener thread */
void tcp_server_thread( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    OSStatus err = kNoErr;
    OSStatus server_err = kNoErr;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sockaddr_t_size = sizeof(client_addr);
    char client_ip_str[16];
    int tcp_listen_fd = -1, client_fd = -1;
    fd_set readfds;

    tcp_listen_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    require_action( IsValidSocket( tcp_listen_fd ), exit, err = kNoResourcesErr );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
    server_addr.sin_port = htons( SERVER_PORT );/* Server listen on port: 20000 */

    err = bind( tcp_listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr) );
    require_noerr( err, exit );

    err = listen( tcp_listen_fd, 0 );
    require_noerr( err, exit );

    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( tcp_listen_fd, &readfds );

        require( select( tcp_listen_fd + 1, &readfds, NULL, NULL, NULL) >= 0, exit );

        if ( FD_ISSET( tcp_listen_fd, &readfds ) )
        {
            client_fd = accept( tcp_listen_fd, (struct sockaddr *) &client_addr, &sockaddr_t_size );
            if ( IsValidSocket( client_fd ) )
            {
                strcpy( client_ip_str, inet_ntoa( client_addr.sin_addr ) );
                tcp_server_log( "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
                if ( kNoErr
                     != mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP Clients",
                                                 tcp_client_thread,
                                                 0x800, client_fd ) )
                    SocketClose( &client_fd );

                if ( kNoErr
                     != mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP Clients Trans_tx",
                                                 tcp_client_tx_thread,
                                                 0x800, client_fd ) )
                    SocketClose( &client_fd );
            }
        }
    }
    exit:
    if ( err != kNoErr ) tcp_server_log( "Server listerner thread exit with err: %d", err );
    SocketClose( &tcp_listen_fd );
    mico_rtos_delete_thread( NULL );
}

#define uart_recv_log_trace() custom_log_trace("UART RECV")



void uartRecv_thread(uint32_t inContext)
{
	  uart_recv_log_trace();



	  inDataBuffer = malloc(UART_ONE_PACKAGE_LENGTH);
	  require(inDataBuffer, exit);

	  while(1) {
	    recvlen = _uart_get_one_packet(inDataBuffer, UART_ONE_PACKAGE_LENGTH);
	    if (recvlen <= 0)
	      continue;
//	    sppUartCommandProcess(inDataBuffer, recvlen, Context);
	    os_sem_log( "release semaphore" );
        mico_rtos_set_semaphore( &os_sem );
//	   	MicoUartSend(UART_FOR_APP, inDataBuffer, recvlen);
	  }

	exit:
	  if(inDataBuffer) free(inDataBuffer);
	  mico_rtos_delete_thread(NULL);

}

/*
//create udp socket
void udp_broadcast_thread( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );

    OSStatus err;
    struct sockaddr_in addr;
    int udp_fd = -1;

    fd_set readfds_sem;
    int len = 0;

    //Establish a UDP port to receive any data sent to this port//
    udp_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    require_action( IsValidSocket( udp_fd ), exit, err = kNoResourcesErr );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( LOCAL_UDP_PORT );

    err = bind( udp_fd, (struct sockaddr *) &addr, sizeof(addr) );
    require_noerr( err, exit );

    udp_broadcast_log("Start UDP broadcast mode, local port: %d, remote port: %d", LOCAL_UDP_PORT, REMOTE_UDP_PORT);

    while ( 1 )
    {
 //       udp_broadcast_log( "broadcast now!" );

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_BROADCAST;
        addr.sin_port = htons( REMOTE_UDP_PORT );
        //the receiver should bind at port=20000/
//        sendto( udp_fd, data, strlen( data ), 0, (struct sockaddr *) &addr, sizeof(addr) );

        FD_ZERO( &readfds_sem );
        FD_SET( semphr_fd, &readfds_sem );
        select( 24, &readfds_sem, NULL, NULL, NULL );

        if ( FD_ISSET( semphr_fd, &readfds_sem ) )
        {
             mico_rtos_get_semaphore( &os_sem, MICO_WAIT_FOREVER ); //wait until get semaphore
             MicoGpioOutputTrigger( MICO_SYS_LED);
             sendto( udp_fd, inDataBuffer, recvlen, 0, (struct sockaddr *) &addr, sizeof(addr) );
             os_sem_log( "get semaphore" );
         }

    }

    exit:
    if ( err != kNoErr )
        udp_broadcast_log("UDP thread exit with err: %d", err);
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
*/
volatile ring_buffer_t rx_buffer;
volatile uint8_t rx_data[UART_BUFFER_LENGTH];
char* data_buf = "liuruirui test program 2018.6.6 time 23:00 ";
int application_start( void )
{

	OSStatus err = kNoErr;
	network_InitTypeDef_st wNetConfig;
	uint32_t	data_len = 0;
//	char *data_buf = "liuruirui usart2 test";
	mico_uart_config_t uart_config;
    app_context_t* app_context;
    mico_Context_t* mico_context;
    uint8_t* inBuf;

  /* Start MiCO system functions according to mico_config.h*/
//  mico_system_init( mico_system_context_init( 0 ) );

    /* Create application context */
  app_context = (app_context_t *) calloc( 1, sizeof(app_context_t) );
  require_action( app_context, exit, err = kNoMemoryErr );

    /* Create mico system context and read application's config data from flash */
  mico_context = mico_system_context_init( sizeof(application_config_t) );
  app_context->appConfig = mico_system_context_get_user_data( mico_context );

    /*Register user function for MiCO nitification: WiFi status changed */
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler,
                                       NULL );
  require_noerr( err, exit );

  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr( err, exit );

  /*UART receive thread*/
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
  require_noerr_action( err, exit, os_helloworld_log("ERROR: Unable to start the uart recv thread.") );



  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

  strcpy((char*)wNetConfig.wifi_ssid, ap_ssid);
  strcpy((char*)wNetConfig.wifi_key, ap_key);

  wNetConfig.wifi_mode = Soft_AP;
  wNetConfig.dhcpMode = DHCP_Server;
  wNetConfig.wifi_retry_interval = 100;
  strcpy((char*)wNetConfig.local_ip_addr, "192.168.0.1");
  strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
  strcpy((char*)wNetConfig.dnsServer_ip_addr, "192.168.0.1");

  os_helloworld_log("ssid:%s  key:%s", wNetConfig.wifi_ssid, wNetConfig.wifi_key);\
  micoWlanStart(&wNetConfig);
  
  /* Output on debug serial port */
  os_helloworld_log( "liuruirui test program 2018.6.6 time 23:00" );

 // Start TCP server listener thread
  err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP_server", tcp_server_thread,
                                 0x800,
                                0 );
  require_noerr_string( err, exit, "ERROR: Unable to start the tcp server thread." );

  //Start UDP server listener thread
//  err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "udp_broadcast", udp_broadcast_thread, 0x800, 0 );
//  require_noerr_string( err, exit, "ERROR: Unable to start the UDP thread." );

  err = mico_rtos_init_semaphore( &os_sem, 1 ); //0/1 binary semaphore || 0/N semaphore
  require_noerr( err, exit );

  semphr_fd = mico_create_event_fd( os_sem );

  /* Trigger MiCO system led available on most MiCOKit */
  while(1)
  {
//	  MicoGpioOutputTrigger( MICO_SYS_LED);
	  mico_thread_msleep(1000);
  }

  exit:
  	  if ( err != kNoErr )
  		  os_sem_log( "Thread exit with err: %d", err );
  	  mico_rtos_delete_thread(NULL);
      return err;
}



