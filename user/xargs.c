#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(2, "xargs: need more args");
        exit(1);
    }
    char c;
    char buf[512];
    char *p = buf;
    char *params[MAXARG];
    while(read(0, &c, 1) != 0) {
        if (c == '\n') {
            *p = 0;
            if (fork() == 0) {
                int i;
                for (i = 1; i < MAXARG && i < argc; i++) {
                    params[i-1] = argv[i];
                }
                if (i >= MAXARG) {
                    fprintf(2, "xargs: too many args");
                    exit(1);
                }
                params[i-1] = buf;
                params[i] = 0;
                exec(argv[1], params);
                fprintf(2, "xargs: %s failed", argv[1]);
            }
            wait(0);
            p = buf;
        }
        else {
            *p++ = c;
        }

    }
    exit(0);
}