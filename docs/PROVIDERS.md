# LLM Provider Architecture

MAG uses a modular provider system that makes it easy to add support for new LLM services.

## Supported Providers

| Provider | Environment Variable | Default Model | Status |
|----------|---------------------|---------------|--------|
| Anthropic Claude | `ANTHROPIC_API_KEY` | `claude-3-haiku-20240307` | ✅ Implemented |
| OpenAI | `OPENAI_API_KEY` | `gpt-3.5-turbo` | ✅ Implemented |
| Google Gemini | `GEMINI_API_KEY` | `gemini-pro` | ✅ Implemented |
| Mistral AI | `MISTRAL_API_KEY` | `mistral-small-latest` | ✅ Implemented |

## Auto-Detection

The system automatically detects which provider to use based on available environment variables, in this priority order:

1. **Anthropic** (preferred)
2. **OpenAI**
3. **Gemini**
4. **Mistral**

## Usage Examples

### Auto-Detection (Recommended)
```bash
# Set any supported API key
export ANTHROPIC_API_KEY="your-key"
# or
export OPENAI_API_KEY="your-key"

# Run the system - it will auto-detect
./llm_adapter
```

### Manual Provider Selection
```cpp
// In code
LLMClient client("anthropic");  // Uses ANTHROPIC_API_KEY
LLMClient client("openai", "sk-...");  // Explicit key
LLMClient client("gemini", "", "gemini-pro-vision");  // Custom model
```

## Adding New Providers

Adding support for a new LLM provider requires just 3 steps:

### 1. Create Provider Class

Create header file `include/providers/newprovider_provider.h`:
```cpp
#pragma once
#include "llm_provider.h"

class NewProviderProvider : public LLMProvider {
public:
    std::string get_name() const override { return "newprovider"; }
    std::string get_api_url() const override { return "https://api.newprovider.com/v1/chat"; }
    std::string get_default_model() const override { return "model-v1"; }
    std::string get_api_key_env_var() const override { return "NEWPROVIDER_API_KEY"; }
    
    nlohmann::json build_request_payload(/*...*/) const override { /*...*/ }
    std::vector<std::string> get_headers(const std::string& api_key) const override { /*...*/ }
    WriteFileCommand parse_response(const std::string& response) const override { /*...*/ }
};
```

### 2. Implement Provider Class

Create implementation file `src/providers/newprovider_provider.cpp` following the existing patterns.

### 3. Register Provider

Add to `src/common/llm_provider.cpp`:
```cpp
#include "providers/newprovider_provider.h"

// In create_provider():
} else if (provider_name == "newprovider") {
    return std::make_unique<NewProviderProvider>();

// In detect_available_provider():
const char* newprovider_key = std::getenv("NEWPROVIDER_API_KEY");
if (newprovider_key && strlen(newprovider_key) > 0) {
    return "newprovider";
}
```

That's it! The new provider is now available throughout the system with zero changes to existing code.

## Provider Interface

All providers must implement the `LLMProvider` interface:

```cpp
class LLMProvider {
public:
    virtual std::string get_name() const = 0;
    virtual std::string get_api_url() const = 0;
    virtual std::string get_default_model() const = 0;
    virtual std::string get_api_key_env_var() const = 0;
    
    virtual nlohmann::json build_request_payload(
        const std::string& system_prompt,
        const std::string& user_prompt,
        const std::string& model
    ) const = 0;
    
    virtual std::vector<std::string> get_headers(const std::string& api_key) const = 0;
    virtual WriteFileCommand parse_response(const std::string& response) const = 0;
};
```

## Benefits

- **Zero coupling**: Adding providers doesn't modify existing code
- **Consistent interface**: All providers work the same way from user perspective
- **Easy testing**: Each provider can be tested independently
- **Configuration driven**: Provider selection via environment variables
- **Future proof**: Easy to add new models and capabilities per provider