# lab1 xv6 and unix utils

## sleep

### 实现
```c
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
```
简单的sleep程序
- 使用sleep系统调用(`user/user.h`中)
- 检查参数数量是否合法
- 使用提供的ulib库中的`atoi()`进行字符串转换

### sleep系统调用分析
`kernel/sysproc.c`中实现了sleep系统调用(内核中运行)
```c
uint64
sys_sleep(void)
{
  int n; // 存储系统调用的参数
  uint ticks0; // 存储开始时间

  if(argint(0, &n) < 0) // 获取系统调用的参数, 存入n
    return -1;
  acquire(&tickslock); // 获取计时器(ticks)的自旋锁
  ticks0 = ticks; // 记录开始时间
  while(ticks - ticks0 < n){ // 当时间未到, 循环
    if(myproc()->killed){ 
        // myproc()获取当前进程的struct proc, 它记录进程状态信息
        // 如果进程状态中killed字段非0, 则进程已经结束, 直接返回
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock); // 传入当前时钟周期(ticks)和ticks的锁, 设置状态后, 在调度器调度下进入休眠, 在程序休眠的状态下释放tickslock, 唤醒时重新获得tickslock
  }
  release(&tickslock); // 释放tickslock
  return 0;
}
```

### sleep系统调用从用户态跳转入内核态
`user/usys.S`中. 程序运行时, 语句跳转到`sleep`标记处, 执行如下指令
```
sleep:
 li a7, SYS_sleep
 ecall
 ret
```
即将系统调用号存入a7寄存器, ecall从a7寄存器获取系统调用号进行系统调用, 系统调用的参数保存在a0寄存器中

## pingpong

### 实现

```c
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
        
        read(p1[0], (void *)0, 1);
        fprintf(1, "%d: received ping\n", getpid());
        write(p2[1], (void *)0, 1);

        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        exit(0);
    }

    read(p2[0], (void *)0, 1);
    fprintf(1, "%d: received pong\n", getpid());

    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    exit(0);
}
```
`pipe(p)`系统调用会创建一个管道, 将读的一端的文件描述符存入p[0], 写的一端存入p[1].

使用两个管道, 一个用于从父进程向子进程发送`ping`的信号, 另一个用于当子进程接收到信号后向父进程返回`pong`的信号

## primes

### 原理

这个部分实现一个素数筛, 原理图如下. 通过多进程, 实现输出指定范围内所有素数的效果:

![img](note.assets/sieve.gif)

- 每个进程的第一个输入必为素数
- 后续输入中, 如果有数为第一个数的整数倍, 则排除这个数
- 如果不是整数倍, 传递到下一个进程中判断

### 实现

主要使用`fork()`和`pipe()`系统调用, 实现上述素数筛

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int MAX = 35;

// 递归调用, 当需要进行下一轮筛选时, 启动新的进程
void process(int *p1){
    close(p1[1]); // 提前关闭写管道的描述符, 则当父进程传输完所有数字后, 写管道关闭

    int num;

    read(p1[0], &num, 4); // 从管道读入第一个数字, 这个数字必为素数
    fprintf(1, "prime %d\n", num);

    int p2[2];
    pipe(p2); // 打开输出管道, 向下一个筛输入数字
    int is_fork = 0;
    int temp;
    // 不断从管道中读取数字
    while (read(p1[0], &temp, 4) != 0) {
        // 当不可被整除, 将数字送入下一轮筛选
        if (temp % num) {
            // 对于第一个需要下一轮筛选的数字, 创建子进程(即下一个筛), 递归调用
            if (!is_fork) {
                if (fork() == 0) {
                    process(p2);
                }
                is_fork = 1;
            }
            // 创建的子进程会读取这里写入的数字
            write(p2[1], &temp, 4);
        }
    }
    close(p1[0]);
    close(p2[0]);
    close(p2[1]);

    // 等待子进程结束后, 本进程才能退出
    if (is_fork) {
        wait((void *)0);
    }

    // 从最底部进程开始, 结束进程, 跳出递归
    exit(0);

}

int main (int argc, char **argv) {
    int p[2];
    pipe(p); // 第一个筛的输入管道
    
    if (fork() == 0) {
        process(p); // 启动第一个筛
    }

    close(p[0]);

    // 将所有数字依次输入筛中
    for (int i = 2; i <= MAX;i++) {
        write(p[1], &i, 4);
    }

    close(p[1]); // 所有输入完成后, 关闭管道, 这样子进程对管道的读(read)会返回0, 标志输入结束
    wait((void *)0); // 等待子进程结束
    exit(0);
}
```

## find

实现一个简单的unix的find指令, 输入两个参数, 第一个为查找的工作目录, 第二个为查找的目标文件

### 实现

参考`user/ls.c`, 其中可以参考的点:

- 系统有三种文件类型: 目录, 文件, 设备. 均可以通过open系统调用打开
- 通过fstat系统调用获取文件元数据. 包含类型, 索引节点等信息
- 对于目录文件, 可以使用read读取, 读取结果为目录项, 包括目录下的文件名

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char pwd[512];
char *p = pwd;

void find(char *dirname, char *filename) {
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(dirname, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", dirname);
        return;
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", dirname);
        close(fd);
        return;
    }
    if (st.type != T_DIR) {
        fprintf(2, "find: %s is not directory\n", dirname);
        close(fd);
        return;
    }
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        if (!strcmp(".", de.name) || !strcmp("..", de.name) ) continue;
        
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(pwd, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", pwd);
            continue;
        }
        
        switch (st.type)
        {
        case T_FILE:
            if (!strcmp(filename, de.name)) {
                fprintf(1, "%s\n", pwd);
            }
            break;
        case T_DIR:
            char *temp_p = p;
            p = p + strlen(de.name);
            *p++ = '/';
            *p = 0;
            find(pwd, filename);
            p = temp_p;
            break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "find: argc wrong\n");
        exit(0);
    }

    char *dirname = argv[1];
    char *filename = argv[2];

    strcpy(pwd, dirname);
    p = pwd + strlen(dirname);
    *p++ = '/';
    *p = 0;

    find(dirname, filename);

    exit(0);
}
```

实现思路: 遍历目标目录的目录项, 如果是目录, 则继续递归调用; 如果是文件, 则比较是否是目标文件, 输出结果

> 代码中使用了一个全局的pwd表示从工作目录开始的完整路径, 方便进行参数传递以及记录完整名称等

## xargs

这条指令从标准输入读入多行字符串, 以每行字符串作为参数, 一行字符串执行一遍目标指令

```bash
$ echo world | xargs echo hello
hello world
$ echo "1\n2" | xargs echo line
line 1
line 2
```

### 实现

使用`fork()`以及`exec()`, 创建新的进程, 重复执行多次指令

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(2, "xargs: need more args");
        exit(1);
    }
    char c; // 读入字符
    char buf[512]; // 存放一行的字符串
    char *p = buf;
    char *params[MAXARG]; // exec 的参数, 参数数组尾为0, 标志参数数组的结束
    while(read(0, &c, 1) != 0) { // 从标准输入逐个字符读入
        if (c == '\n') { // '\n'表示一行的结束
            *p = 0; // 字符串尾部为'\0'
            if (fork() == 0) {
                int i;
                // 构造参数数组
                for (i = 1; i < MAXARG && i < argc; i++) {
                    params[i-1] = argv[i];
                }
                if (i >= MAXARG) {
                    fprintf(2, "xargs: too many args");
                    exit(1);
                }
                params[i-1] = buf;
                params[i] = 0;
                exec(argv[1], params); // 执行exec, 新的地址映像将覆盖这个子进程
                fprintf(2, "xargs: %s failed", argv[1]); // 如果执行成功, 这条指令不会执行
            }
            wait(0); // 等待子进程完成, 再进入下一循环, 读取后续参数, 执行
            p = buf;
        }
        else {
            *p++ = c;
        }

    }
    exit(0);
}
```

