/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 */

#include <stdio.h>

#include "common/mbuf.h"
#include "common/platform.h"
#include "mgos_app.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"
#include "mgos_uart.h"

#define UART_NO 1

int state =0;
int counter = 0;

static void timer_cb(void *arg) {
  /*
   * Note: do not use mgos_uart_write to output to console UART (0 in our case).
   * It will work, but output may be scrambled by console debug output.
   */
  switch (state){
    case 0 :
//   printf("Open the topic Mongoose\r\n");
     mgos_uart_printf(UART_NO,"at+awspubopen=0,\"Mongoose\"\r\n");
     state = 1;
     break;
     
   case 1 :
//     printf("Publish to 0; 8 characters\r\n");
     mgos_uart_printf(UART_NO,"at+awspublish=0,8\r\n");
     state = 2;
     break;
   
   case 2 :
     // really should use dispatcher to wait for > prompt but we assume no errors. 
//     printf("send the 8 characters to the Module\r\n");
     mgos_uart_printf(UART_NO,"12345678");
     state = 3;
     break;
   
   default :
     printf (".\r\n");
     state++;
     break;

    case 10 : 
       printf("!\r\n");
       state= 1;
  }
  (void) arg;
}

int esp32_uart_rx_fifo_len(int uart_no);

static void uart_dispatcher(int uart_no, void *arg) {
  assert(uart_no == UART_NO);
  size_t rx_av = mgos_uart_read_avail(uart_no);
  if (rx_av > 0) {
    struct mbuf rxb;
    mbuf_init(&rxb, 0);
    mgos_uart_read_mbuf(uart_no, &rxb, rx_av);
    if (rxb.len > 0) {
      printf("%.*s", (int) rxb.len, rxb.buf);
    }
    mbuf_free(&rxb);
  }
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct mgos_uart_config ucfg;
  mgos_uart_config_set_defaults(UART_NO, &ucfg);
  /*
   * At this point it is possible to adjust baud rate, pins and other settings.
   * 115200 8-N-1 is the default mode, but we set it anyway
   */
  ucfg.baud_rate = 9600;
  ucfg.num_data_bits = 8;
  ucfg.parity = MGOS_UART_PARITY_NONE;
  ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
  if (!mgos_uart_configure(UART_NO, &ucfg)) {
    return MGOS_APP_INIT_ERROR;
  }
  state = 0; // initialised state
  mgos_set_timer(2000 /* ms */, true /* repeat */, timer_cb, NULL /* arg */);

  mgos_uart_set_dispatcher(UART_NO, uart_dispatcher, NULL /* arg */);
  mgos_uart_set_rx_enabled(UART_NO, true);
  
//  printf("Open the topic Mongoose\r\n");
  mgos_uart_printf(UART_NO,"at\r\n");

  return MGOS_APP_INIT_SUCCESS;
}
