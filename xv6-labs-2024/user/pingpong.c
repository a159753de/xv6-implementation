#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    // 此程式不需要任何參數，偵測到參數就退出
    if (argc > 2)
    {
        fprintf(2, "Pingpong needs no arguments\n");
        exit(1);
    }
    char byte = 'a';
    int p1[2], p2[2]; // 用2條pipe傳訊息
    pipe(p1);         // 將 fd 0 和 1 存入 p1[]中，p1[0] 為讀取端，p1[1] 為寫入端
    pipe(p2);

    if (fork() == 0)
    {
        // 子 process：用 p1 接收父 process 的 data，p2 回覆父 process
        if (read(p1[0], &byte, 1) > 0)
        {
            printf("%d: received ping\n", getpid());
            write(p2[1], &byte, 1);
        }
        exit(0);
    }
    else
    {
        // 父 process：用 p1 傳 data 給子 process，p2 接收子 process 的回覆
        write(p1[1], &byte, 1);
        wait(0);

        if (read(p2[0], &byte, 1) > 0)
        {
            printf("%d: received pong\n", getpid());
        }
    }
    exit(0);
}