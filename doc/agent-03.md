# Building an Agent from Scratch: Part 3 - The Sandbox

In Part 2, we used `eval()`.
If the specific LLM decided to output `eval("system('rm -rf /')")`, your computer would be erased.

**Rule #1 of Agents**: Never trust the Model.

## The Concept

We strictly separate the **Agent Logic** (your computer) from the **Tool Execution** (a disposable computer).

The industry standard for this is **Docker**.

### How it works (The `llm` approach)

1.  **Isolation**: The tool runs inside a container. It has its own `/home`, `/bin`, etc.
2.  **Ephemerality**: When the task is done, the container is destroyed. Any mess is gone.
3.  **Mounting**: To let the agent see files, we "mount" a specific temporary directory from your host to the container (e.g., `/tmp/agent_123` -> `/data`).

This means the agent can `rm -rf /data` and only destroy the temp folder, or `rm -rf /` and only destroy the container OS.

## The Traffic

The API traffic doesn't change here! The LLM doesn't know it's in a sandbox. It just asks for `bash`.
We change the **Execution** step of our loop.

**Old Execution**:
```ruby
system(cmd) # RUNS ON HOST
```

**New Execution**:
```ruby
system("docker run alpine #{cmd}") # RUNS IN CONTAINER
```

## Implementation: The Sandboxed Agent

```ruby
#!/usr/bin/env ruby
require 'net/http'
require 'json'
require 'uri'
require 'open3'
require 'tmpdir'
require 'fileutils'

# --- Config ---
API_URL = "https://openrouter.ai/api/v1/chat/completions"
MODEL   = "google/gemini-2.0-flash-001"
API_KEY = ENV['OPENROUTER_API_KEY']

# --- Sandbox Class ---
# This wrapper handles the Docker complexity.
class Sandbox
  attr_reader :host_workdir

  def initialize
    @host_workdir = Dir.mktmpdir("agent_sandbox")
    puts "[Sandbox] Created at #{@host_workdir}. I am safe."
  end

  def run(command)
    # The magical incantation to run safely:
    # --rm: Delete container after run
    # -v: Mount our temp dir
    # -w: Start inside that dir
    # -u: (Optional) Run as non-root (skipped here for brevity but recommended)
    docker_cmd = [
      "docker", "run", "--rm",
      "-v", "#{@host_workdir}:/data",
      "-w", "/data",
      "alpine:latest",
      "/bin/sh", "-c", command
    ]

    stdout, stderr, status = Open3.capture3(*docker_cmd)

    output = stdout.strip
    output += "\n[Stderr]: #{stderr.strip}" unless stderr.strip.empty?
    output
  end

  def cleanup
    puts "[Sandbox] Cleaning up..."
    FileUtils.remove_entry(@host_workdir)
  end
end

# --- Tools ---
TOOLS = [
  {
    type: "function",
    function: {
      name: "bash",
      description: "Run a shell command in an Alpine Linux sandbox",
      parameters: {
        type: "object",
        properties: {
          cmd: { type: "string", description: "Shell command" }
        },
        required: ["cmd"]
      }
    }
  }
]

# --- LLM Call ---
def call_llm(messages)
  uri = URI(API_URL)
  req = Net::HTTP::Post.new(uri, 'Content-Type' => 'application/json', 'Authorization' => "Bearer #{API_KEY}")
  req.body = { model: MODEL, messages: messages, tools: TOOLS }.to_json
  resp = Net::HTTP.start(uri.hostname, uri.port, use_ssl: true) { |http| http.request(req) }
  raise "API Error: #{resp.code} #{resp.body}" if resp.code != "200"
  JSON.parse(resp.body)['choices'][0]['message']
end

# --- Main ---
sandbox = Sandbox.new
# Ensure cleanup even if we crash (Ruby standard)
at_exit { sandbox.cleanup }

history = [{ role: 'system', content: "You are a safe agent. You use the 'bash' tool to explore files and solve problems." }]

puts "Agent v3 (Sandbox). Type 'exit' to quit."

loop do
  print "\n> "
  input = gets
  break if input.nil? || input.strip == 'exit'

  history << { role: 'user', content: input.strip }

  loop do # ReAct Loop
    print "?"
    msg = call_llm(history)
    history << msg

    if msg['tool_calls']
      msg['tool_calls'].each do |tc|
        name = tc['function']['name']
        args = JSON.parse(tc['function']['arguments'])
        id   = tc['id']

        puts "\n[Tool] #{name}(#{args})"

        # --- execution switch ---
        result = if name == 'bash'
          sandbox.run(args['cmd'])
        else
          "Error: Unknown tool"
        end
        # ------------------------

        # Truncation: Docker output can be huge. We must not crash the LLM context.
        if result.length > 2000
          result = result[0..2000] + "\n... [truncated]"
        end

        puts " -> #{result}"

        history << { role: 'tool', tool_call_id: id, name: name, content: result }
      end
    else
      puts "\n#{msg['content']}"
      break
    end
  end
end
```
