# Building an Agent from Scratch: Part 1 - The Loop

In this series, we will rebuild the core mechanics of an agent to understand how it works under the hood.

## The Core Concept

An agent is effectively a **while-loop** that wraps a stateful conversation with a stateless API.

1.  **State**: The agent must remember what was said (the `history` array).
2.  **Stateless API**: The LLM (e.g., Gemini via OpenRouter) has no memory. You must send the *entire* conversation history every single time.
3.  **The Loop**: User Input -> Append to History -> Send to API -> Append Reply -> Repeat.

## The Traffic (What actually happens)

When you ask "Hi", your script doesn't just send "Hi". It sends a JSON payload.

### The Request

If you utilized `curl`, it would look like this:

```bash
curl https://openrouter.ai/api/v1/chat/completions \
  -H "Authorization: Bearer $KEY" \
  -d '{
    "model": "google/gemini-2.0-flash-001",
    "messages": [
      {"role": "system", "content": "You are a helpful agent."},
      {"role": "user",   "content": "Hi"}
    ]
  }'
```

NOTICE: We explicitly tell the model who it is (`system` role) and what we said (`user` role).

### The Response

The API replies with a large JSON object. We only care about `choices[0].message`.

```json
{
  "id": "gen-123...",
  "model": "google/gemini-2.0-flash-001",
  "choices": [
    {
      "message": {
        "role": "assistant",
        "content": "Hello! How can I help you today?"
      },
      "finish_reason": "stop"
    }
  ],
  "usage": {
    "prompt_tokens": 12,
    "completion_tokens": 8,
    "total_tokens": 20
  }
}
```

**Crucial Observation**:
1.  **`choices` Array**: This list exists because the API *can* generate multiple alternative answers at once (if you set `n: 2` in the request). By default, it returns just one. We almost always want the first one (`choices[0]`).
2.  **`assistant` Role**: The response contains the message we need to save. If we don't save this to our `history` array, the model will have "amnesia" and forget it answered us.

## Implementation: The Basic Loop

Here is the minimal Ruby implementation of this loop.

```ruby
#!/usr/bin/env ruby
require 'net/http'
require 'json'
require 'uri'

# --- Configuration ---
API_URL = "https://openrouter.ai/api/v1/chat/completions"
MODEL   = "google/gemini-2.0-flash-001"

# --- The Core Function ---
# This function wraps the stateless HTTP request.
# It takes the full history (Array) and returns the new message (Hash).
def call_llm(messages)
  api_key = ENV['OPENROUTER_API_KEY']
  raise "Please set OPENROUTER_API_KEY" unless api_key

  uri = URI(API_URL)
  req = Net::HTTP::Post.new(uri, 'Content-Type' => 'application/json', 'Authorization' => "Bearer #{api_key}")

  req.body = {
    model: MODEL,
    messages: messages
  }.to_json

  resp = Net::HTTP.start(uri.hostname, uri.port, use_ssl: true) { |http| http.request(req) }

  if resp.code != "200"
    raise "API Error: #{resp.code} - #{resp.body}"
  end

  # Extract just the message object from the JSON response
  JSON.parse(resp.body)['choices'][0]['message']
end

# --- The Loop ---
# 1. Initialize State
history = [{ role: 'system', content: "You are a minimal Ruby agent." }]

puts "Agent v1. Type 'exit' to quit."

loop do
  # 2. Get Input
  print "\n> "
  input = gets
  break if input.nil? || input.strip == 'exit'

  # 3. Update State (User)
  history << { role: 'user', content: input.strip }

  begin
    # 4. Stateless Call (Send everything)
    msg = call_llm(history)

    puts "\n#{msg['content']}"

    # 5. Update State (Assistant) - CRITICAL!
    history << msg
  rescue => e
    puts "\n[!] #{e.message}"
  end
end
```

In **Part 2**, we will break this simple Request/Response pattern by introducing **Tools**, which turn the conversation into a multi-step game.
