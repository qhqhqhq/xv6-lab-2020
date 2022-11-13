#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv){
    int ticks;

    if(argc != 2){
        fprintf(1, "usage: echo time\n");
        exit(1);
    }

    ticks = atoi(argv[1]);
    sleep(ticks);
    exit(0);
}