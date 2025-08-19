## 1. Requirement

Create a Python class named `FileTool` that handles all filesystem interactions. This isolates the destructive "apply" logic from the preview logic.

The class should have no `__init__` method and two public methods: `dry_run` and `apply`.

## 2. `dry_run` Method

This method generates a preview of the file operation **without modifying the filesystem**.

-   **Signature:** `dry_run(self, path: str, content: str) -> str`
-   **Behavior:**
    -   It accepts the file `path` and `content` as arguments.
    -   It **MUST NOT** write to disk.
    -   It should check if the target file already exists using `os.path.exists()`.
    -   It must return a human-readable string describing the action.
-   **Example Return Strings:**
    -   If file does not exist: `"[DRY-RUN] Will create new file 'src/app.py' with 21 bytes."`
    -   If file exists: `"[DRY-RUN] Will overwrite existing file 'src/app.py' with 21 bytes."`

## 3. `apply` Method

This method performs the actual file write operation.

-   **Signature:** `apply(self, path: str, content: str) -> str`
-   **Behavior:**
    -   It accepts the file `path` and `content`.
    -   It **MUST** write the `content` to the specified `path`.
    -   Ensure parent directories are created if they do not exist (e.g., using `os.makedirs`).
    -   It should return a human-readable string confirming the action was completed.
-   **Example Return String:**
    -   `"[APPLIED] Successfully wrote 21 bytes to 'src/app.py'."`

## 4. Policy

This tool does not need to know about the security policy. The main script will be responsible for checking if an operation is allowed before calling the tool's methods.
