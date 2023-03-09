#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv)
{
    char ball = 'q';

    int p1[2];
    pipe(p1);
    int p2[2];
    pipe(p2);

    write(p1[1], &ball, 1);
    if (fork() == 0)
    {
        // char *buff = (char*)malloc(1);
        
        read(p1[0], (void *)0, 1);
        fprintf(1, "%d: received ping\n", getpid());
        write(p2[1], (void *)0, 1);

        // free(buff);
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        exit(0);
    }

    // char *buff = (char*)malloc(1);
    read(p2[0], (void *)0, 1);
    fprintf(1, "%d: received pong\n", getpid());

    // free(buff);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    exit(0);
}