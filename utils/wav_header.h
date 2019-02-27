// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: congfu@mobvoi.com (Cong Fu)

#ifndef SDK_WAV_HEADER_H
#define SDK_WAV_HEADER_H

#define WAVE_FORMAT_PCM         ((short)0x0001)
#define WAVE_FORMAT_IEEE_FLOAT  ((short)0x0003)
#define WAVE_FORMAT_ALAW        ((short)0x0006)
#define WAVE_FORMAT_MULAW       ((short)0x0007)
#define WAVE_FORMAT_EXTENSIBLE  ((short)0xFFFE)

// WAV header spec information:
// https://web.archive.org/web/20140327141505/https://ccrma.stanford.edu/
// courses/422/projects/WaveFormat/
// http://www.topherlee.com/software/pcm-tut-wavformat.html

typedef struct wav_header {
  // RIFF Header, contains "RIFF"
  char riff_header[4];
  // Size of the wav portion of the file, which follows the first 8 bytes.
  // File size - 8
  int wav_size;
  // Contains "WAVE"
  char wave_header[4];

  // Format Header, contains "fmt " (includes trailing space)
  char fmt_header[4];
  // Should be 16 for PCM
  int fmt_chunk_size;
  // Should be 1 for PCM. 3 for IEEE Float
  short audio_format;
  short num_channels;
  int sample_rate;
  // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
  int byte_rate;
  // num_channels * Bytes Per Sample
  short sample_alignment;
  // Number of bits per sample
  short bit_depth;

  // Data，Contains "data"
  char data_header[4];
  // Number of bytes in data. Number of samples * num_channels * sample byte size
  int data_bytes;
  // Remainder of wave file is bytes
  // uint8_t bytes[];
} wav_header;

static int isRiffHeader(wav_header* header) {
  return header->riff_header[0] == 'R' && header->riff_header[1] == 'I' &&
      header->riff_header[2] == 'F' && header->riff_header[3] == 'F';
}

static int isWaveHeader(wav_header* header) {
  return header->wave_header[0] == 'W' && header->wave_header[1] == 'A' &&
      header->wave_header[2] == 'V' && header->wave_header[3] == 'E';
}

static int isFmtHeader(wav_header* header) {
  return header->fmt_header[0] == 'f' && header->fmt_header[1] == 'm' &&
      header->fmt_header[2] == 't' && header->fmt_header[3] == ' ';
}

static int isDataHeader(wav_header* header) {
  return header->data_header[0] == 'd' && header->data_header[1] == 'a' &&
      header->data_header[2] == 't' && header->data_header[3] == 'a';
}

static void wav_header_init(wav_header* p_header) {
  p_header->riff_header[0] = 'R';
  p_header->riff_header[1] = 'I';
  p_header->riff_header[2] = 'F';
  p_header->riff_header[3] = 'F';
  p_header->wave_header[0] = 'W';
  p_header->wave_header[1] = 'A';
  p_header->wave_header[2] = 'V';
  p_header->wave_header[3] = 'E';
  p_header->fmt_header[0] = 'f';
  p_header->fmt_header[1] = 'm';
  p_header->fmt_header[2] = 't';
  p_header->fmt_header[3] = ' ';
  p_header->data_header[0] = 'd';
  p_header->data_header[1] = 'a';
  p_header->data_header[2] = 't';
  p_header->data_header[3] = 'a';

  // Default values for PCM
  p_header->fmt_chunk_size = 16;
  p_header->audio_format = WAVE_FORMAT_PCM;
}

#endif  // SDK_WAV_HEADER_H
