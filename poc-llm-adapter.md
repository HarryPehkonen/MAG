# PoC Component: LLM Adapter

## 1. Requirement

Create a single function, `get_plan_from_llm(user_prompt: str) -> dict`, that is responsible for communicating with a Large Language Model.

## 2. Function Behavior

-   It accepts a single string argument: the user's natural language request.
-   It must call an LLM API (e.g., OpenAI, Anthropic, or a local model via Ollama).
-   It must parse the LLM's text response, which is expected to be a JSON string.
-   It must return a Python dictionary matching the Simplified Command Format defined in `poc-objective.md`.

## 3. System Prompt

Use the following system prompt to instruct the model. This prompt is designed to be simple and constrain the model's output to the required format.

```text
You are a helpful AI assistant that converts user requests into a single, specific JSON command. You must only respond with a JSON object. Do not add any conversational text or markdown formatting around the JSON.

The only command you can use is "WriteFile".

The JSON object must have three keys:
- "command": This must always be "WriteFile".
- "path": The relative path to the file.
- "content": The string content to write into the file.

Example user request: "create a python file in the src folder called app.py that prints hello world"

Example JSON response:
{
  "command": "WriteFile",
  "path": "src/app.py",
  "content": "print(\"hello world\")"
}

## 4. Implementation Notes
-  You will need a library to make HTTP requests to the LLM API (e.g., openai, anthropic, requests).
-  Hardcode the API key or use an environment variable for authentication.
-  Use Python's json library to parse the response from the LLM.
-  Assume the LLM will respond correctly. Do not build complex retry logic for this PoC.
