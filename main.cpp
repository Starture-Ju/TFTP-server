#include <iostream>
#include <process.h>
#include <socket.h>
#define ARG_ERROR 350
#define PLATFORM_ERROR 234
#define PORT 69

extern int now_process_count;

int main(int argc, [[maybe_unused]]char *argv[]) {
    #ifndef __linux__
        std::cerr << "Platform error: This TFTP server only works on Linux!" << std::endl;
        return PLATFORM_ERROR;
    #endif
    //只能在linux平台下运行

    if (argc != 1) return ARG_ERROR;//不带参数使用

    const std::string port = std::to_string(PORT);
    server serverTFTP(port);
    process::listenForChildProcess();
    std::cout << "TFTP server is running on " + serverTFTP.getServerIpByStr() + ":" + port << std::endl;

    bool endFlag = false;

     fd_set rset;                //每次传给 select 的读位图
     int maxfd = serverTFTP.getSock();
     timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    //设置超时时间1s

     /*-------------------------事件循环 --------------------- */
     while (!endFlag) {
         FD_ZERO(&rset);         //清空位图
         FD_SET(serverTFTP.getSock(), &rset);   //把 UDP 套接字塞进去

         int nready = select(maxfd + 1, &rset, nullptr, nullptr, &timeout);
         if (nready < 0) {
             perror("select");
             continue;
         }

         if (FD_ISSET(serverTFTP.getSock(), &rset)) {
             process newProc;//创建新进程


             if (newProc.getPid() == 0) { //子进程
                 clientLink client(serverTFTP); //创建专有链接
                 client.dataProc();//处理数据
             } else { //父进程
                 //继续运行即可
             }

         }
         while (std::cin.peek() != EOF) { //当输入exit\n时终止程序
             std::string input;
             std::getline(std::cin, input);
             if (input == "exit") {
                 endFlag = true;
                 break;
             }
         }
     }
    // 等待sigaction()销毁所有子进程
    while (now_process_count >= 0) {
        now_process_count--;
        now_process_count++;//为了让IDE不犯病的举动，没有其他意义
        sleep(10);
    }

    return 0;
}
