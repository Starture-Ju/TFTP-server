//
// Created by Starture on 9/30/25.
//
#include <iostream>
#include <socket.h>
#include <cstring>
#include <sys/time.h>
#include <arpa/inet.h>

#define MAX_TIMEOUT_TIMES 10
int Timeout_times;//超时重传次数
long long transferBytes;

server::server(const std::string& port) {
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;


    //将字符串转换成int类型
    char *endptr = nullptr;
    address.sin_port = htons(std::strtol(port.c_str(), &endptr, 10));
    if (*endptr != 0) {
        serverLog(error, "Invalid port number");
        std::exit(PORT_ERROR);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        serverLog(error, "Failed to create socket");
        std::exit(SOCKET_ERROR);
    }

    if (bind(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
        perror("bind");
        serverLog(error, "Failed to bind socket");
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
        serverLog(error, "Failed to get server ip");
        std::exit(IP_FORMAT_ERROR);
    }
    auto ret =  std::string(buffer);
    return ret;
}

void server::serverLog(logLevel logLevel, const std::string& msg) {
    std::string level = logWrite::enumToString(logLevel);
    std::string time = logWrite::getLocalTime();
    std::ofstream logFile;
    logFile.open(LOGFILE_PATH, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
        return;
    }
    std::string logMessage = time + "-    LogType: " + level + ", Message: " + msg;
    logFile << logMessage << std::endl;
}

clientLink::clientLink(const server &serv): myServer(std::to_string(0)){
    int sock = serv.getSock(); //获取服务器套接字
    //阻塞性地进行套接字监听
    long recvBytes = recvfrom(sock, recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);

    if (recvBytes == BUFFERSIZE) {
        error("Buffer overflow: Data should be less than 2048 bytes.\n", NotDefined); //数据包太大导致缓冲区溢出
        logGenerate(logLevel::error, "Buffer overflow: Data should be less than 2048 bytes.");
        while (recvBytes != 0) {
            recvBytes = recvfrom(sock, recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address),
                                 &clientAddrLen);
        } //清空套接字
        std::exit(BUFFEROVERFLOW);
    }

    if (recvBytes < 0) {
        perror("recvfrom");
        error("recvfrom error", NotDefined);
        logGenerate(logLevel::error, "recvfrom error");
        std::exit(RECV_ERROR);
    } //外层使用select轮询
    transferBytes += recvBytes;


    this->clientLink::logConstructor(LOGFILE_PATH);//初始化日志信息

    //数据包解析
    reqType = static_cast<requestType>(ntohs(*reinterpret_cast<unsigned short *>(recvBuffer))); //解析前两个字节
    const char *filePointer = recvBuffer + 2;
    fileName = std::string(filePointer);
    if (fileName.empty()) {
        error("File name is empty.\n", NotDefined);
        logGenerate(logLevel::error, "File name is empty.");
        std::exit(DATA_TYPEERROR);
    }
    const char *transModPointer = filePointer + fileName.length() + 1;
    transMod = std::string(transModPointer);
    if (transMod.empty()) {
        error("transmod is missing.\n", NotDefined);
        logGenerate(logLevel::error, "transmod is missing.");
        std::exit(DATA_TYPEERROR);
    }
    if (transMod != "netascii" && transMod != "octet") {
        error("Invalid transmod.\n", NotDefined);
        logGenerate(logLevel::error, "Invalid transmod.");
        std::exit(DATA_TYPEERROR);
    };

    //根据operation code和transfer mode分类
    if (reqType == RRQ) {
        if (transMod == "netascii") {
            fileHandle.open(fileName, std::ios::in);
        } else if (transMod == "octet") {
            fileHandle.open(fileName, std::ios::in | std::ios::binary);
        } else {
            error("Invalid transmod.\n", NotDefined);
            logGenerate(logLevel::error, "Invalid transmod.");
            std::exit(DATA_TYPEERROR);
        }
        if (!fileHandle.is_open()) {
            error("There was no file: " + fileName, FileNotFound);
            logGenerate(logLevel::error, "There was no file: " + fileName);
            std::exit(FILE_NOT_FOUND);
        }
    } else if (reqType == WRQ) {

        fileHandle.open(fileName, std::ios::in);
        if (fileHandle.is_open()) {
            error("The file was already exists: " + fileName, FileExists);
            logGenerate(logLevel::error, "The file was already exists: " + fileName);
            std::exit(FileExists);
        }//检查文件是否存在
        fileHandle.close();

        if (transMod == "netascii") {
            fileHandle.open(fileName, std::ios::out | std::ios::app);
        } else if (transMod == "octet") {
            fileHandle.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
        } else {
            error("Invalid reqType.\n", NotDefined);
            logGenerate(logLevel::error, "Invalid reqType.");
            std::exit(DATA_TYPEERROR);
        }
        seq--; //ACK从0开始
        acceptOk();
    } else {
        //未被定义的行为，不予理睬
        std::exit(DATA_TYPEERROR);
    }

    //构建成功，配置服务端socket
    timeval tv{};
    tv.tv_sec = ESTIME_INIT_S;
    tv.tv_usec = ESTIME_INIT_MS;
    if (setsockopt(myServer.getSock(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        logGenerate(logLevel::error, "setsockopt SO_RCVTIMEO failed.");
        std::exit(SOCKET_ERROR);
    } //设置超时时间

}

clientLink::~clientLink() {
    logGenerate(info, "A client has been linked:" + getClientIpByStr() + ":" + std::to_string(getPort()));
    std::cout << "A client has been linked:" + getClientIpByStr() + ":" + std::to_string(getPort()) << std::endl;
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
    long recvBytes = recvfrom(myServer.getSock(), recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);
    if (recvBytes == BUFFERSIZE) {
        error("Buffer overflow: Data should be less than 2048 bytes.\n", NotDefined);
        logGenerate(logLevel::error, "Buffer overflow.");
        while (recvBytes != 0) {
            recvBytes = recvfrom(myServer.getSock(), recvBuffer, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), &clientAddrLen);
        }//清空套接字
        std::system(("rm -f" + fileName).c_str());//删除该文件
        std::exit(BUFFEROVERFLOW);
    }

    if (recvBytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            logGenerate(warning, "Connection is timed out");
            Timeout_times++;
            if (Timeout_times >= MAX_TIMEOUT_TIMES) {
                error("Connection is Timeout.", NotDefined);
                logGenerate(logLevel::error, "Connection is Timeout.");
                std::system(("rm -f" + fileName).c_str());//删除该文件
                std::exit(TIMEOUT);
            }
            acceptOk();
            return TIMEOUT;
        }//超时重传
        perror("recvfrom");
        error("recvfrom error", NotDefined);
        logGenerate(logLevel::error, "recvfrom error.");
        std::system(("rm -f" + fileName).c_str());//删除该文件
        std::exit(RECV_ERROR);
    }
    transferBytes += recvBytes;

    Timeout_times = 0;


    const unsigned short operation = static_cast<unsigned char>(ntohs(*reinterpret_cast<unsigned short *>(recvBuffer)));
    const unsigned short blockNumber = ntohs(*reinterpret_cast<const unsigned short *>(recvBuffer + 2));
    //提取数据头
    if (operation == ERROR) errorHandle();
    if (operation != DATA) {
        acceptOk();
        return TO_BE_RESENT;//可能是第一个请求包被重传
    }
    seq++;//接受完ack后序号自增
    if (blockNumber != seq + 1) {
        acceptOk();
        return TO_BE_RESENT;//乱序重传ack
    }
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
        logGenerate(warning, "sendto() error.");
        perror("sendto");
        return SEND_ERROR;//重传
    }
    transferBytes += sendBytes;
    int ack_flag = false;

    while (ack_flag == false) {//接受ack
        long recvBytes = recvfrom(myServer.getSock(), recvBuffer, BUFFERSIZE, 0, nullptr, nullptr);
        if (recvBytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                logGenerate(warning, "Connection is Timeout.");
                Timeout_times++;
                if (Timeout_times >= MAX_TIMEOUT_TIMES) {
                    error("Connection is Timeout.", NotDefined);
                    logGenerate(logLevel::error, "Connection is Timeout.");
                    std::exit(TIMEOUT);
                }
                continue;
            }//超时重传
            perror("recvfrom");
            error("recvfrom error", NotDefined);
            std::exit(RECV_ERROR);//错误处理
        }
        Timeout_times = 0;
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
            return SEND_ERROR;//重传
        }
        seq++;
        ack_flag = true;
    }//数据包ack接受完毕
    return 0;
}

void clientLink::acceptOk() {
    char ack[BUFFERSIZE] = {};
    *reinterpret_cast<unsigned short *>(ack) = htons(ACK);
    *reinterpret_cast<unsigned short *>(ack + 2) = htons(seq + 1);
    //封装ack
    transferBytes += sendto(myServer.getSock(), ack, BUFFERSIZE, 0, reinterpret_cast<sockaddr *>(&address), clientAddrLen);
}

void clientLink::error(const std::string& errorMsg, errorType errorCode) {
    char errorMessage[BUFFERSIZE] = {};
    std::string message;
    switch(errorCode) {
        case FileNotFound:
            message = "File not found.";
            break;
        case AccessViolation:
            message = "Access violation.";
            break;
        case DiskFull:
            message = "Disk full or allocation exceeded";
            break;
        case IllegalOperation:
            message = "Illegal TFTP operation.";
            break;
        case UnknownId:
            message = "Unknown transfer ID.";
            break;
        case FileExists:
            message = "File already exists.";
            break;
        case NoSuchUser:
            message = "No such user.";
            break;
        default:
            message = "not defined, see error message";
    }
    *reinterpret_cast<unsigned short *>(errorMessage) = htons(ERROR);
    *reinterpret_cast<unsigned short *>(errorMessage + 2) = htons(errorCode);
    message += errorMsg;
    std::memcpy(errorMessage + 4, message.c_str(), message.length() + 1);
    //封装error包
    transferBytes += sendto(myServer.getSock(), errorMessage, message.length() + 5, 0, reinterpret_cast<sockaddr *>(&address), clientAddrLen);
}

int clientLink::dataProc() {
    if (reqType == RRQ) {
        char buf[BUFFERSIZE] = {};
        int endFlag = false;
        while (!endFlag) {
            std::memset(buf, 0, BUFFERSIZE);
            fileHandle.read(buf, MAX_DATA_SIZE);
            if (fileHandle.gcount() < MAX_DATA_SIZE) endFlag = true;
            std::string data(buf);
            while (dataPack(data) == SEND_ERROR);//错误重传
        }
        logGenerate(info, "Send " + fileName + " to the client: " + getClientMessage());
        std::cout << "Send " + fileName + " to the client: " + getClientMessage() << std::endl;
    } else if (reqType == WRQ) {
        while (true) {
            int recvBytes = dataRecv();
            while (recvBytes == TIMEOUT || recvBytes == TO_BE_RESENT) recvBytes = dataRecv();
            if (recvBytes < 512) break;
        }
        logGenerate(info, "Receive " + fileName + " from the client: " + getClientMessage());
        std::cout << "Receive " + fileName + " from the client: " + getClientMessage() << std::endl;
    }
    return 0;
}


void clientLink::errorHandle() const {
    const std::string errorMessage = recvBuffer + 4;
    std::cerr << errorMessage << std::endl;
    std::exit (CLIENT_ERROR);
}




//overwrite父类虚函数
void clientLink::logConstructor(std::string path) {
    setLogFilePath(path);
    std::ofstream constructorLog(path, std::ofstream::app);
    if (!constructorLog.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
    }
    setlog(constructorLog);
    std::string clientMessage = getClientIpByStr() + ":" + std::to_string(getPort());
    setClientMessage(clientMessage);
}