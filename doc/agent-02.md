# Building an Agent from Scratch: Part 2 - Tools

In Part 1, the flow was linear: User -> AI -> User.
In Part 2, we introduce **Tools**, which creates a loop: User -> AI -> **Code** -> AI -> User.

## The "Why" of Tools

LLMs can't count letters in words accurately (they see tokens), they can't browse the web, and they can't run code. They are "Frozen brains in a concise jar".

To fix this, we give them **Hands** (Tools). But since they are text-in/text-out engines, they can't *actually* move hands. They can only **ask** us to move them.

## The Traffic (The ReAct Dance)

Here is what actually happens on the wire when you ask "What is 100 + 200?".

### Request 1 (Initial Ask)
We send the tools definition along with the prompt.
```json
{
  "messages": [{"role": "user", "content": "What is 100 + 200?"}],
  "tools": [{
    "type": "function",
    "function": {
      "name": "calc",
      "parameters": {"properties": {"expression": {"type": "string"}}}
    }
  }]
}
```

### Response 1 (The Request to Act)
The LLM does **not** give the answer. It gives a specific "stop reason" (`tool_calls`) and a payload instructions.

```json
{
  "choices": [{
    "message": {
      "role": "assistant",
      "content": null,
      "tool_calls": [{
        "id": "call_abc123",
        "function": {
          "name": "calc",
          "arguments": "{\"expression\": \"100 + 200\"}"
        }
      }]
    },
    "finish_reason": "tool_calls"
  }]
}
```

### The Execution (Client Side)
Your script sees `tool_calls`. It runs `eval("100 + 200")` and gets `300`.

### Request 2 (Reporting Back)
We must send back the **entire history**, including the assistant's request AND our result. The result message has a special role: `tool`.

```json
{
  "messages": [
    {"role": "user", "content": "..."},
    {"role": "assistant", "tool_calls": [...]},
    {
      "role": "tool",
      "tool_call_id": "call_abc123",
      "content": "300"
    }
  ]
}
```

### Response 2 (The Final Answer)
Now the LLM has the "memory" of asking + the "memory" of the result. It can finally answer.

```json
{
  "choices": [{
    "message": {
      "role": "assistant",
      "content": "The answer is 300."
    }
  }]
}
```

## Implementation: The ReAct Loop

```ruby
#!/usr/bin/env ruby
require 'net/http'
require 'json'
require 'uri'

API_URL = "https://openrouter.ai/api/v1/chat/completions"
MODEL   = "google/gemini-2.0-flash-001"
API_KEY = ENV['OPENROUTER_API_KEY']

# --- Tools Definition ---
TOOLS = [
  {
    type: "function",
    function: {
      name: "calc",
      description: "Calculate a math expression safely using Ruby",
      parameters: {
        type: "object",
        properties: {
          expression: { type: "string", description: "Math expression" }
        },
        required: ["expression"]
      }
    }
  }
]

def call_llm(messages)
  uri = URI(API_URL)
  req = Net::HTTP::Post.new(uri, 'Content-Type' => 'application/json', 'Authorization' => "Bearer #{API_KEY}")

  req.body = {
    model: MODEL,
    messages: messages,
    tools: TOOLS # <--- Passed here
  }.to_json

  resp = Net::HTTP.start(uri.hostname, uri.port, use_ssl: true) { |http| http.request(req) }
  raise "API Error: #{resp.code} #{resp.body}" if resp.code != "200"

  JSON.parse(resp.body)['choices'][0]['message']
end

# --- Main Loop ---
history = [{ role: 'system', content: "You are a helpful agent with a calculator." }]

puts "Agent v2 (Tools). Type 'exit' to quit."

loop do
  print "\n> "
  input = gets
  break if input.nil? || input.strip == 'exit'

  history << { role: 'user', content: input.strip }

  # Inner Loop: Keep calling LLM until it stops asking for tools
  loop do
    print "?" # Progress indicator
    msg = call_llm(history)
    history << msg

    # CHECK: Did the LLM allow us to stop?
    if msg['tool_calls']
      # NO: It wants us to run code.
      msg['tool_calls'].each do |tc|
        name = tc['function']['name']
        args = JSON.parse(tc['function']['arguments'])
        id   = tc['id']

        puts "\n[Tool] #{name}(#{args})"

        # Execute (DANGEROUS: eval is unsafe, but fine for tutorial)
        content = ""
        begin
          if name == 'calc'
            content = eval(args['expression']).to_s
          else
            content = "Unknown tool"
          end
        rescue => e
          content = "Error: #{e.message}"
        end

        puts " -> #{content}"

        # VITAL: Add the tool output with the correct ID
        history << {
          role: 'tool',
          tool_call_id: id,
          name: name,
          content: content
        }
      end
      # Loop continues...
    else
      # YES: It gave us text content. We are done with this turn.
      puts "\n#{msg['content']}"
      break
    end
  end
end
```
