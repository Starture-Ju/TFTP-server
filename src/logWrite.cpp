//
// Created by Starture on 10/31/25.
//
#include <logWrite.h>
#include <chrono>
#include <iostream>
#include <utility>



void logWrite::setClientMessage(std::string message) { clientMessage = std::move(message); }
void logWrite::setlog(std::ofstream &newLog) { log = std::move(newLog); }
void logWrite::setLogFilePath(std::string path) { logFilePath = std::move(path); }



std::ofstream &logWrite::getLog() { return log; }
std::string logWrite::getClientMessage() { return clientMessage; }


logWrite::~logWrite() {
    log.close();
}


std::string logWrite::enumToString(logLevel level) {
    switch (level) {
        case info:
            return "info";
        case debug:
            return "debug";
        case warning:
            return "warning";
        case error:
            return "error";
        default:
            return "unknown";
    }
}



std::string logWrite::getLocalTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    const tm* localTime = localtime(&timeNow);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localTime);

    return timeStr;
}


void logWrite::logGenerate(logLevel level, const std::string& message) {
    if (!log.is_open()) { return; }

    std::string LogLevel = enumToString(level);
    std::string time = getLocalTime();
    std::string logMessage = time + "-    Client: " + clientMessage + ", LogType: " + LogLevel + ", Message: " + message;
    log << logMessage << std::endl;
}

