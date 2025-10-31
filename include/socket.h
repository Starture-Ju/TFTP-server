//
// Created by Starture on 9/30/25.
//

#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <fstream>
#include <logWrite.h>

#define TIMEOUT (-5)
#define TO_BE_RESENT (-4)
#define SOCKET_ERROR 1
#define BIND_ERROR 2
#define ACCEPT_ERROR 4
#define IP_FORMAT_ERROR 5
#define PORT_ERROR 6
#define SELECT_ERROR 7
#define BUFFERSIZE 2048
#define MAX_DATA_SIZE 512

#define DATA_TYPEERROR 8
#define BUFFEROVERFLOW 9
#define RECV_ERROR 10
#define SEND_ERROR 11

#define CLIENT_ERROR 13
#define FILE_NOT_FOUND 14


#define ESTIME_INIT_S 0
#define ESTIME_INIT_MS 100 //100ms




#define LOGFILE_PATH "./logfile.txt"

enum errorType {
    NotDefined,
    FileNotFound,
    AccessViolation,
    DiskFull,
    IllegalOperation,
    UnknownId,
    FileExists,
    NoSuchUser
};




enum requestType {
    RRQ = 1,
    WRQ,
    DATA,
    ACK,
    ERROR
};

class server {
    int sock = 0;
    unsigned short port = 0;
    sockaddr_in address{};
public:
    explicit server(const std::string& port);
    server(server &&other) = default;

    ~server() = default;
    server &operator = (server &&other) = default;


    [[nodiscard]] int getSock() const;
    [[nodiscard]] unsigned short getPort() const;
    [[nodiscard]] unsigned int getServerIp() const;
    [[nodiscard]] std::string getServerIpByStr() const;

    static void serverLog(logLevel level, const std::string& msg);

};//主进程服务器

class clientLink : logWrite{
    server myServer;//与客户端交互的端口
    sockaddr_in address{};//客户端地址结构
    socklen_t clientAddrLen = sizeof(address);
    int seq = 0;//当前要发送的包序号

    requestType reqType;//连接请求的类型（RRQ/WRQ）
    std::string fileName;
    std::string transMod;

    char sendBuffer[BUFFERSIZE] = {};
    char recvBuffer[BUFFERSIZE] = {};

    std::fstream fileHandle;
    void logConstructor(std::string path) override;
public:
    explicit clientLink(const server &serv);
    clientLink(clientLink &&other)  noexcept = delete;
    ~clientLink() override;// 在父进程对象销毁时显示连接信息
    clientLink &operator = (clientLink &&other) noexcept = delete;



    [[nodiscard]] unsigned short getPort() const;
    [[nodiscard]] unsigned int getClientIp() const;
    [[nodiscard]] std::string getClientIpByStr() const;
    [[nodiscard]] unsigned short getReqType() const;
    [[nodiscard]] std::string getTransMod() const;

    int dataPack(const std::string &data);//发送数据包并接受ACK
    int dataRecv();//接受文件数据包并发送ACK
    void acceptOk();//发送ACK
    void error(const std::string& errorMsg, errorType errorCode);//发送error包
    void errorHandle() const;//处理接受到的error包

    int dataProc();
};//将所有的io处理封装为一个链接类

#endif //SOCKET_H
