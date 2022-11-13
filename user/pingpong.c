#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv)
{
    char ball = 'q';

    int p[2];
    pipe(p);

    write(p[0], &ball, 1);
    if (fork() == 0)
    {
        // char *buff = (char*)malloc(1);
        
        read(p[1], (void *)0, 1);
        fprintf(1, "%d: received ping\n", getpid());
        write(p[1], (void *)0, 1);

        // free(buff);
        close(p[0]);
        close(p[1]);
        exit(0);
    }

    // char *buff = (char*)malloc(1);
    read(p[0], (void *)0, 1);
    fprintf(1, "%d: received pong\n", getpid());

    // free(buff);
    close(p[0]);
    close(p[1]);
    exit(0);
}