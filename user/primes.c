#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int MAX = 35;

void process(int *p1){
    close(p1[1]);

    int num;

    read(p1[0], &num, 4);
    fprintf(1, "prime %d\n", num);

    int p2[2];
    pipe(p2);
    int is_fork = 0;
    int temp;
    while (read(p1[0], &temp, 4) != 0) {
        if (temp % num) {
            if (!is_fork) {
                if (fork() == 0) {
                    process(p2);
                }
                is_fork = 1;
            }
            write(p2[1], &temp, 4);
        }
    }
    close(p1[0]);
    close(p2[0]);
    close(p2[1]);

    if (is_fork) {
        wait((void *)0);
    }

    exit(0);

}

int main (int argc, char **argv) {
    int p[2];
    pipe(p);
    
    if (fork() == 0) {
        process(p);
    }

    close(p[0]);

    for (int i = 2; i <= MAX;i++) {
        write(p[1], &i, 4);
    }

    close(p[1]);
    wait((void *)0);
    exit(0);
}