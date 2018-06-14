/*
 * net_APP.c
 *
 *  Created on: 2018Äê6ÔÂ12ÈÕ
 *      Author: Administrator
 */

#include "mico.h"
#include "MICOAppDefine.h"
#include "net_APP.h"
#include "sci_APP.h"
#include "sem_os_APP.h"

extern uint8_t *inDataBuffer;
extern int recvlen;


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


OSStatus app_net_init(void){

	network_InitTypeDef_st wNetConfig;
	OSStatus err = kNoErr;
	static char *ap_ssid = "mxchip_zfw";
	static char *ap_key = "12345678";

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
	// Output on debug serial port //
	os_helloworld_log( "liuruirui test program 2018.6.6 time 23:00" );

	// Start TCP server listener thread
	err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP_server", tcp_server_thread,
	                                 0x800,
	                                0 );
	return err;
}



