#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    int pp2c[2], pc2p[2];
    pipe(pp2c);
    pipe(pc2p);

    if(fork() != 0) {
        // 父进程先写后读
        write(pp2c[1], "p", 1);
  
        char buf;
        read(pc2p[0], &buf, sizeof buf);
        fprintf(1, "%d: received pong\n", getpid());
        wait(0);
    } else {
        // 子进程先读后写
        char buf;
        read(pp2c[0], &buf, sizeof buf);
        fprintf(1, "%d: received ping\n", getpid());
        write(pc2p[1], "c", 1);
    }

    exit(0);
}