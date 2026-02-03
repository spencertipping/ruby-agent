#include <vterm.h>
#include <iostream>

int main() {
    VTerm *vt = vterm_new(25, 80);
    if (vt) {
        std::cout << "libvterm initialized successfully!" << std::endl;
        vterm_free(vt);
    } else {
        std::cerr << "Failed to initialize libvterm." << std::endl;
        return 1;
    }
    return 0;
}
