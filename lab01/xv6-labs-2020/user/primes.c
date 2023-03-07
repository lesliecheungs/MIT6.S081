#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int pleft[2]) {
    int p;
    read(pleft[0], &p, sizeof p);
    if(p == -1) exit(0);
    fprintf(1, "prime %d\n", p);

    int pright[2];
    pipe(pright);
    if(fork() != 0) {
        close(pright[0]);
        int buf;
        while(read(pleft[0], &buf, sizeof buf) && buf != -1) {
            if(buf%p != 0) {
                write(pright[1], &buf, sizeof buf);
            }
        }
        buf = -1;
        write(pright[1], &buf, sizeof buf);
        wait(0);
        exit(0);
    } else {
        close(pright[1]);
        close(pleft[0]);
        sieve(pright);
    }
}
int main(int argc, char* argv[]) {
    int ipipe[2];
    pipe(ipipe);

    if(fork() != 0) {
        close(ipipe[0]);
        int i;
        for(i = 2; i <= 35; i++) {
            write(ipipe[1], &i, sizeof i);
        }
        i = -1;
        write(ipipe[1], &i, sizeof i);
        close(ipipe[1]);
    } else {
        close(ipipe[1]);
        sieve(ipipe);
        exit(0);
    }

    wait(0);
    exit(0);
}