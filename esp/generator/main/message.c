#include <stdio.h>
#include "esp_timer.h"
#include "message.h"

void newMessage(message_t * m,  uint8_t *sa) {
    int i;
    m->version = 1;
    for (i=0;i<6;i++) m->sa[i] = sa[i];
}

void updateTimeStamp(message_t * m) {
    m->timeStamp = esp_timer_get_time();
}

void printMessage(message_t * m) {
    
    printf("time: %lli sa: %02x:%02x:%02x:%02x:%02x:%02x\n", \
    m->timeStamp, m->sa[0], m->sa[1], m->sa[2], m->sa[3], m->sa[4], m->sa[5]);
    
}