#include <iostream>
#include <process.h>
#include <socket.h>
#define ARG_ERROR 10
#define PORT 69
#define PLATFORM_ERROR 14


int main(int argc, char *argv[]) {
    #ifndef __linux__
        std::cerr << "Platform error: This TFTP server only works on Linux!" << std::endl;
        return PLATFORM_ERROR;
    #endif
    //只能在linux平台下运行

    const std::string port = std::to_string(PORT);
    server serverTFTP(port);
    process::listenForChildProcess();
    std::cout << "TFTP server is running on " + serverTFTP.getServerIpByStr() + ":" + port << std::endl;

    while (true) {
        clientLink client(serverTFTP);
        process newProc;
        if (newProc.getPid() == 0) { //子进程
            client.dataProc();//处理数据
        } else { //父进程
            //继续运行即可
        }

    }

    //一下为暂时的废案
    // fd_set rset;                //每次传给 select 的读位图
    // int maxfd = serverTFTP.getSock();
    //
    // /*-------------------------事件循环 --------------------- */
    // while (true)
    // {
    //     FD_ZERO(&rset);         //清空位图
    //     FD_SET(serverTFTP.getSock(), &rset);   //把 UDP 套接字塞进去
    //
    //     //永久阻塞，直到有读事件
    //     int nready = select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
    //     if (nready < 0) { perror("select"); continue; }
    //
    //     if (FD_ISSET(serverTFTP.getSock(), &rset))
    //     {
    //
    //     }
    // }

    return 0;
}