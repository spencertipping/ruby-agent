# Building an Agent from Scratch: Part 4 - Memory

As conversations get longer, the `history` array grows. This causes two problems:
1.  **Cost**: You pay for every token you send, *every time* you send the history. 1 msg = $0.001. 100 msgs = $0.10.
2.  **Context Window**: Models have a hard limit (e.g., 1M tokens). If you exceed it, the API returns `400 Bad Request`.

## The "Why" of Compaction

Imagine reading a book where you must re-read every previous page before reading the new one.
-   Page 1: Read Pg 1.
-   Page 2: Read Pg 1, Pg 2.
-   Page 100: Read Pg 1 ... Pg 100.

Eventually, you will die of old age (or run out of RAM).
We need to **forget** things.

## Visualizing the Memory Pressure

If we don't manage memory, the graph of "Total Tokens Processed" is quadratic ($O(n^2)$).

```
Step 1: [A]
Step 2: [A, B]
Step 3: [A, B, C]
...
Step 100: [A ... Z ... ] (HUGE)
```

We want it to be linear (sliding window).

```
Step 100: [ ... X, Y, Z] (Fixed size)
```

## Strategy: FIFO with System Persistence

We cannot just delete everything. We usually want to keep:
1.  The **System Prompt** (Who am I?).
2.  The **Most Recent Messages** (What are we talking about?).
3.  (Optional) **Pinned Information** (Key facts).

The `llm` tool uses a "Compactor" before every API call to enforce this.

## Implementation: The Compacting Agent

```ruby
#!/usr/bin/env ruby
require 'net/http'
require 'json'
require 'uri'

API_URL = "https://openrouter.ai/api/v1/chat/completions"
MODEL   = "google/gemini-2.0-flash-001"
API_KEY = ENV['OPENROUTER_API_KEY']

# Heuristic: 4 chars ~= 1 token. Fast and "good enough" for safety.
def count_tokens(messages)
  messages.map { |m| m.to_s.length / 4 }.sum
end

# The Compactor logic
def compact_history(history, limit: 8000)
  current = count_tokens(history)
  return history if current < limit

  puts "[Memory] Context full (#{current} > #{limit}). Pruning..."

  # 1. Protect the System Prompt
  sys = history.select { |m| m[:role] == 'system' }
  rest = history.reject { |m| m[:role] == 'system' }

  # 2. Drop oldest messages until we fit
  # Note: A real implementation might summarize them first!
  while count_tokens(sys + rest) > limit && !rest.empty?
    dropped = rest.shift
    # puts "Dropped: #{dropped[:content]}"
  end

  sys + rest
end

def call_llm(messages)
  # IMPORTANT: We compact the VIEW of the history we send.
  # We might still keep the full history in our local variable if we wanted to log it.
  payload_msgs = compact_history(messages)

  uri = URI(API_URL)
  req = Net::HTTP::Post.new(uri, 'Content-Type' => 'application/json', 'Authorization' => "Bearer #{API_KEY}")
  req.body = { model: MODEL, messages: payload_msgs }.to_json

  resp = Net::HTTP.start(uri.hostname, uri.port, use_ssl: true) { |http| http.request(req) }
  raise "API Error: #{resp.code} #{resp.body}" if resp.code != "200"
  JSON.parse(resp.body)['choices'][0]['message']
end

# --- Main ---
history = [{ role: 'system', content: "You are a chatty agent." }]

puts "Agent v4 (Memory). Type 'exit' to quit."

loop do
  print "\n> "
  input = gets
  break if input.nil? || input.strip == 'exit'

  history << { role: 'user', content: input.strip }

  begin
    msg = call_llm(history)
    puts "\n#{msg['content']}"
    history << msg

    # Debug info to show the cost
    puts "\n[Debug] History: #{history.size} msgs. Tokens: #{count_tokens(history)}"
  rescue => e
    puts "\n[!] #{e.message}"
  end
end
```

## Summary

You have now built a complete Agent from scratch!
1.  **Loop**: Managing state.
2.  **Tools**: Interacting with the world.
3.  **Sandbox**: Doing it safely.
4.  **Memory**: Doing it sustainably.

The `llm` tool in this repo is just a highly polished version of these four concepts, with added features like Streaming, Pinning, and multiple model support.
