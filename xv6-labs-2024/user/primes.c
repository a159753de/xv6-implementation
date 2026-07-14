#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void stage(int) __attribute__((noreturn));

// 傳入左邊 pipe 的 FD 0，然後產生每個 stage 的 process
void stage(int l_pipe)
{
    int p;

    // 左邊傳入的第一個數字必定是質數
    // 若read 回傳 <= 0，表示左邊 pipe 已經沒有數字了，直接結束
    if (read(l_pipe, &p, sizeof(int)) <= 0)
    {
        close(l_pipe);
        exit(0);
    }
    printf("prime %d\n", p); // 印出質數

    // 建立與下個 stage 的 pipe
    int r_pipe[2];
    pipe(r_pipe);

    int pid = fork();
    if (pid < 0)
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        // 子 process
        close(r_pipe[1]); // 不需要 r_pipe 的寫入端
        close(l_pipe);    // 不需要 l_pipe 的讀取端

        stage(r_pipe[0]); // 遞迴呼叫下一個 stage
    }
    else
    {
        // 父 process
        close(r_pipe[0]); // 關閉與右邊 stage 的讀取端

        // 讀取左邊 pipe 的數字，篩選出質數後傳入右邊 pipe
        int num;
        while (read(l_pipe, &num, sizeof(int)) > 0)
        {
            // 無法被當前 stage 的質數整除，就送給右邊 stage
            if (num % p != 0)
            {
                write(r_pipe[1], &num, sizeof(int));
            }
        }

        close(r_pipe[1]); // 關閉與右邊 stage 的寫入端
        close(l_pipe);    // 關閉與左邊 stage 的讀取端

        wait(0); // 等待子 process 結束
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    // 建立 pipe
    int p[2];
    pipe(p);

    int pid = fork();
    if (pid < 0)
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        // 子 process
        close(p[1]); // 關閉寫入端
        // 遞迴進行質數的篩選
        stage(p[0]);
    }
    else
    {
        // 父 process
        close(p[0]); // 關閉讀取端

        // 開始將 2~280 的數字傳入子 process
        for (int i = 2; i <= 280; i++)
        {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]); // 關閉寫入端

        wait(0); // 等待子 process 結束
        exit(0);
    }
}