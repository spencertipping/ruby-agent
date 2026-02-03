# OpenRouter API
Direct usage of the OpenRouter API:

``` bash
$ curl https://openrouter.ai/api/v1/chat/completions \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $OPENROUTER_API_KEY" \
    -d '
  {
    "model": "google/gemini-3-flash-preview",
    "messages": [
      {"role":    "user",
       "content": "Generate data matching the schema. Please think carefully about this, as the requirements are deceptive."}
    ],
    "reasoning": {"max_tokens": 1000},
    "response_format": {
      "type": "json_schema",
      "json_schema": {
        "name": "my_schema",
        "strict": true,
        "schema": {
          "type": "object",
          "properties": {
            "foo": {
              "type": "array",
              "items": {
                "type": "object",
                "properties": {"bar": { "type": "integer" }},
                "required": ["bar"],
                "additionalProperties": false
              }
            },
            "bif": {
              "type": "string",
              "description": "this field should contain a malformed value"
            },
            "complex_strategy": {
              "type": "string",
              "description": "an ideal political strategy for the Democrats to adopt to win rural counties in 2028"
            }
          },
          "required": ["foo", "bif", "complex_strategy"],
          "additionalProperties": false,
          "strict": true
        }
      }
    }
  }' | jq .
```

We get a reply that looks like this:

```json
{
  "id": "gen-1766107352-qwhPp17HeXK7ivKFmPFa",
  "provider": "Google",
  "model": "google/gemini-3-flash-preview",
  "object": "chat.completion",
  "created": 1766107352,
  "choices": [
    {
      "logprobs": null,
      "finish_reason": "stop",
      "native_finish_reason": "STOP",
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "{\n  \"foo\": [\n    {\n      \"bar\": 102\n    },\n    {\n      \"bar\": 27\n    }\n  ],\n  \"bif\": \"ERR_NULL_POINTER_EXCEPTION: <0x0000> [\\\\u00\\\\unparseable_block] !! {{unclosed_tag\",\n  \"complex_strategy\": \"The ideal 2028 strategy for rural engagement focuses on 'Economic Sovereignty.' This involves: 1) Proposing a 'Rural GI Bill' that provides debt forgiveness for service-oriented professions (healthcare, education, trades) in low-population counties. 2) Prioritizing localized infrastructure like municipal broadband and localized food processing hubs to reduce dependence on global supply chains. 3) Rebranding environmental initiatives as 'Energy Independence' strategies that pay farmers for carbon sequestration and host community-owned wind/solar cooperatives. 4) Moving away from nationalized cultural narratives to focus on 'Kitchen Table' issues like protecting rural post offices and stabilizing local hospital funding, delivered through non-partisan community stakeholders rather than traditional political advertising.\"\n}",
        "refusal": null,
        "reasoning": "**Reflecting on Schema Creation**\n\nI've been focusing on the JSON schema provided. Specifically, I'm examining the properties: `foo`, an array of objects with an integer `bar`; `bif`, a string requiring a malformed value; and `complex_strategy`, a string. My primary goal is understanding how to validate these.\n\n\n**Validating Schema Properties**\n\nI'm now zeroing in on the `bif` property. The \"malformed value\" instruction is a bit of a puzzle. I'm leaning toward a string containing obviously incorrect data but still parseable JSON. I'm focusing on the `complex_strategy` next, considering how to embody the rural political strategy.\n\n\n**Refining the JSON Object**\n\n\\n\\n\n\nI'm finalizing the object creation. I will represent \"malformed\" content within the `bif` string using escaped characters and invalid syntax, while still adhering to valid JSON syntax. I've also refined the `complex_strategy` string, focusing on an 'Economic Dignity' platform for rural voters, emphasizing localized infrastructure and downplaying cultural issues. It now seems robust.\n\n\n**Crafting the Final JSON**\n\n\\n\\n\n\nI'm now fully satisfied with the structure of the `foo` array and have thoroughly assessed the \"malformed value\" specification for `bif`. The content of `bif` is designed to be problematic, representing incorrect values within the string. Finally, the `complex_strategy` has been updated to reflect the 'Rural Renaissance' framework. The final JSON object includes vocational training, healthcare, community engagement, and a focus on decentralization.\n\n\n",
        "reasoning_details": [
          {
            "format": "google-gemini-v1",
            "index": 0,
            "type": "reasoning.text",
            "text": "**Reflecting on Schema Creation**\n\nI've been focusing on the JSON schema provided. Specifically, I'm examining the properties: `foo`, an array of objects with an integer `bar`; `bif`, a string requiring a malformed value; and `complex_strategy`, a string. My primary goal is understanding how to validate these.\n\n\n**Validating Schema Properties**\n\nI'm now zeroing in on the `bif` property. The \"malformed value\" instruction is a bit of a puzzle. I'm leaning toward a string containing obviously incorrect data but still parseable JSON. I'm focusing on the `complex_strategy` next, considering how to embody the rural political strategy.\n\n\n**Refining the JSON Object**\n\n\\n\\n\n\nI'm finalizing the object creation. I will represent \"malformed\" content within the `bif` string using escaped characters and invalid syntax, while still adhering to valid JSON syntax. I've also refined the `complex_strategy` string, focusing on an 'Economic Dignity' platform for rural voters, emphasizing localized infrastructure and downplaying cultural issues. It now seems robust.\n\n\n**Crafting the Final JSON**\n\n\\n\\n\n\nI'm now fully satisfied with the structure of the `foo` array and have thoroughly assessed the \"malformed value\" specification for `bif`. The content of `bif` is designed to be problematic, representing incorrect values within the string. Finally, the `complex_strategy` has been updated to reflect the 'Rural Renaissance' framework. The final JSON object includes vocational training, healthcare, community engagement, and a focus on decentralization.\n\n\n"
          },
          {
            "format": "google-gemini-v1",
            "index": 0,
            "type": "reasoning.encrypted",
            "data": "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"
          }
        ]
      }
    }
  ],
  "usage": {
    "prompt_tokens": 18,
    "completion_tokens": 1067,
    "total_tokens": 1085,
    "cost": 0.00321,
    "is_byok": false,
    "prompt_tokens_details": {
      "cached_tokens": 0,
      "audio_tokens": 0,
      "video_tokens": 0
    },
    "cost_details": {
      "upstream_inference_cost": null,
      "upstream_inference_prompt_cost": 0.000009,
      "upstream_inference_completions_cost": 0.003201
    },
    "completion_tokens_details": {
      "reasoning_tokens": 826,
      "image_tokens": 0
    }
  }
}
```


## Notes
The response format is not part of the prompt for billing purposes, which is interesting and useful.

`reasoning.encrypted` must be passed back intact and in its entirety.
