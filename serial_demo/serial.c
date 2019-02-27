// Copyright 2018 Mobvoi Inc. All Rights Reserved.
// Author: hongliang.che@mobvoi.com(Hongliang Che)
//         ljliu@mobvoi.com (Lijie Liu)

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "third_party/mobvoidsp/include/mobvoi_msg.h"

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

#define SEND_TEST_COMMAND
#define LOG_TAG "DEMO"

static inline void SerialLog(const char* fmt, ...) {
  va_list vl;
  char s[1024] = {0};
  va_start(vl, fmt);
  vsprintf(s, fmt, vl);
  va_end(vl);

  struct timeval tv;
  gettimeofday(&tv, NULL);

  struct tm timeinfo;
  localtime_r(&tv.tv_sec, &timeinfo);

  printf("%04d-%02d-%02d %02d:%02d:%02d.%03ld %s: %s",
         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, tv.tv_usec / 1000,
         LOG_TAG, s);
}

void Usage() {
  SerialLog(
    "\nCommand list:\n"
    "\t0      : Exit\n"
    "\t1      : Enable beamforming\n"
    "\t2      : Disable beamforming\n"
    "\t3      : Get beamforming status\n"
    "\t4 <dir>: Set beamforming direction\n"
    "\t5      : Get DOA direction\n"
    "\t6      : Get all beamforming directions\n"
    "\t7      : Get wakeup words\n"
    "\t8 <dir>: Turn on LED\n"
    "\t9      : Turn off LED\n"
    "\t98     : Switch hex dumpping\n"
    "\t99     : Switch showing log from UART\n"
    "\tENTER  : Repeat last command\n"
    );
}

static int exit_flag = 0;
static int show_log = 1;
static int dump_hex = 0;
static int test_mode = 0;
void send_command(void* fd, int argc, int cmd, int param);

#ifdef SEND_TEST_COMMAND
void* send_test_command(void* fd) {
  int cmds[] = {4, 5, 6, 7};
  while (exit_flag == 0) {
    int cmd = cmds[rand() % (sizeof(cmds) / sizeof(cmds[0]))];
    int argc = 1;
    if (cmd == 4) {
      send_command(fd, 2, cmd, rand() % 360);
    } else {
      send_command(fd, 1, cmd, 0);
    }

    sleep(rand() % 10 + 1);
  }
}
#endif

void* send_commands(void* fd) {
  char line[64];
  int cmd = 0;
  int param = 0;
  int argc = 0;
  while (exit_flag == 0 && fgets(line, sizeof(line), stdin) != NULL) {
    if (line[0] != '\n' || cmd == 0) {
      argc = sscanf(line, "%d %d", &cmd, &param);
      if (argc < 1) {
        Usage();
        continue;
      }
    }
    send_command(fd, argc, cmd, param);
  }

  return NULL;
}

void send_command(void* fd, int argc, int cmd, int param) {
  int msg_len = 0;
  char msg_buff[MAX_MSG_LEN] = {0};
  mob_gs_msg* msg = (mob_gs_msg*) msg_buff;
  switch (cmd) {
    case CMD_EXIT: {
      exit_flag = 1;
      SerialLog("Exiting\n");
      break;
    }
    case CMD_ENABLE_BF: {
      msg->event = EVENT_FROM_HOST_ENABLE_BF;
      msg->payload_len = 0;
      msg_len = sizeof(mob_gs_msg);
      break;
    }
    case CMD_DISABLE_BF: {
      msg->event = EVENT_FROM_HOST_DISABLE_BF;
      msg->payload_len = 0;
      msg_len = sizeof(mob_gs_msg);
      break;
    }
    case CMD_GET_BF_STATUS: {
      msg->event = EVENT_FROM_HOST_GET_BF_STATUS;
      msg->payload_len = 0;
      msg_len = sizeof(mob_gs_msg);
      break;
    }
    case CMD_SET_BF_DIR: {
      if (argc < 2) {
        SerialLog("Parameter error\n");
        Usage();
        break;
      }
      msg->event = EVENT_FROM_HOST_SET_BF_DIR;
      msg->payload_len = 2;
      mob_short2bytes(param, msg->payload_data);
      msg_len = sizeof(mob_gs_msg) + msg->payload_len;
      break;
    }
    case CMD_GET_DOA_DIR: {
      msg->event = EVENT_FROM_HOST_GET_DOA_DIR;
      msg->payload_len = 0;
      msg_len = sizeof(mob_gs_msg);
      break;
    }
    case CMD_GET_BF_DIRS: {
      msg->event = EVENT_FROM_HOST_GET_BF_DIR_LIST;
      msg->payload_len = 0;
      msg_len = sizeof(mob_gs_msg);
      break;
    }
    case CMD_GET_WAKEUP_WORDS: {
      msg->event = EVENT_FROM_HOST_GET_WAKEUP_WORD;
      msg->payload_len = 0;
      msg_len = sizeof(mob_gs_msg);
      break;
    }
    case CMD_SET_LED_ON: {
      if (argc < 2) {
        SerialLog("Parameter error\n");
        Usage();
        break;
      }
      msg->event = EVENT_FROM_HOST_SET_LED;
      msg->payload_len = 2;
      short leds = 1 << param;
      mob_short2bytes(leds, msg->payload_data);
      msg_len = sizeof(mob_gs_msg) + msg->payload_len;
      break;
    }
    case CMD_SET_LED_OFF: {
      msg->event = EVENT_FROM_HOST_SET_LED;
      msg->payload_len = 2;
      short leds = 0;
      mob_short2bytes(leds, msg->payload_data);
      msg_len = sizeof(mob_gs_msg) + msg->payload_len;
      break;
    }
    case CMD_DUMP_UART_HEX: {
      dump_hex = !dump_hex;
      SerialLog("Flag of dump is %s\n", dump_hex ? "ON" : "OFF");
      break;
    }
    case CMD_SHOW_UART_LOG: {
      show_log = !show_log;
      SerialLog("Flag of log is %s\n", show_log ? "ON" : "OFF");
      break;
    }
    default: {
      Usage();
      break;
    }
  }
  if (msg_len > 0) {
    msg->sync_tag[0] = SYNC_TAG[0];
    msg->sync_tag[1] = SYNC_TAG[1];
    msg->sync_tag[2] = SYNC_TAG[2];
    msg->sync_tag[3] = SYNC_TAG[3];
    msg->check_bits = 0;
    msg->reserved = 0;

#ifdef SEND_TEST_COMMAND
    if (test_mode) {
      msg_len = 10000;
    }
#endif

    int ret = write(*(int*)fd, msg, msg_len);
    if (ret < 0) {
      SerialLog("Wrote %d %d\n", ret, errno);
    }
  }
}

void process_results(mob_gs_msg* msg) {
  SerialLog("Event: 0x%02hhX\n", msg->event);
  switch (msg->event) {
    case EVENT_TO_HOST_WAKEUP: {
      SerialLog("Waked up\n");
      break;
    }
    case EVENT_TO_HOST_WAKEUP_WORDS: {
      SerialLog("Wake up words: ");
      char* buff = msg->payload_data;
      while (buff < (msg->payload_data + msg->payload_len)) {
        printf("%s ", buff);
        buff += strlen(buff) + 1;
      }
      printf("\n");
      break;
    }
    case EVENT_TO_HOST_DOA_DIR: {
      if (msg->payload_len < sizeof(short)) {
        SerialLog("Invalid payload len %d\n", msg->payload_len);
      } else {
        SerialLog("Doa dir: %d\n", mob_bytes2short(msg->payload_data));
      }
      break;
    }
    case EVENT_TO_HOST_BF_DIR_LIST: {
      if (msg->payload_len < sizeof(short)) {
        SerialLog("Invalid payload len %d\n", msg->payload_len);
      } else {
        SerialLog("BF dir list:");
        for (int i = 0; i < msg->payload_len / sizeof(short); i++) {
          printf(" %d",
              mob_bytes2short(msg->payload_data + i * sizeof(short)));
        }
        printf("\n");
      }
      break;
    }
    case EVENT_TO_HOST_BF_STATUS: {
      if (msg->payload_len < sizeof(short)) {
        SerialLog("Invalid payload len %d\n", msg->payload_len);
      } else {
        SerialLog("BF dir: %d, ", mob_bytes2short(msg->payload_data));
        printf("%s\n",
            mob_bytes2short(msg->payload_data + 2) ? "enabled" : "disabled");
      }
      break;
    }
    default: {
      SerialLog("Unknown event: %d\n", msg->event);
      break;
    }
  }
}

void* recv_results(void* fd) {
  char msg_buff[MAX_MSG_LEN] = {0};
  int last_remain = 0;
  while (exit_flag == 0) {
    char* buff = msg_buff + last_remain;
    int size = read(*(int*)fd, buff, MAX_MSG_LEN - last_remain);
    // printf("read out %d, %d\n", size, errno);
    if (size <= 0) {
      sleep(1);
      continue;
    }
    if (size > 0) {
      if (show_log) printf("%s", buff);
      if (dump_hex) {
        SerialLog("Msg arrived(%d):", size);
        for (int i = 0; i < size; i++) {
          printf(" %02hhX", buff[i]);
        }
        printf("\n");
      }
    }

    buff = msg_buff;
    size += last_remain;
    while (size >= sizeof(mob_gs_msg)) {
      if (buff[0] != SYNC_TAG[0] || buff[1] != SYNC_TAG[1] ||
          buff[2] != SYNC_TAG[2] || buff[3] != SYNC_TAG[3]) {
        buff++;
        size--;
        // printf("sync failed! discard 1 byte\n");
        continue;
      }

      mob_gs_msg* msg = (mob_gs_msg*) buff;
      if ((sizeof(mob_gs_msg) + msg->payload_len) > MAX_MSG_LEN) {
          size -= sizeof(mob_gs_msg);
          buff += sizeof(mob_gs_msg);
          SerialLog("Invalid payload len, discard msg\n");
          continue;
      }
      if (size < (sizeof(mob_gs_msg) + msg->payload_len)) break;

      process_results(msg);
      int processed = sizeof(mob_gs_msg) + msg->payload_len;
      buff += processed;
      size -= processed;
    }

    last_remain = size;
    if (size > 0) {
      memmove(msg_buff, buff, size);

      if (dump_hex) {
        SerialLog("Remain data(%d):", size);
        for(int i = 0; i < size; i++) {
            printf(" %02hhX", msg_buff[i]);
        }
        printf("\n");
      }
    }

    memset(msg_buff + size, 0, MAX_MSG_LEN - size);
  }
  return NULL;
}

int open_serial(const char* name, int speed, int databits, int stopbits, char parity) {
  int flags = O_NONBLOCK | O_NOCTTY;
  int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300};
  int name_arr[]  = {  115200,  57600,  38400,  19200,  9600,  4800,  2400,  1200,  300};

  int fd = open(name, O_RDWR | flags);
  if (fd == -1) {
    SerialLog("Cannot open port\n");
    return 0;
  }

  tcdrain(fd);
  tcflush(fd,TCIOFLUSH);

  struct termios cfg;
  if (tcgetattr(fd, &cfg)) {
    SerialLog("tcgetattr() failed\n");
    close(fd);
    return 0;
  }

  for (int i= 0; i < (int)(sizeof(speed_arr) / sizeof(int)); i++) {
    if  (speed == name_arr[i]) {
      if (cfsetispeed(&cfg, speed_arr[i]) != 0)
        SerialLog("Serial config cfsetispeed error\n");
      if (cfsetospeed(&cfg, speed_arr[i]) != 0)
        SerialLog("Serial config cfsetospeed error\n");
      break;
    }
  }
  cfg.c_cflag &= ~CSIZE;
  switch (databits) {
    case 7:
      cfg.c_cflag |= CS7;
      break;
    case 8:
      cfg.c_cflag |= CS8;
      break;
    default:
      SerialLog("Unsupported data size\n");
      close(fd);
      return 0;
  }

  switch (parity) {
    case 'n':
    case 'N':
      cfg.c_cflag &= ~PARENB;
      cfg.c_iflag &= ~INPCK;
      break;
    case 'o':
    case 'O':
      cfg.c_cflag |= (PARODD | PARENB);
      cfg.c_iflag |= INPCK;
      break;
    case 'e':
    case 'E':
      cfg.c_cflag |= PARENB;
      cfg.c_cflag &= ~PARODD;
      cfg.c_iflag |= INPCK;
      break;
    case 'S':
    case 's':
      cfg.c_cflag &= ~PARENB;
      cfg.c_cflag &= ~CSTOPB;
      break;
    default:
      SerialLog("Unsupported parity\n");
      close(fd);
      return 0;
  }

  switch (stopbits)
  {
    case 1:
      cfg.c_cflag &= ~CSTOPB;
      break;
    case 2:
      cfg.c_cflag |= CSTOPB;
      break;
    default:
      SerialLog("Unsupported stop bits\n");
      close(fd);
      return 0;
  }

  if (parity != 'n') {
    cfg.c_iflag |= INPCK;
  }
  cfg.c_cc[VTIME] = 150;
  cfg.c_cc[VMIN] = 0;

  cfg.c_cflag &= ~CRTSCTS;
  cfg.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  cfg.c_oflag &= ~OPOST;
  cfg.c_iflag &= ~(INLCR | ICRNL | IGNCR | IXON);

  if (tcsetattr(fd, TCSANOW, &cfg)) {
    SerialLog("tcsetattr() failed\n");
    close(fd);
    return 0;
  }

  SerialLog("Configuring serial port done\n");
  return fd;
}

// echo 0x1d6b 0xA4A6 >/sys/bus/usb-serial/drivers/generic/new_id

int main(int argc, char * argv[]) {
  const char* p_file = "/dev/ttyUSB0";
  while (*argv) {
    if (strcmp(*argv, "-f") == 0) {
      argv++;
      if (*argv) {
        p_file = *argv;
      }
    } else if (strcmp(*argv, "-test") == 0) {
      test_mode = 1;
    }
    if (*argv)
      argv++;
  }

  int fd = open_serial(p_file, 115200, 8, 1, 'N');
  if (fd <= 0) {
    SerialLog("Failed to open %s\n", p_file);
    return 1;
  }

  pthread_t send_tid, recv_tid;

#ifdef SEND_TEST_COMMAND
  pthread_t test_tid1, test_tid2;
  if (test_mode) {
    time_t t;
    srand((unsigned) time(&t));
    pthread_create(&test_tid1, NULL, send_test_command, &fd);
    pthread_create(&test_tid2, NULL, send_test_command, &fd);
  }
#endif

  pthread_create(&send_tid, NULL, send_commands, &fd);
  pthread_create(&recv_tid, NULL, recv_results, &fd);

#ifdef SEND_TEST_COMMAND
  if (test_mode) {
    pthread_join(test_tid1, NULL);
    pthread_join(test_tid2, NULL);
  }
#endif

  pthread_join(send_tid, NULL);
  pthread_join(recv_tid, NULL);

  close(fd);
}
