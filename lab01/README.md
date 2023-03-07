# Lab: Xv6 and Unix utilities



## 1. sleep(easy)

​		Implement the UNIX program `sleep` for xv6; your `sleep` should pause for a user-specified number of ticks. A tick is a notion of time defined by the xv6 kernel, namely the time between two interrupts from the timer chip. Your solution should be in the file `user/sleep.c`.

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    if(argc != 2){
        fprintf(2, "usage: sleep times\n");
        exit(1);
    }

    int times = atoi(argv[1]);
    sleep(times);
    exit(0);
}
```

​	需要注意的是记得在Makefile中的**OBJS**部分加入sleep，往后都需要。

```makefile
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\
	$U/_sleep\  ###
```





## 2. pingpong(easy)

​		Write a program that uses UNIX system calls to ''ping-pong'' a byte between two processes over a pair of pipes, one for each direction. The parent should send a byte to the child; the child should print "<pid>: received ping", where <pid> is its process ID, write the byte on the pipe to the parent, and exit; the parent should read the byte from the child, print "<pid>: received pong", and exit. Your solution should be in the file `user/pingpong.c`.

```c
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
```





## 3. primes

​		Write a concurrent version of prime sieve using pipes. This idea is due to Doug McIlroy, inventor of Unix pipes. The picture halfway down [this page](http://swtch.com/~rsc/thread/) and the surrounding text explain how to do it. Your solution should be in the file `user/primes.c`.

​	可以将该任务想象一层一层的漏斗，数字就是沙子，每经过一层漏斗都会进行一次筛选(输出管道中的第一个数字，并且以此为倍数的数字全部淘汰)，每一对相邻的漏斗的关系可以理解成父子进程。

​	例如咱们以2、3....10为例子。

​	第一层漏斗输出2，并且淘汰以2为倍数的数字(4、6、8、10)，那么剩下3、5、7、9.（创建子进程并且写入，即第二层漏斗）

​	第二层漏斗输出3，淘汰6、9，剩下5、7.

​	。。。。。。

​	以上便是怎么在多进程的条件下做到素数的选择，但还有一个点也非常需要注意，就是无用的管道号需要即使进行关闭, 然后结束信号可以在管道最后插入一个-1来表示。



```c
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
```





## 4. find

​		Write a simple version of the UNIX find program: find all the files in a directory tree with a specific name. Your solution should be in the file `user/find.c`.

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define BUFSIZE 512
static char target[DIRSIZ]; // 存储目标文件名称
static char path[BUFSIZE];	// 存储当前所在的文件路径


// 在当前路径中，提取最后的一个文件的名称
char* fmtname()
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--);
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;

    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
    return buf;
}

void search()
{
    char *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot opens %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        // 如果是文件的话，直接进行判断
        case T_FILE:
            if(strcmp(target, fmtname()) == 0)
                printf("%s\n", path);
            break;
		// 如果是文件夹的话，需要先判断文件夹是否存在文件，并且不陷入自环(.和..)再继续递归
        case T_DIR:
            p = path+strlen(path);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(strcmp(p, ".") && strcmp(p, ".."))
                    search();
                memset(p, 0, DIRSIZ);
            }
            break;
    }
    close(fd);
}

int main(int argc,char* argv[]){
	if(argc != 3){
		fprintf(2,"find <pathname> <filename>\n");
		exit(1);
	}
	strcpy(path,argv[1]);
	strcpy(target,argv[2]);
	search();
	exit(0);
}
```





### 5. xargs

​		Write a simple version of the UNIX xargs program: read lines from the standard input and run a command for each line, supplying the line as arguments to the command. Your solution should be in the file `user/xargs.c`.

​	大致思路就是，将new_argv分为三部分进行操作，第一部分new_argv[0]存储为需要调用的功能的名称，第二部分new_argv[1]-new_argv[i-2]存储已有的供函数调用的参数，第三部分new_argv[i-1]存储每行额外的参数，且每次都会进行更新。

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc, char* argv[]) {
    char buf[12];
    int i = 2;
    if(argc < 3) {
        fprintf(2,"xargs <function> <args>\n");
		exit(1);
    }

    char* new_argv[MAXARG];
    new_argv[0] = argv[1];
    while(argv[i] != 0) {
        new_argv[i-1] = argv[i];
        if(i > MAXARG - 1){
			fprintf(2,"too many args\n");
			exit(1);
		}
		i++;
    }
    new_argv[i] = 0;

    char c, *p = buf;
    while(read(0, &c, sizeof c) != 0) {
        if(c == '\n') {
            *p = '\0';
            new_argv[i-1] = buf;
            if(fork() == 0) {
                exec(new_argv[0], new_argv);
                exit(0);
            }
            wait(0);
            p = buf;
        } else {
            *p++ = c;
        }
    }

    exit(0);
}
```

