# LLM Agent Runner

`llm` is a powerful, self-contained Ruby script that serves as an agentic framework runner. It allows you to script LLM interactions with a rich set of tools (filesystem, Docker) and persistent context management.

## Setup

Ensure you have the following environment variable set:

```bash
export OPENROUTER_API_KEY="sk-or-..."
```

The script manages its own cache in `.llm-cache` and `.llm-pinned` directories relative to the working directory.

## Usage

### Running Scripts

You can use `llm` as an interpreter for Ruby scripts. The idiom is to write a Ruby script with a "heredoc" prompt passed to the global `llm` function.

```bash
./llm my_script.rb
```

Or make your script executable with a shebang:

```ruby
#!/path/to/llm
llm <<'EOF', into: "workspace_dir", model: :pro
  Please build a web server...
EOF
```

### The `llm` Function

The core API is the global `llm` function available in the script environment.

```ruby
result = llm(
  prompt,           # String or Array of messages
  schema: nil,      # Optional JSON schema for structured output
  tools: :docker,   # Tool set: :docker (default), :basic, or hash
  replay_e2e: false, # If true, ignores E2E cache
  pin: false,       # If true, pins the result to .llm-pinned
  keep_sandbox: false, # If true, keeps buffer sandbox after exit
  into: "dir",      # Persistent workspace directory (enables persistence)
  limit_usd: nil,   # Budget limit in USD
  model: :flash,    # Model alias (:opus, :pro, :flash)
  compaction: nil,  # Context compaction strategy (:fifo, :summarize)
  seed: nil         # RNG seed for deterministic generation
)
```

#### Return Value
Returns an `LLMResult` object with method accessors:
- `result.s`: Content as a string (or JSON string).
- `result.x`: Content as raw object (Hash if JSON parsed).
- `result['usage']`: Token usage stats.

### Model Aliases

- `:opus` -> `anthropic/claude-opus-4.5`
- `:pro`  -> `google/gemini-3-pro-preview`
- `:flash` -> `google/gemini-3-flash-preview`

## Built-in Tools

The agent has access to a variety of tools depending on the `tools` argument.

### Filesystem & Analysis (Basic)
Available in both `:basic` and `:docker` modes.

- **`ls(path: ".", recursive: true)`**: List files.
- **`read_file(path: "...")`**: Read entire file content.
- **`read_lines(path: "...", start_line: 1, end_line: N)`**: Read specific lines with line numbers.
- **`write_file(path: "...", content: "...")`**: Overwrite a file.
- **`write_lines(path: "...", line: N, content: "...", context: "...")`**: Surgically replace lines. Requires exact context match for safety.
- **`grep(regex: "...", path: ".")`**: Search for patterns in files.
- **`delegate_to_flash_preview(path: "...", instruction: "...")`**: Fast, cheap file editing using a smaller model. Generates a `.flash_preview` file and returns a diff.
- **`ask_file(path: "...", question: "...")`**: Fast, cheap RAG-style query on a single file.

### Docker (Advanced)
Available when `tools: :docker` (default).

- **`docker_run(image: "...", command: "...")`**: Run a one-off command in a container with the workspace mounted at `/sandbox` (or `/data` if persistent).
- **`docker_build(dockerfile_path: "...")`**: Build a Docker image from the sandbox.
- **`docker_start(image: "...", command: "...")`**: Start a *background* container session.
- **`docker_stop(session_id: "...")`**: Stop a background session.
- **`docker_input(session_id: "...", input: "...")`**: Send stdin to a session.
- **`docker_logs(session_id: "...")`**: Read stdout/stderr from a session.

### Task Notebook
Tools available if a `notebook` object is passed to `llm()`.

- **`task_add(text: "...")`**: Add a todo item.
- **`task_update(id: N, status: "...", text: "...")`**: Update task status (`todo`, `in_progress`, `done`).

## Advanced Features

### Caching & Determinism

#### E2E Cache
The `llm` runner features an aggressive "end-to-end" cache. Before execution, it calculates a hash key based on:
- The messages/prompt
- Tool definitions
- Arguments (schema, model, etc.)
- Task notebook state

If a cache hit is found in `.llm-cache` (matching specific file hash), the **entire execution is skipped** and the cached result is returned instantly. This enables rapid iteration on long multi-step scripts.
- **Override**: Pass `replay_e2e: true` to bypass this cache and force re-execution.

#### Determinism
The `seed` argument allows for deterministic generation by the LLM, but has limitations:
- **Model Variance**: Floating-point non-determinism in GPUs means outputs may still vary slightly.
- **Environment**: The seed controls the LLM, not the world. If `ls` returns different files or `Time.now` changes, the prompt for subsequent turns changes, breaking the deterministic chain.

### Compaction Strategies
Used to manage context window for long-running conversations.
- `:fifo` (default for standard): Keeps system prompt + last N messages.
- `:summarize`: Summarizes older compacted messages using a cheaper model to retain context.

### Interactive Mode
The `i(result)` helper prints a formatted interaction summary to stdout.

```ruby
res = llm("Hello")
i(res)
```

### String Extension
Strings have a helper for quick queries:

```ruby
"some content".llm_get("Summarize this")
```
