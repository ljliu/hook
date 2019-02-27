//
// Created by chehongliang on 19-1-25.
//

#include <stdio.h>

#include "MobPipeline.h"

static void speechCallback(void* ud, char* buffer, int length)
{
}

int main()
{
    printf("mob dsp demp\n");
    MobPipeline* pipeline = new MobPipeline(speechCallback, NULL);
    pipeline->start();

    char c;
    while((c = getchar()) > 0) {
        if (c == 'q' || c == 'Q') {
            break;
        }
    }

    pipeline->stop();
    delete pipeline;
}
