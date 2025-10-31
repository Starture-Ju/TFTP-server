//
// Created by Starture on 10/31/25.
//

#ifndef LOGWRITE_H
#define LOGWRITE_H
#include <fstream>

#define LOGPATH "./logfile.txt"



enum logLevel {
    info,
    debug,
    warning,
    error
};

class logWrite{
    std::string logFilePath = LOGPATH;
    std::ofstream log;
    std::string clientMessage;//记录ip:端口

public:
    logWrite() = default;

    virtual ~logWrite();

    virtual void logConstructor(std::string path) = 0;

    [[nodiscard]] std::ofstream& getLog();
    [[nodiscard]] std::string getClientMessage();

    void setLogFilePath(std::string path);
    void setClientMessage(std::string message);
    void setlog(std::ofstream &newLog);


    static std::string enumToString(logLevel level);
    static std::string getLocalTime();
    void logGenerate(logLevel level, const std::string& message);
};

#endif //LOGWRITE_H
