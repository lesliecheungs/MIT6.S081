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