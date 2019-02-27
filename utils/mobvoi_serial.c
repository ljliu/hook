// Copyright 2018 Mobvoi Inc. All Rights Reserved.
// Author: hongliang.che@mobvoi.com(Hongliang Che)
//         ljliu@mobvoi.com (Lijie Liu)

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "third_party/mobvoidsp/include/mobvoi_msg.h"
#include "utils/mobvoi_serial.h"

#define LOG_TAG "SERIAL"
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

void send_command(int fd, int argc, int cmd, int param) {
  int msg_len = 0;
  char msg_buff[MAX_MSG_LEN] = {0};
  mob_gs_msg* msg = (mob_gs_msg*) msg_buff;
  switch (cmd) {
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
    default: {
      SerialLog("Unknown cmd %d\n", cmd);
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

    int ret = write(fd, msg, msg_len);
    if (ret < 0) {
      SerialLog("Wrote %d %d\n", ret, errno);
    }
  }
}

void process_results(void* buff) {
  mob_gs_msg* msg = (mob_gs_msg*) buff;
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

int open_serial(const char* name, int speed, int databits, int stopbits, char parity) {
  int flags = O_NONBLOCK | O_NOCTTY;
  int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300};
  int name_arr[]  = {  115200,  57600,  38400,  19200,  9600,  4800,  2400,  1200,  300};

  int fd = open(name, O_RDWR | flags);
  if (fd == -1) {
    SerialLog("Cannot open port\n");
    return -1;
  }

  tcdrain(fd);
  tcflush(fd,TCIOFLUSH);

  struct termios cfg;
  if (tcgetattr(fd, &cfg)) {
    SerialLog("tcgetattr() failed\n");
    close(fd);
    return -1;
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
      return -1;
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
      return -1;
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
      return -1;
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
    return -1;
  }

  SerialLog("Configuring serial port done\n");
  return fd;
}
