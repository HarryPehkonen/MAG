## 1. Requirement

Create a single executable Python script (e.g., `main_poc.py`) that orchestrates the entire PoC workflow. This script will import and use the `get_plan_from_llm` function and the `FileTool` class.

## 2. Hardcoded Policy

At the top of the script, define a simple policy check function. This function will be used to guard against unintended file writes.

```python
import os

def is_allowed(path: str) -> bool:
    """Checks if a file path is in an allowed directory."""
    # Normalize path to prevent '..' traversal attacks.
    real_path = os.path.realpath(path)
    safe_cwd = os.path.realpath(os.getcwd())
    
    # Ensure the path is within the current working directory.
    if not real_path.startswith(safe_cwd):
        return False

    # Define allowed directories.
    ALLOWED_PREFIXES = ("src/", "tests/", "docs/")
    return path.startswith(ALLOWED_PREFIXES)
```

## 3. Execution Flow
-  The script's main execution block must implement the following logic:
-  Get Prompt: Take the user's request from the command-line arguments.
-  Get Plan: Call get_plan_from_llm() with the user's prompt to get the command dictionary.
-  Instantiate Tool: Create an instance of the FileTool.
-  Check Policy: Call is_allowed() on the path from the plan.
  -  If it returns False, print a "Policy Denied" message and exit.
-  Dry Run: If allowed, call tool.dry_run() with the plan's path and content. Print the returned preview string.
-  Get Confirmation: Prompt the user for approval using input("Apply this change? [y/n]: ").
-  Apply or Cancel:
  -  If the user enters 'y' (case-insensitive), call tool.apply() and print the result.
  -  Otherwise, print an "Operation cancelled" message and exit.

## 4. Example Invocation
The user should be able to run the script like this from their terminal:
```
python main_poc.py "create a file in src/api called user.js with a simple console log"
```
