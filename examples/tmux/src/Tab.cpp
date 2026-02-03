#include "Tab.hpp"
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <cstring>
#include <sys/ioctl.h>

Tab::Tab(int width, int height) : width(width), height(height), vt(nullptr), pty_fd(-1), pid(-1) {
    vt = vterm_new(height, width);
    vterm_set_utf8(vt, 1);
    vt_screen = vterm_obtain_screen(vt);
    vterm_screen_reset(vt_screen, 1);
}

Tab::~Tab() {
    cleanup();
}

void Tab::cleanup() {
    if (vt) {
        vterm_free(vt);
        vt = nullptr;
    }
    if (pty_fd >= 0) {
        close(pty_fd);
        pty_fd = -1;
    }
    if (pid > 0) {
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        pid = -1;
    }
}

bool Tab::start_shell() {
    // forkpty opens a pseudoterminal
    pid = forkpty(&pty_fd, nullptr, nullptr, nullptr);
    
    if (pid < 0) {
        perror("forkpty");
        return false;
    }
    
    if (pid == 0) {
        // Child process
        // Set environment variables if needed
        setenv("TERM", "xterm-256color", 1);
        
        // Execute shell
        const char *shell = getenv("SHELL");
        if (!shell) shell = "/bin/bash";
        
        execl(shell, shell, nullptr);
        perror("execl");
        _exit(1);
    }
    
    // Parent process
    // Nothing special needed right now
    return true;
}

void Tab::process_input(const char* buffer, size_t len) {
    vterm_input_write(vt, buffer, len);
}

void Tab::update_size(int w, int h) {
    width = w;
    height = h;
    vterm_set_size(vt, height, width);
    vterm_screen_flush_damage(vt_screen);
    // Also need to resize PTY
    struct winsize wz;
    wz.ws_row = height;
    wz.ws_col = width;
    ioctl(pty_fd, TIOCSWINSZ, &wz);
}

bool Tab::is_running() const {
    if (pid <= 0) return false;
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == 0) {
        return true; // Still running
    }
    // result == pid (exited) or -1 (error/no child)
    return false;
}
