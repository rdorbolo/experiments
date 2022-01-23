//

#define ALT_GPIO_PIN 15

enum State
{
    reset = 1,
    ready = 2,
    running = 3,
    stopped = 5,
    adcLow = 6,
    adcHigh = 7,
    linkBad = 8
};

typedef struct Stat
{
    int sampleCnt;
    int sampleSum;
    int currentSample;
    int maxValue;
    int minValue;
} Stat_t;

void startTach();
