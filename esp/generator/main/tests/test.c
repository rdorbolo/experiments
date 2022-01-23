#include <stdio.h>

int main() {
printf("hello world\n");


//char * string = "data/rick=2/haleh=3";
char * string = "data/rick=2,3";
char tmp[20];

int rick = -1;
int haleh = -1;

//sscanf(string, "data/rick=%d,%d", &rick,&haleh);
sscanf(string, "%srick=%d,%d", tmp, &rick,&haleh);

printf("%s\n", string);
printf("tmp=%s\n",tmp);
printf("rick=%d\n",rick);
printf("haleh=%d\n",haleh);
}
