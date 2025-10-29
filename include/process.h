//
// Created by Starture on 9/30/25.
//

#ifndef PROCESS_H
#define PROCESS_H
#include <unistd.h>
#define PROCESS_MAXCOUNT 128

class process {
    pid_t pid = 0;

    static void childProcDestroyed(int sig);

public:
    explicit process();
    ~process() = default;
    [[nodiscard]] int getPid() const;

    static void listenForChildProcess();


};

#endif //PROCESS_H
