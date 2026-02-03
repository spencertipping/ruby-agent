#!../llm
llm <<'EOF', into: "tmux", model: :pro, limit_usd: 10, replay_e2e: true

Please write a simple version of tmux for Linux. You should use C++ and
libvterm, and CMake to build. Please use Docker tools to create the
build and testing environment.

Key features include:

+ C-a C-c to create a new tab
+ C-a C-n to switch to next tab
+ Delete tab on shell exit
+ Persistent status bar on the bottom

You don't need to implement multi-sessioning, persistence, or attachment.

Please follow this development process:

1. Create the scaffolding for the project and hello-world for libvterm
2. Implement the core tmux event loop
3. Create the tmux data structures (window/tab, probably)
4. Create the modeline
5. Run an interactive shell (bash) within each tab
6. Create a tty-aware testing harness (can use libvterm, or something else if you'd prefer)
7. Create a comprehensive series of assertions about screen and data-structure state
8. Test and develop your program until it is fully functional and passes all tests
9. Finalize by writing a short walkthrough of your work into `walkthrough.md`

Don't stop until the program is fully shipped.

EOF
