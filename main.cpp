#include <chrono>
#include <iostream>
#include <process.h>
#include <socket.h>
#define ARG_ERROR 350
#define PLATFORM_ERROR 234
#define PORT 69


extern int now_process_count;
extern long long transferBytes;



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

     fd_set rset;                //每次传给 select 的读位图
     int maxfd = serverTFTP.getSock();


     /*-------------------------事件循环 --------------------- */
     while (true) {
         FD_ZERO(&rset);         //清空位图
         FD_SET(serverTFTP.getSock(), &rset);   //把 UDP 套接字塞进去
         timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
         //设置select()超时时间1s
         int nready = select(maxfd + 1, &rset, nullptr, nullptr, &timeout);
         if (nready < 0) {
             perror("select");
             continue;
         }

         if (FD_ISSET(serverTFTP.getSock(), &rset)) {


             process newProc;//创建新进程


             if (newProc.getPid() == 0) { //子进程
                 auto startTime = std::chrono::high_resolution_clock::now();
                 clientLink client(serverTFTP); //创建专有链接
                 client.dataProc();//处理数据
                 auto endTime = std::chrono::high_resolution_clock::now();
                 auto duration  = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                 const long long duration_ms = duration.count();
                 if (duration_ms == 0) std::cout << "Tranfer " << transferBytes << " bytes within 1ms"<< std::endl;//传输时间过短
                 else if (transferBytes < duration_ms) std::cout << "Throughput: " << transferBytes * 1000 / duration_ms << " Bytes/s " << "during the connection to " + client.getClientIpByStr() + ":" + std::to_string(client.getPort()) << std::endl;
                 else std::cout << "Throughput: " << transferBytes / duration_ms << " kB/s " << "during the connection to " + client.getClientIpByStr() + ":" + std::to_string(client.getPort()) << std::endl;
                 std::exit(0);
             } else { //父进程
                 sleep(1);//稍微等等子进程创建
                 //继续运行即可
             }

         }
         if (kbhit()) { //当输入exit\n时终止程序
             std::string input;
             std::getline(std::cin, input);
             if (input == "exit") {
                 break;
             }
         }
     }
    // 等待sigaction()销毁所有子进程
    while (now_process_count > 0) {
        std::cerr << "Waiting for " << now_process_count << " child processes to finish..." << std::endl;
        sleep(10); // 等待 SIGCHLD 信号处理函数清理子进程
    }

    return 0;
}
