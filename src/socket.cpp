//
// Created by Starture on 9/30/25.
//
#include <iostream>
#include <socket.h>
#include <cstring>
#include <sys/time.h>
//#include <sys/select.h>
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
    timeval tv{};
    tv.tv_sec  = ESTIME_INIT_S;
    tv.tv_usec = ESTIME_INIT_MS;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        exit(SOCKET_ERROR);
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

clientLink::clientLink(const server &serv) {
    sock = serv.getSock();//获取服务器套接字
    //阻塞性地进行套接字监听
    long recvBytes = recvfrom(sock, buffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);

    if (recvBytes == BUFFERSIZE) {
        error("Buffer overflow: Data should be less than 2048 bytes.\n", BUFFEROVERFLOW);//数据包太大导致缓冲区溢出
        while (recvBytes != 0) {
            recvBytes = recvfrom(sock, buffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);
        }//清空套接字
        exit(BUFFEROVERFLOW);
    }

    if (recvBytes < 0) {
        perror("recvfrom");
        error("recvfrom error", RECV_ERROR);
        exit(RECV_ERROR);
    }//外层使用select轮询

    //数据包解析
    reqType = static_cast<unsigned char>(buffer[1]);
    char *filePointer = buffer + 2;
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
    } else if (reqType == WRQ) {
        if (transMod == "netascii") {
            fileHandle.open(fileName, std::ios::out | std::ios::app);
        } else if (transMod == "octet") {
            fileHandle.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
        } else {
            error("Invalid reqType.\n", DATA_TYPEERROR);
            exit(DATA_TYPEERROR);
        }
    }



    //构建成功，构建服务端socket
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;


    //将字符串转换成int类型
    char *endptr = nullptr;
    serverAddress.sin_port = htons(0);
    if (*endptr != 0) {
        std::cerr << "Invalid port number" << std::endl;
        std::exit(PORT_ERROR);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        std::exit(SOCKET_ERROR);
    }
    timeval tv{};
    tv.tv_sec  = ESTIME_INIT_S;
    tv.tv_usec = ESTIME_INIT_MS;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        exit(SOCKET_ERROR);
    }

    if (bind(sock, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == -1) {
        perror("bind");
        std::exit(BIND_ERROR);
    }
}

clientLink::~clientLink() {
    std::cout << "A client has been linked:" << getClientIpByStr() << ":" << std::to_string(getPort()) << std::endl;
}

unsigned short clientLink::getPort() const { return ntohs(address.sin_port); }
unsigned int clientLink::getClientIp() const { return ntohl(address.sin_addr.s_addr); }
std::string clientLink::getClientIpByStr() const {
    char buffer[INET6_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &address.sin_addr, buffer, INET6_ADDRSTRLEN);
    auto ret = std::string(buffer);
    return ret;
}
unsigned short clientLink::getReqType() const { return reqType; }
std::string clientLink::getTransMod() const { return transMod; }




int clientLink::dataRecv() {
    long recvBytes = recvfrom(sock, buffer, BUFFERSIZE, 0, nullptr, nullptr);
    if (recvBytes == BUFFERSIZE) {
        error("Buffer overflow: Data should be less than 2048 bytes.\n", BUFFEROVERFLOW);
        while (recvBytes != 0) {
            recvBytes = recvfrom(sock, buffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);
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


    unsigned short operation = static_cast<unsigned char>(buffer[1]);
    if (operation != DATA) {
        error("Invalid operation.\n", DATA_TYPEERROR);
        exit(DATA_TYPEERROR);
    }
    auto blockNumberPointer = reinterpret_cast<const unsigned short *>(buffer + 2);
    const unsigned short blockNumber = ntohs(*blockNumberPointer);
    if (blockNumber != seq + 1) {
        error("Block number is Out Of Order.\nThe next Data should be " + std::to_string(seq + 1) + ".\n", DATA_TYPEERROR);
        exit(DATA_TYPEERROR);
    }
    seq++;//处理完后序号自增
    fileHandle.write(buffer + 4, recvBytes - 4);
    acceptOk();
    return static_cast<int>(recvBytes);
}//接受数据包并发送ack

int clientLink::dataPack(std::string &data) {
    buffer[0] = 0;
    buffer[1] = 3;
    auto blockNumberPointer = reinterpret_cast<unsigned short *>(buffer + 2);
    *blockNumberPointer = htons(seq + 1);
    char *dataPointer = buffer + 4;
    std::memcpy(dataPointer, data.c_str(), data.length() + 1);
    int sendBytes = 4 + data.length() + 1;
    long realSent = sendto(sock, buffer, sendBytes, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (sendBytes != realSent) {
        std::cerr << "sendto() error: " << "the sent bytes is error" << std::endl;
        perror("sendto");
        return SEND_ERROR;//重传
    }
    int ack_flag = false;

    while (ack_flag == false) {
        long recvBytes = recvfrom(sock, buffer, BUFFERSIZE, 0, nullptr, nullptr);
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
            error("ack is error.", RECV_ERROR);
            exit(RECV_ERROR);
        }
        unsigned short operation = static_cast<unsigned char>(buffer[1]);
        unsigned short blockNumber = htons(*blockNumberPointer);
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
    auto operationPointer = reinterpret_cast<unsigned short *>(buffer);
    auto blockNumberPointer = reinterpret_cast<unsigned short *>(buffer + 2);
    *operationPointer = htons(ACK);
    *blockNumberPointer = htons(seq + 1);
    //封装ack
    sendto(sock, ack, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), 4);
}

void clientLink::error(const std::string& message, unsigned short errorCode) {
    char errorMessage[BUFFERSIZE] = {};
    auto operationPointer = reinterpret_cast<unsigned short *>(errorMessage);
    auto errorCodePointer = reinterpret_cast<unsigned short *>(errorMessage + 2);
    auto messagePointer = errorMessage + 4;
    std::memcpy(messagePointer, message.c_str(), message.length() + 1);
    *operationPointer = htons(ERROR);
    *errorCodePointer = htons(errorCode);
    sendto(sock, errorMessage, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), 4);
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
