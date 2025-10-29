//
// Created by Starture on 9/30/25.
//
#include <iostream>
#include <socket.h>
#include <cstring>
#include <sys/time.h>
#include <arpa/inet.h>

server::server(const std::string& port) {
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;


    //将字符串转换成int类型
    char *endptr = nullptr;
    address.sin_port = htons(std::strtol(port.c_str(), &endptr, 10));
    if (*endptr != 0) {
        std::cerr << "Invalid port number" << std::endl;
        std::exit(PORT_ERROR);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        std::exit(SOCKET_ERROR);
    }

    if (bind(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
        perror("bind");
        std::exit(BIND_ERROR);
    } //构建一个这来监听的UDP套接字
}//完成UDP套接字的构建与监听
int server::getSock() const { return sock; }
unsigned short server::getPort() const { return ntohs(address.sin_port); }
unsigned int server::getServerIp() const { return address.sin_addr.s_addr; }

std::string server::getServerIpByStr() const {
    char buffer[INET6_ADDRSTRLEN] = {};
    if (inet_ntop(AF_INET, &address.sin_addr, buffer, INET6_ADDRSTRLEN) == nullptr) {
        perror("inet_ntop");
        std::exit(IP_FORMAT_ERROR);
    }
    auto ret =  std::string(buffer);
    return ret;
}

clientLink::clientLink(const server &serv): myServer(std::to_string(0)) {
    int sock = serv.getSock(); //获取服务器套接字
    //阻塞性地进行套接字监听
    long recvBytes = recvfrom(sock, recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);

    if (recvBytes == BUFFERSIZE) {
        error("Buffer overflow: Data should be less than 2048 bytes.\n", BUFFEROVERFLOW); //数据包太大导致缓冲区溢出
        while (recvBytes != 0) {
            recvBytes = recvfrom(sock, recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);
        } //清空套接字
        exit(BUFFEROVERFLOW);
    }

    if (recvBytes < 0) {
        perror("recvfrom");
        error("recvfrom error", RECV_ERROR);
        exit(RECV_ERROR);
    } //外层使用select轮询

    //数据包解析
    reqType = static_cast<requestType>(ntohs(*reinterpret_cast<unsigned short *>(recvBuffer)));//解析前两个字节
    char *filePointer = recvBuffer + 2;
    fileName = std::string(filePointer);
    if (fileName.empty()) {
        error("File name is empty.\n", DATA_TYPEERROR);
        exit(DATA_TYPEERROR);
    }
    char *transModPointer = filePointer + fileName.length() + 1;
    transMod = std::string(transModPointer);
    if (transMod.empty()) {
        error("transmod is missing.\n", DATA_TYPEERROR);
        exit(DATA_TYPEERROR);
    }
    if (transMod != "netascii" && transMod != "octet") {
        error("Invalid transmod.\n", DATA_TYPEERROR);
        exit(DATA_TYPEERROR);
    };

    //根据operation code和transfer mode分类
    if (reqType == RRQ) {
        if (transMod == "netascii") {
            fileHandle.open(fileName, std::ios::in | std::ios::app);
        } else if (transMod == "octet") {
            fileHandle.open(fileName, std::ios::in | std::ios::app | std::ios::binary);
        } else {
            error("Invalid transmod.\n", DATA_TYPEERROR);
            exit(DATA_TYPEERROR);
        }
        if (!fileHandle.is_open()) {
            error("There was no file: " + fileName, FILE_NOT_FOUND);
        }
    } else if (reqType == WRQ) {
        if (transMod == "netascii") {
            fileHandle.open(fileName, std::ios::out | std::ios::app);
        } else if (transMod == "octet") {
            fileHandle.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
        } else {
            error("Invalid reqType.\n", DATA_TYPEERROR);
            exit(DATA_TYPEERROR);
        }
    } else { //未被定义的行为，不予理睬
        exit(DATA_TYPEERROR);
    }

    //构建成功，配置服务端socket
    timeval tv{};
    tv.tv_sec = ESTIME_INIT_S;
    tv.tv_usec = ESTIME_INIT_MS;
    if (setsockopt(myServer.getSock(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        exit(SOCKET_ERROR);
    } //设置超时时间
}

clientLink::~clientLink() {
    std::cout << "A client has been linked:" << getClientIpByStr() << ":" << std::to_string(getPort()) << " on the port: " << myServer.getPort() << std::endl;
}//父进程输出链接信息

unsigned short clientLink::getPort() const { return ntohs(address.sin_port); }
unsigned int clientLink::getClientIp() const { return ntohl(address.sin_addr.s_addr); }
std::string clientLink::getClientIpByStr() const {
    char buffer[INET6_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &address.sin_addr, buffer, INET6_ADDRSTRLEN);
    auto ret = std::string(buffer);
    return ret;
}//获取点分十进制的ip
unsigned short clientLink::getReqType() const { return reqType; }
std::string clientLink::getTransMod() const { return transMod; }




int clientLink::dataRecv() {
    long recvBytes = recvfrom(myServer.getSock(), recvBuffer, BUFFERSIZE, 0, nullptr, nullptr);
    if (recvBytes == BUFFERSIZE) {
        error("Buffer overflow: Data should be less than 2048 bytes.\n", BUFFEROVERFLOW);
        while (recvBytes != 0) {
            recvBytes = recvfrom(myServer.getSock(), recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);
        }//清空套接字
        exit(BUFFEROVERFLOW);
    }

    if (recvBytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cerr << "Connection is Timeout" << std::endl;
            acceptOk();
            return TIMEOUT;
        }//超时重传
        perror("recvfrom");
        error("recvfrom error", RECV_ERROR);
        exit(RECV_ERROR);
    }


    const unsigned short operation = static_cast<unsigned char>(ntohs(*reinterpret_cast<unsigned short *>(recvBuffer)));
    const unsigned short blockNumber = ntohs(*reinterpret_cast<const unsigned short *>(recvBuffer + 2));
    //提取数据头
    if (operation == ERROR) errorHandle();
    if (operation != DATA) {
        return TO_BE_RESENT;//可能是第一个请求包被重传
    }
    if (blockNumber != seq + 1) {
        return TO_BE_RESENT;//乱序重传ack
    }
    seq++;//处理完后序号自增
    fileHandle.write(recvBuffer + 4, recvBytes - 4);
    acceptOk();
    return static_cast<int>(recvBytes);
}//接受数据包并发送ack

int clientLink::dataPack(const std::string &data) {
    *reinterpret_cast<unsigned short *>(sendBuffer) = htons(DATA);
    *reinterpret_cast<unsigned short *>(sendBuffer + 2) = htons(seq + 1);
    char *dataPointer = sendBuffer + 4;
    std::memcpy(dataPointer, data.c_str(), data.length());
    int sendBytes = static_cast<int>(data.length()) + 4;
    long realSent = sendto(myServer.getSock(), sendBuffer, sendBytes, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (sendBytes != realSent) {
        std::cerr << "sendto() error: " << "the sent bytes is error" << std::endl;
        perror("sendto");
        return SEND_ERROR;//重传
    }
    int ack_flag = false;

    while (ack_flag == false) {//接受ack
        long recvBytes = recvfrom(myServer.getSock(), recvBuffer, BUFFERSIZE, 0, nullptr, nullptr);
        if (recvBytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "Connection is Timeout" << std::endl;
                acceptOk();
                continue;
            }//超时重传
            perror("recvfrom");
            error("recvfrom error", RECV_ERROR);
            exit(RECV_ERROR);//错误处理
        }
        if (recvBytes != 4) {
            std::cerr << "ack bytes is error." << std::endl;
            continue; //继续接受ack
        }
        unsigned short operation = ntohs(*reinterpret_cast<unsigned short *>(recvBuffer));
        unsigned short blockNumber = htons(*reinterpret_cast<unsigned short *>(recvBuffer + 2));
        if (operation == ERROR) errorHandle();
        if (operation != ACK) {
            std::cerr << "Invalid operation." << std::endl;
            continue;
        }
        if (blockNumber != seq + 1) {
            std::cerr << "Block number is Out Of Order." << std::endl;
            continue;
        }
        seq++;
        ack_flag = true;
    }//数据包ack接受完毕
    return 0;
}

void clientLink::acceptOk() {
    char ack[BUFFERSIZE] = {};
    *reinterpret_cast<unsigned short *>(sendBuffer) = htons(ACK);
    *reinterpret_cast<unsigned short *>(sendBuffer + 2) = htons(seq + 1);
    //封装ack
    sendto(myServer.getSock(), ack, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), 4);
}

void clientLink::error(const std::string& message, unsigned short errorCode) {
    char errorMessage[BUFFERSIZE] = {};
    *reinterpret_cast<unsigned short *>(errorMessage) = htons(ERROR);
    *reinterpret_cast<unsigned short *>(errorMessage + 2) = htons(errorCode);
    std::memcpy(errorMessage + 4, message.c_str(), message.length() + 1);
    //封装error包

    sendto(myServer.getSock(), errorMessage, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), 4);
}

int clientLink::dataProc() {
    if (reqType == RRQ) {
        char buf[BUFFERSIZE] = {};
        int endFlag = false;
        while (!endFlag) {
            std::memset(buf, 0, BUFFERSIZE);
            fileHandle.read(buf, MAX_DATA_SIZE);
            if (fileHandle.gcount() < 0) endFlag = true;
            std::string data(buf);
            dataPack(data);
        }
    } else if (reqType == WRQ) {
        while (true) {
            int recvBytes = dataRecv();
            if (recvBytes < 512) break;
        }
    }
    return 0;
}


void clientLink::errorHandle() const {
    const std::string errorMessage = recvBuffer + 4;
    std::cerr << errorMessage << std::endl;
    exit (CLIENT_ERROR);
}
