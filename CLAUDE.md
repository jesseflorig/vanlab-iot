# vanlab-iot Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-04-07

## Active Technologies
- C++14, Arduino framework (earlephilhower arduino-pico core) (002-feather-rp2040-support)
- N/A (no new persistent storage; generated_config.h and LittleFS runtime config unchanged) (002-feather-rp2040-support)
- C++14, Arduino framework (earlephilhower arduino-pico core) + PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (all existing — no new deps) (003-feather-neopixel-light)
- None (last-commanded color retained in-memory only; no flash persistence) (003-feather-neopixel-light)

- C++14, Arduino framework + PubSubClient (MQTT), ESP_SSLClient (TLS/W5500), ArduinoJson (001-core-framework)

## Project Structure

```text
src/
tests/
```

## Commands

# Add commands for C++14, Arduino framework

## Code Style

C++14, Arduino framework: Follow standard conventions

## Recent Changes
- 003-feather-neopixel-light: Added C++14, Arduino framework (earlephilhower arduino-pico core) + PubSubClient, ESP_SSLClient, ArduinoJson, Adafruit NeoPixel (all existing — no new deps)
- 002-feather-rp2040-support: Added C++14, Arduino framework (earlephilhower arduino-pico core)

- 001-core-framework: Added C++14, Arduino framework + PubSubClient (MQTT), ESP_SSLClient (TLS/W5500), ArduinoJson

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
