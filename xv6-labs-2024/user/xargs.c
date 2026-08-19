#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

// xargs 格式： xargs <選項> <執行的指令和參數>
void run_cmd(char *cmd, char *args[])
{
    int pid = fork();
    if (pid < 0)
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        // 子 process
        exec(cmd, args);
        fprintf(2, "exec %s failed\n", cmd);
        exit(1);
    }
    else
    {
        // 父 process
        wait(0); // 等待子 process 結束
    }
}
// argv[0] 是程式名稱
// argv[1]：pattern， 目前只有 '-n'，若沒有 -n，則 argv[1] 是要執行的指令
// argv[2]：'-n' 的參數/ 若沒有 -n，則 argv[2] 是要執行的指令
int main(int argc, char *argv[])
{
    char *cmd_args[MAXARG]; // 儲存 xargs 要執行的指令參數
    int cmd_argc = 0;       // 計算指令總長度
    int arg_index = 1;      // 用來走訪傳入的參數
    int n = 0;              // 每次執行指令時最多帶幾個參數

    // 若只傳入1個參數，預設使用 echo
    if (argc < 2)
    {
        cmd_args[cmd_argc++] = "echo";
    }
    // 2 個參數以上
    else
    {
        // 處理 '-n'
        if (strcmp(argv[1], "-n") == 0)
        {
            // 檢查 argc 是否足夠，若有 '-n'，則至少要有四個參數(xargs, -n, n的數目, 指令+它的參數)
            if (argc < 4)
            {
                fprintf(2, "Usage: xargs -n <number> <command>\n");
                exit(1);
            }

            n = atoi(argv[2]); // 取得 -n 的參數值
            arg_index = 3;     // 待執行的指令與其參數從 argv[3] 開始
        }

        // 將指令和參數複製進 cmd_args
        for (int i = arg_index; i < argc; i++)
        {
            cmd_args[cmd_argc++] = argv[i];
        }
    }

    // 讀入 stdin 的輸入(左邊 pipe 的輸出)
    char c;                  // 用來存放 stdin 讀入的字元(byte)
    char buf[512];           // 用來存放 stdin 讀入的字串
    int buf_idx = 0;         // buf 的索引
    int start_idx = 0;       // 當前引入的起始位置
    int cur_argc = cmd_argc; // 動態追蹤當前總參數量

    while (read(0, &c, 1) > 0)
    {
        // 遇到空白，將 buf 中的字串存入 cmd_args
        if (c == ' ' || c == '\t' || c == '\n')
        {
            // 邊界處理。要判斷 buf_idx 大於0，且上一次存入的字元不是 '\0'
            if (buf_idx > 0 && buf[buf_idx - 1] != '\0')
            {
                buf[buf_idx++] = '\0';                  // 補 0
                cmd_args[cur_argc++] = &buf[start_idx]; // 將參數存入 cmd_args
                start_idx = buf_idx;                    // 下個字元的開頭
            }

            // 判斷是否滿足 n 個參數
            if (n > 0 && cur_argc - cmd_argc >= n)
            {
                cmd_args[cur_argc] = 0;         // exec() 的規定，要靠最後面的 NULL 判斷結束
                run_cmd(cmd_args[0], cmd_args); // 傳入指令和參數
                cur_argc = cmd_argc;
            }

            // 將剩下的參數都送去執行
            if (c == '\n')
            {
                // 若還有剩餘參數未執行
                if (cur_argc > cmd_argc)
                {
                    cmd_args[cur_argc] = 0;         // exec() 的規定，要靠最後面的 NULL 判斷結束
                    run_cmd(cmd_args[0], cmd_args); // 傳入指令和參數
                }

                // 重置
                start_idx = 0;
                buf_idx = 0;
                cur_argc = cmd_argc;
            }
        }
        // 讀到換行符，要執行一次指令
        else
        {
            // 將字元存入 buf
            buf[buf_idx++] = c;
        }
    }

    exit(0);
}