#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// 用來找尋 path 中的檔案名稱
char *fmtname(char *path)
{
    char *p;

    // 找到最後一個 '/' 後的字元
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;

    return p + 1; // 回傳最後一個 '/' 後的字元位置
}

void find(char *path, char *filename)
{
    char buf[512]; // 用來存放目錄路徑
    char *p;
    int fd;
    struct dirent de; // 共 16 bytes： inum(2 bytes) + name(14 bytes)
    struct stat st;   // i-node 的簡易資訊

    // <0 代表開啟失敗
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // 這一步中， st中就會存入 path 的資訊
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 檢查 path 的狀態是檔案還是目錄
    switch (st.type)
    {
    case T_DEVICE:

    // 是檔案，直接比對檔名
    case T_FILE:
        //* 取得 path 中的檔名 *//
        char *file_name = fmtname(path);

        // 比對檔名是否相同
        if (strcmp(file_name, filename) == 0)
        {
            printf("%s\n", path); // 如果相同，印出完整路徑
        }
        break;

    // 是目錄
    case T_DIR:
        // 檢查目錄名稱(path + '/' + DIRSIZ + '\0') 是否超過 buf 大小
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            fprintf(2, "find: path too long\n");
            break;
        }

        strcpy(buf, path);     // 將 path 複製到 buf 中
        p = buf + strlen(buf); // 更新指標
        *p++ = '/';            // 在 buf 中加入 '/'，為下一個路徑準備

        // 讀取目錄中的每個檔案或子目錄
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            // inum == 0 代表該目錄項目不存在，跳過
            // name 為 "." 或 ".." 也跳過，避免無限遞迴
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            //* 將 name 複製到 buf 中，形成完整路徑 *//
            // 但因為 de.name 只有 14 bytes，可能不會以 '\0' 結尾，所以要用 memmove 來複製，並手動加上 '\0'
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            find(buf, filename); // 遞迴呼叫 find，搜尋子目錄或檔案
        }
        break;
    }
    close(fd); // 關閉目錄檔案描述符
}

// argv[0] 是程式名稱， argv[1]： path； argv[2]：檔案名稱
int main(int argc, char *argv[])
{
    // 一定要傳入兩個參數，否則就不執行
    if (argc != 3)
    {
        fprintf(2, "Usage: find <path> <filename>\n");
        exit(1);
    }

    // 取得 path 與 filename
    char *path = argv[1];
    char *filename = argv[2];

    find(path, filename);

    exit(0);
}