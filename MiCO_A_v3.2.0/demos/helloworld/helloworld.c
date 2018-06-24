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
#include "sci_APP.h"
#include "net_APP.h"
#include "sem_os_APP.h"
#include "vfd.h"
#include "can.h"


uint8_t *inDataBuffer;
int recvlen;

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


int application_start( void )
{

	OSStatus err = kNoErr;
    app_context_t* app_context;
    mico_Context_t* mico_context;

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
  err = app_sci_init();
  require_noerr_action( err, exit, os_helloworld_log("ERROR: Unable to start the uart recv thread.") );

  mico_vfd_init();

  //CAN1_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);

  err = app_net_init();
  require_noerr_string( err, exit, "ERROR: Unable to start the tcp server thread." );


  while(1)
  {
	  MicoGpioOutputTrigger( MICO_SYS_LED);
	  mico_thread_msleep(300);
  }

  exit:
  	  if ( err != kNoErr )
  		  os_sem_log( "Thread exit with err: %d", err );
  	  mico_rtos_delete_thread(NULL);
      return err;
}



