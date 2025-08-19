# PoC Objective: Core AI Assistant Loop

## 1. Goal

The goal of this Proof of Concept (PoC) is to create a single, command-line script that demonstrates the core feedback loop of the AI assistant:

**LLM Proposes -> User Approves -> System Applies**

This PoC will focus exclusively on the `WriteFile` command. It will prove that we can safely translate a natural language request into a filesystem change with explicit user consent.

## 2. Core Components

You will create three main pieces of logic within a single script:

1.  **LLM Adapter:** A function that takes a user prompt and gets a command from an LLM.
2.  **File Tool:** A class with `dry_run` and `apply` methods for file writing.
3.  **Main Script:** The orchestrator that calls the other components and handles user interaction.

## 3. Simplified Command Format

All components will communicate using a simple Python dictionary. This is the only command format you need to support for the PoC.

```python
{
  "command": "WriteFile",
  "path": "path/to/file.txt",
  "content": "The content to be written."
}

## 4. User Workflow
The script's execution flow must follow these steps:

1.  Take a user's request as a command-line argument.
2.  Send the request to the LLM to get a WriteFile command.
3.  Check if the path in the command is allowed by a hardcoded policy.
4.  If allowed, perform a dry run and print a preview of the change.
5.  Ask the user for confirmation (y/n).
6.  If the user confirms, apply the change by writing the file to disk.

## 5. Non-Goals (What NOT to build)
- No complex TUI: Use only print() and input().
- No external policy files: The policy will be a hardcoded list of allowed path prefixes.
- No complex error handling: A simple try/except is sufficient.
- No other tools: Do not implement ReadFile, RunCommand, etc.
- No streaming or concurrency: All operations should be simple, blocking function calls.
