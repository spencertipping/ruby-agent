#pragma once

#include <vterm.h>
#include <string>
#include <unistd.h>
#include <pty.h>
#include <vector>

class Tab {
public:
    Tab(int width, int height);
    ~Tab();

    bool start_shell();
    void process_input(const char* buffer, size_t len);
    void update_size(int width, int height);
    
    // Returns true if the process is still running
    bool is_running() const;
    
    // Getters for integration
    int get_pty_fd() const { return pty_fd; }
    VTerm* get_vterm() const { return vt; }
    VTermScreen* get_screen() const { return vterm_obtain_screen(vt); }
    pid_t get_pid() const { return pid; }

private:
    int width;
    int height;
    VTerm* vt;
    VTermScreen* vt_screen;
    int pty_fd;
    pid_t pid;
    
    void cleanup();
};
