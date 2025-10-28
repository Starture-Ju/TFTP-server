//
// Created by Starture on 9/30/25.
//

#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <fstream>

#define TIMEOUT -5
#define SOCKET_ERROR 1
#define BIND_ERROR 2
#define ACCEPT_ERROR 4
#define IP_FORMAT_ERROR 5
#define PORT_ERROR 6
#define SELECT_ERROR 7
#define BUFFERSIZE 2048
#define MAX_DATA_SIZE 508

#define DATA_TYPEERROR 8
#define BUFFEROVERFLOW 9
#define RECV_ERROR 10
#define SEND_ERROR 11


#define ESTIME_INIT_S 0
#define ESTIME_INIT_MS 100 //100ms

//type
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5


class server {
    int sock = 0;
    unsigned short port = 0;
    sockaddr_in address{};
public:
    explicit server(const std::string& port);//构造函数
    server(server &&other) = default;//复制函数

    ~server() = default;
    server &operator = (server &&other) = default;


    [[nodiscard]] int getSock() const;
    [[nodiscard]] unsigned short getPort() const;
    [[nodiscard]] unsigned int getServerIp() const;
    [[nodiscard]] std::string getServerIpByStr() const;

};

class clientLink {
    int sock = 0;//新服务器套接字
    sockaddr_in serverAddress{};
    socklen_t serverAddressLen = 0;

    socklen_t clientAddrLen = 0;
    sockaddr_in address{};//客户端地址结构
    int seq = 0;//当前要发送的包序号

    unsigned short reqType = 0;//连接请求的类型（RRQ/WRQ）
    std::string fileName;
    std::string transMod;
    char buffer[BUFFERSIZE] = {};

    std::fstream fileHandle;
public:
    explicit clientLink(const server &serv);
    clientLink(clientLink &&other)  noexcept = default;
    ~clientLink();// try: 尝试在父进程对象销毁时显示连接信息
    clientLink &operator = (clientLink &&other) = default;



    [[nodiscard]] unsigned short getPort() const;
    [[nodiscard]] unsigned int getClientIp() const;
    [[nodiscard]] std::string getClientIpByStr() const;
    [[nodiscard]] unsigned short getReqType() const;
    [[nodiscard]] std::string getTransMod() const;

    int dataPack(std::string &data);//发送数据包并接受ACK
    int dataRecv();//接受文件数据包并发送ACK
    void acceptOk();
    void error(const std::string& message, unsigned short errorCode);//发送error包


    int dataProc();
};//将所有的io处理封装为一个链接类

#endif //SOCKET_H
