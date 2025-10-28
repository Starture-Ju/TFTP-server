//
// Created by Starture on 9/30/25.
//
#include <process.h>
#include <csignal>
#include <sys/wait.h>
#include <cstring>

void process::childProcDestroyed([[maybe_unused]] int sig){
    int status;
    waitpid(-1, &status, WNOHANG);
    if (!WIFEXITED(status)) perror("waitpid");
}//暂时不做更多情况的进程处理


process::process() { pid = fork(); }

int process::getPid() const { return pid; }


void process::listenForChildProcess() {
    struct sigaction act {};
    memset(&act, 0, sizeof(act));
    act.sa_handler = childProcDestroyed;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, nullptr);
}//使用sigaction自动监听子程序的返回
