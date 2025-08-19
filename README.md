# MAG - Multi-Agent Gateway

A C++ implementation of an AI assistant system that safely converts natural language requests into file operations. Features a modular, plugin-based architecture using NNG for inter-process communication.

## What Does MAG Do?

MAG is an AI-powered file creation assistant that implements a safe "propose-approve-apply" workflow:

1. **Takes natural language requests** like "create a Python file in src called hello.py that prints hello world"
2. **Converts to structured commands** using your choice of LLM provider (Anthropic, OpenAI, Gemini, Mistral)
3. **Validates security policies** (only allows files in `src/`, `tests/`, `docs/`)
4. **Shows a preview** of what will be created/modified
5. **Asks for user confirmation** before making any changes
6. **Safely applies the changes** to your filesystem

Essentially, it's like having an AI coding assistant that can create files for you, but with built-in safety guardrails and explicit approval steps.

## Architecture

### Unified Launcher

MAG includes a unified launcher script (`mag`) that automatically manages all services:

- **Automatic startup**: Starts all required services in the correct order
- **Health checking**: Waits for services to be ready before proceeding  
- **Coordinated shutdown**: Gracefully stops all services on exit
- **Error handling**: Provides clear error messages if services fail to start

### Service Components

The system consists of three main components that communicate via NNG:

1. **LLM Adapter** (`llm_adapter`) - Modular LLM interface supporting multiple providers
2. **File Tool** (`file_tool`) - Handles file operations with dry-run and apply capabilities  
3. **Main Orchestrator** (`main_orchestrator`) - Coordinates the workflow and provides user interface

### Multi-Provider LLM Support

MAG features a plugin-based provider architecture supporting multiple LLM services:

| Provider | Environment Variable | Default Model | Status |
|----------|---------------------|---------------|--------|
| **Anthropic Claude** | `ANTHROPIC_API_KEY` | `claude-3-haiku-20240307` | ✅ Implemented |
| **OpenAI** | `OPENAI_API_KEY` | `gpt-3.5-turbo` | ✅ Implemented |
| **Google Gemini** | `GEMINI_API_KEY` | `gemini-pro` | ✅ Implemented |
| **Mistral AI** | `MISTRAL_API_KEY` | `mistral-small-latest` | ✅ Implemented |

**Key Features:**
- **Auto-Detection**: Automatically selects provider based on available API keys
- **Zero Coupling**: Adding new providers requires no changes to existing code
- **Strategy Pattern**: Each provider implements a common interface
- **Easy Extension**: New providers need only implement an interface and register

## Workflow

```
User Request → LLM Adapter → Policy Check → File Tool (Dry Run) → User Confirmation → File Tool (Apply)
```

## Dependencies

**Required (must be installed):**
- CMake 3.16+
- C++17 compiler (GCC 7+, Clang 7+, MSVC 2019+)
- Git (for downloading dependencies)
- libcurl development package

**Auto-downloaded if not found:**
- NNG (nanomsg-next-gen) messaging library
- FTXUI for terminal user interface  
- nlohmann/json for JSON handling
- GTest and GMock for testing

## Building

### Quick Build (Recommended)
```bash
# Install only required system dependencies
# Ubuntu/Debian:
sudo apt-get install build-essential cmake git libcurl4-openssl-dev

# CentOS/RHEL:
sudo yum install gcc-c++ cmake git libcurl-devel

# macOS:
brew install cmake git curl

# Build MAG (all other dependencies auto-download)
mkdir build && cd build
cmake .. && make
```

### Build Output
```
=== MAG Build Configuration ===
CURL: TRUE
nlohmann/json: FALSE (downloading...)
NNG: FALSE (downloading...)
FTXUI: FALSE (downloading...)
GTest: FALSE (downloading...)
===============================
```

The build system automatically downloads and configures missing dependencies, so you only need to install the minimal system requirements.

## Quick Start

### 1. Build the System
```bash
mkdir build && cd build
cmake .. && make
```

### 2. Set Your LLM API Key

**Choose any one provider** (the system will auto-detect which to use):

```bash
# Option 1: Anthropic Claude (recommended - fast and reliable)
export ANTHROPIC_API_KEY="your-anthropic-key-here"

# Option 2: OpenAI GPT
export OPENAI_API_KEY="your-openai-key-here" 

# Option 3: Google Gemini
export GEMINI_API_KEY="your-gemini-key-here"

# Option 4: Mistral AI
export MISTRAL_API_KEY="your-mistral-key-here"
```

### 3. Use MAG

**Easy Mode - Single Command (Recommended):**
```bash
# Interactive Mode (TUI)
./mag

# OR Command Line Mode  
./mag "create a Python file in src called app.py that imports requests and fetches a URL"

./mag "make a test file in tests called test_app.py with basic pytest structure"

./mag "create a markdown file in docs called API.md with API documentation template"
```

The `mag` launcher automatically starts all required services and coordinates shutdown when you exit.

**Manual Mode - For Development/Debugging:**

If you need to run services separately (e.g., for debugging):

**Terminal 1 - LLM Adapter:**
```bash
./build/src/llm_adapter
# Output: Using anthropic with model claude-3-haiku-20240307
#         LLM Adapter listening on tcp://*:5555
```

**Terminal 2 - File Tool:**
```bash
./build/src/file_tool
# Output: File Tool listening on tcp://*:5556
```

**Terminal 3 - Main Orchestrator:**
```bash
./build/src/main_orchestrator
# Opens full-screen terminal interface
```

## How to Choose LLM Provider

### Automatic Detection (Recommended)

The system automatically detects available API keys in this priority order:

1. **Anthropic** (if `ANTHROPIC_API_KEY` is set)
2. **OpenAI** (if `OPENAI_API_KEY` is set)  
3. **Gemini** (if `GEMINI_API_KEY` is set)
4. **Mistral** (if `MISTRAL_API_KEY` is set)

Just set **one** API key and the system will use it automatically.

### Manual Provider Selection

If you have multiple API keys set but want to use a specific provider:

```bash
# Force use of specific provider by only setting that key
unset ANTHROPIC_API_KEY OPENAI_API_KEY GEMINI_API_KEY MISTRAL_API_KEY
export OPENAI_API_KEY="your-openai-key"  # Will use OpenAI
```

### Provider Comparison

| Provider | Speed | Cost | Quality | Best For |
|----------|-------|------|---------|----------|
| **Anthropic Claude** | Fast | Low | High | General use (recommended) |
| **OpenAI GPT** | Medium | Medium | High | Well-tested workflows |
| **Google Gemini** | Fast | Low | Good | Google ecosystem |
| **Mistral AI** | Fast | Low | Good | European/privacy-focused |

## Example Session

```bash
$ ./mag "create a hello world script in src"

Starting MAG services...
llm_adapter started (PID: 12345)
file_tool started (PID: 12346)
Processing request: create a hello world script in src
LLM proposed: WriteFile src/hello.py
[DRY-RUN] Will create new file 'src/hello.py' with 23 bytes.
Apply this change? [y/n]: y
[APPLIED] Successfully wrote 23 bytes to 'src/hello.py'.
Shutting down MAG services...
```

## Security Features

- Policy-based path restrictions (only allows `src/`, `tests/`, `docs/` directories)
- Path traversal protection
- Explicit user confirmation for all file operations
- Dry-run preview before applying changes

## Testing

```bash
# Run all tests
cd build
make test

# Or run the test executable directly
./mag_tests
```

## Adding New LLM Providers

The modular architecture makes adding new LLM providers incredibly simple:

### Step 1: Create Provider Class
```cpp
// include/providers/newprovider_provider.h
class NewProviderProvider : public LLMProvider {
public:
    std::string get_name() const override { return "newprovider"; }
    std::string get_api_url() const override { return "https://api.newprovider.com/v1/chat"; }
    std::string get_default_model() const override { return "model-v1"; }
    std::string get_api_key_env_var() const override { return "NEWPROVIDER_API_KEY"; }
    
    // Implement request/response methods...
};
```

### Step 2: Register Provider
```cpp
// src/common/llm_provider.cpp - add one line:
} else if (provider_name == "newprovider") {
    return std::make_unique<NewProviderProvider>();
```

### Step 3: Use It
```bash
export NEWPROVIDER_API_KEY="your-key"
./llm_adapter  # Automatically detects and uses new provider
```

**That's it!** No other code changes needed. The new provider works with all existing functionality.

## Launcher Script Options

The `mag` launcher script provides several options for different use cases:

### Basic Usage
```bash
# Interactive TUI mode
./mag

# Command line mode with prompt
./mag "your request here"

# Chat mode (default) - conversational AI assistant
./mag --provider=chatgpt "Which LLM are you?"
```

### Provider Selection
```bash
# Force specific provider (overrides auto-detection)
./mag --provider=claude "create a hello world script"
./mag --provider=chatgpt "create a hello world script"  
./mag --provider=gemini "create a hello world script"
./mag --provider=mistral "create a hello world script"
```

### Development/Debugging
```bash
# Verbose mode - see service startup logs
./mag --verbose "your request"

# Manual mode - run services separately (see Manual Mode section above)
./build/src/llm_adapter &
./build/src/file_tool &
./build/src/main_orchestrator "your request"
```

### Service Management
The launcher automatically:
- Detects if build directory exists
- Starts services in dependency order (llm_adapter → file_tool → orchestrator)
- Waits for each service to be ready before proceeding
- Coordinates graceful shutdown on exit or interrupt (Ctrl+C)
- Cleans up background processes

## Project Structure

```
MAG/
├── CMakeLists.txt
├── README.md
├── docs/             # Documentation
│   └── PROVIDERS.md  # Provider system details
├── include/          # Header files
│   ├── llm_provider.h
│   ├── http_client.h
│   └── providers/    # LLM provider implementations
├── src/              # Source code
│   ├── common/       # Shared utilities & provider system
│   ├── providers/    # Provider implementations
│   ├── llm_adapter/  # LLM communication service
│   ├── file_tool/    # File operations service
│   └── orchestrator/ # Main coordination service
└── tests/            # Unit tests
```

## Advanced Usage

See [PROVIDERS.md](docs/PROVIDERS.md) for detailed documentation on the provider system, including advanced configuration options and implementation details.
