//

typedef struct message
{
    uint8_t version;
    uint8_t sa[6];
    int64_t timeStamp;
    uint8_t reserved[236];
} message_t;


void newMessage(message_t * m,  uint8_t *sa);
void updateTimeStamp(message_t * m);
void printMessage(message_t *m);
