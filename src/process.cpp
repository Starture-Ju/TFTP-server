//
// Created by Starture on 9/30/25.
//
#include <process.h>
#include <csignal>
#include <sys/wait.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <bits/ostream.tcc>

int now_process_count;

void process::childProcDestroyed([[maybe_unused]] int sig){
    int status;
    waitpid(-1, &status, WNOHANG);
    if (!WIFEXITED(status)) perror("waitpid");
    now_process_count--;
}//暂时不做更多情况的进程处理


process::process() {
    while (now_process_count >= PROCESS_MAXCOUNT) {
        std::cerr << "Process count overflowed" << std::endl;
        now_process_count--;
        now_process_count++;//防止IDE犯病的两行代码，没有别的意义
        sleep(10);//等待sigaction()处理子进程
    }
    pid = fork();
    now_process_count++;
}

int process::getPid() const { return pid; }


void process::listenForChildProcess() {
    struct sigaction act {};
    memset(&act, 0, sizeof(act));
    act.sa_handler = childProcDestroyed;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, nullptr);
}//使用sigaction自动监听子程序的返回


bool kbhit() {                 // AI给出的不阻塞地检查缓冲区方案
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    timeval tv{0, 0};          // 立即返回
    return select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv) > 0;
}
