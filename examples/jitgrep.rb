#!../llm
llm <<'EOF', into: "jitgrep", model: :pro, limit_usd: 10, replay_e2e: true

Please write a simple `grep` implementation that matches lines of stdin against a regex,
but your regex engine should use AMD64 JIT compilation to compile the regex to efficient
machine code. Please hand-write your JIT.

Your project should be written in C++ and built with GNU make, with no dependencies.

Your regex implementation should include basic features; it doesn't have to be full PCRE.

Please delegate complex aspects of this to dedicated sub-agents, and iterate until you
have a fully tested and functioning command-line tool.

EOF
