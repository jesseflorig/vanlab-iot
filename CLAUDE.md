# vanlab-iot Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-04-18

## Active Technologies
- C++14, Arduino framework (earlephilhower arduino-pico core) (002-feather-rp2040-support)
- N/A (no new persistent storage; generated_config.h and LittleFS runtime config unchanged) (002-feather-rp2040-support)
- C++14, Arduino framework (earlephilhower arduino-pico core) + PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (all existing — no new deps) (003-feather-neopixel-light)
- None (last-commanded color retained in-memory only; no flash persistence) (003-feather-neopixel-light)
- C++14 (Arduino framework); Python 3.x (build scripts) + PlatformIO CLI, PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (004-test-flash-feather)
- N/A — no new storage; `generated_config.h` and LittleFS config unchanged (004-test-flash-feather)
- C++14 (Arduino framework) + Existing — PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (no new deps) (005-remove-rgb-light-env)
- N/A — `generated_config.h` regenerated at build time; no new persistent storage (005-remove-rgb-light-env)

- C++14, Arduino framework + PubSubClient (MQTT), ESP_SSLClient (TLS/W5500), ArduinoJson (001-core-framework)

## Project Structure

```text
src/
tests/
```

## Commands

# Run native unit tests (no hardware required)
pio test --environment native

# Flash firmware — choose the environment matching your board
# NeoPixel mode (status indicator vs HA RGB light) is set by the YAML config type field
pio run --target upload --environment rp2040_shim    # Silicognition RP2040-Shim
pio run --target upload --environment feather_rp2040 # Adafruit RP2040 Feather

# Monitor serial output after flashing (auto-detects port; 115200 baud)
pio device monitor

## Code Style

C++14, Arduino framework: Follow standard conventions

## Recent Changes
- 005-remove-rgb-light-env: Added C++14 (Arduino framework) + Existing — PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (no new deps)
- 004-test-flash-feather: Added C++14 (Arduino framework); Python 3.x (build scripts) + PlatformIO CLI, PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel
- 003-feather-neopixel-light: Added C++14, Arduino framework (earlephilhower arduino-pico core) + PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (all existing — no new deps)


<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
