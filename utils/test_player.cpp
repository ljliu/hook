//
// Created by chehongliang on 19-1-29.
//

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "AudioPlayer.h"

void playPCM(const char* file)
{
  FILE* fd = fopen(file, "rb");

  AudioPlayer* player = new AudioPlayer(STREAMING);
  player->createStreamingAudioPlayer(16000, 1, 16 * 2 * 80);
  player->start();

  char buffer[2048];
  int size = 0;
  while((size = fread(buffer, 1, sizeof(buffer), fd)) > 0) {
    player->write(buffer, size, true);
  }

  player->stop();
  player->destoryAudioPlayer();
  delete player;

  fclose(fd);
}

static pthread_cond_t sCond;
static pthread_mutex_t sMutex;
static int sPlayComplete;
void playerCallback(int event)
{
  printf("playerCallback %d\n", event);
  if (event == PLAY_COMPLETE) {
    sPlayComplete = 1;
    pthread_mutex_lock(&sMutex);
    pthread_cond_signal(&sCond);
    pthread_mutex_unlock(&sMutex);
  }
}

void playUri(const char* uri)
{
  pthread_mutex_init(&sMutex, NULL);
  pthread_cond_init(&sCond, NULL);
  sPlayComplete = 0;
  AudioPlayer* player = new AudioPlayer(URI);
  player->createUriAudioPlayer(uri);
  player->setPlayerCallback(playerCallback);
  player->start();

  // Wait for play complete.
  pthread_mutex_lock(&sMutex);
  while(!sPlayComplete) {
    pthread_cond_wait(&sCond, &sMutex);
  }
  pthread_mutex_unlock(&sMutex);
  printf("player complete\n");

  player->stop();
  player->destoryAudioPlayer();
  delete player;

  pthread_cond_destroy(&sCond);
  pthread_mutex_destroy(&sMutex);
}

int main(int argc, char* argv[])
{
  char* rawFile = NULL;
  char* uri = NULL;
  while (*argv) {
    if (strcmp(*argv, "-raw") == 0) {
      argv++;
      if (*argv) {
        rawFile = *argv;
      }
    } else if (strcmp(*argv, "-file") == 0) {
      argv++;
      if (*argv) {
        uri = *argv;
        printf("uri: %s\n",uri);
      }
    }
    if (*argv)
      argv++;
  }

  if (rawFile != NULL) {
    playPCM(rawFile);
  } else if (uri != NULL){
    playUri(uri);
  }

  return 1;
}
