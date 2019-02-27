// Copyright 2018 Mobvoi Inc. All Rights Reserved.
// Author: hongliang.che@mobvoi.com(Hongliang Che)
//         ljliu@mobvoi.com (Lijie Liu)

#ifndef UTILS_MOBVOI_SERIAL_H
#define UTILS_MOBVOI_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
  CMD_EXIT             = 0,
  CMD_ENABLE_BF        = 1,
  CMD_DISABLE_BF       = 2,
  CMD_GET_BF_STATUS    = 3,
  CMD_SET_BF_DIR       = 4,
  CMD_GET_DOA_DIR      = 5,
  CMD_GET_BF_DIRS      = 6,
  CMD_GET_WAKEUP_WORDS = 7,
  CMD_SET_LED_ON       = 8,
  CMD_SET_LED_OFF      = 9,
  CMD_DUMP_UART_HEX    = 98,
  CMD_SHOW_UART_LOG    = 99,
};

void send_command(int fd, int argc, int cmd, int param);
void process_results(void* msg);
int open_serial(const char* name, int speed, int databits, int stopbits, char parity);

#ifdef __cplusplus
}
#endif

#endif
