#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <vterm.h>
#include <vector>
#include <memory>
#include <poll.h>
#include <unistd.h>
#include <clocale>
#include "Tab.hpp"

// Global state
std::vector<std::unique_ptr<Tab>> tabs;
int current_tab_index = 0;
bool running = true;
bool command_mode = false;
int prev_rows = 0;
int prev_cols = 0;

void to_utf8(uint32_t cp, char* out) {
    if (cp < 0x80) {
        out[0] = cp;
        out[1] = 0;
    } else if (cp < 0x800) {
        out[0] = 0xC0 | (cp >> 6);
        out[1] = 0x80 | (cp & 0x3F);
        out[2] = 0;
    } else if (cp < 0x10000) {
        out[0] = 0xE0 | (cp >> 12);
        out[1] = 0x80 | ((cp >> 6) & 0x3F);
        out[2] = 0x80 | (cp & 0x3F);
        out[3] = 0;
    } else {
        out[0] = 0xF0 | (cp >> 18);
        out[1] = 0x80 | ((cp >> 12) & 0x3F);
        out[2] = 0x80 | ((cp >> 6) & 0x3F);
        out[3] = 0x80 | (cp & 0x3F);
        out[4] = 0;
    }
}

void draw_status_bar(int width, int height) {
    move(height - 1, 0);
    attron(A_REVERSE);
    for (int i = 0; i < width; ++i) {
        addch(' ');
    }
    move(height - 1, 0);
    printw("[mytmux] ");
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (i == (size_t)current_tab_index) {
            printw("*%lu ", i);
        } else {
            printw(" %lu ", i);
        }
    }
    if (command_mode) {
        printw(" (C-a)");
    }
    attroff(A_REVERSE);
}

void render_tab(int width, int height) {
    if (tabs.empty()) return;

    Tab* tab = tabs[current_tab_index].get();
    VTermScreen* vts = tab->get_screen();
    
    for (int row = 0; row < height - 1; ++row) {
        for (int col = 0; col < width; ++col) {
            VTermScreenCell cell;
            vterm_screen_get_cell(vts, {row, col}, &cell);
            
            char buf[5] = {0};
            if (cell.chars[0]) {
               to_utf8(cell.chars[0], buf);
            } else {
               buf[0] = ' ';
            }
            
            mvaddstr(row, col, buf);
            
            if (cell.width > 1) {
                col += (cell.width - 1);
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, ""); // Enable UTF-8 for ncurses
    
    // Ncurses setup
    initscr();
    raw();
    noecho();
    // keypad(stdscr, TRUE); 
    timeout(10); 
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    prev_rows = rows;
    prev_cols = cols;

    // Initial tab
    tabs.push_back(std::make_unique<Tab>(cols, rows - 1));
    tabs.back()->start_shell();

    while (running) {
        getmaxyx(stdscr, rows, cols);
        
        bool resized = (rows != prev_rows || cols != prev_cols);
        prev_rows = rows;
        prev_cols = cols;
        
        if (tabs.size() > 0) {
            if (resized) {
                // Resize all tabs or just current? 
                // Tmux resizes all usually, or viewports. 
                // Simplest: resize all
                for (auto& t : tabs) {
                     t->update_size(cols, rows - 1);
                }
            }
        }

        // Render
        erase();
        render_tab(cols, rows);
        draw_status_bar(cols, rows);
        refresh();

        // Check for dead tabs
        for (int i = tabs.size() - 1; i >= 0; --i) {
            if (!tabs[i]->is_running()) {
                tabs.erase(tabs.begin() + i);
                if (current_tab_index >= i && current_tab_index > 0) {
                     current_tab_index--;
                }
            }
        }
        
        if (tabs.empty()) {
            running = false;
        } else {
            if (current_tab_index >= (int)tabs.size()) {
                current_tab_index = tabs.size() - 1;
            }
        }

        // Input handling
        int ch = getch();
        if (ch != ERR) {
             if (command_mode) {
                 if (ch == 'c') {
                     tabs.push_back(std::make_unique<Tab>(cols, rows - 1));
                     tabs.back()->start_shell();
                     current_tab_index = tabs.size() - 1;
                 } else if (ch == 'n') {
                     if (!tabs.empty())
                         current_tab_index = (current_tab_index + 1) % tabs.size();
                 } else if (ch == 1) { 
                     if (!tabs.empty()) {
                         char c = 1;
                         write(tabs[current_tab_index]->get_pty_fd(), &c, 1);
                     }
                 }
                 command_mode = false;
             } else {
                 if (ch == 1) { // C-a
                     command_mode = true;
                 } else {
                     if (!tabs.empty()) {
                         char c = (char)ch;
                         write(tabs[current_tab_index]->get_pty_fd(), &c, 1);
                     }
                 }
             }
        }

        // Poll PTYs
        std::vector<struct pollfd> fds;
        for (const auto& t : tabs) {
            fds.push_back({t->get_pty_fd(), POLLIN, 0});
        }
        
        int ret = poll(fds.data(), fds.size(), 0); 
        
        if (ret > 0) {
            for (size_t i = 0; i < tabs.size(); ++i) {
                if (fds[i].revents & POLLIN) {
                    char buf[4096];
                    ssize_t bytes = read(fds[i].fd, buf, sizeof(buf));
                    if (bytes > 0) {
                        tabs[i]->process_input(buf, bytes);
                    }
                }
            }
        }
    }

    endwin();
    return 0;
}
