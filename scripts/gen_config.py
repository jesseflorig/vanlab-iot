"""
gen_config.py — PlatformIO pre-script: YAML device config → generated_config.h

Reads the YAML file specified by the DEVICE_CONFIG_FILE build flag (default:
devices/example.yml) and generates src/generated_config.h containing a const
DeviceConfig DEVICE_CONFIG initializer.

Usage (automatic via platformio.ini):
    extra_scripts = pre:scripts/gen_config.py

Manual usage:
    python scripts/gen_config.py devices/my_device.yml
"""

import os
import sys

# ── PlatformIO integration ────────────────────────────────────────────────────


def _get_device_file_from_env(env):
    """Extract DEVICE_CONFIG_FILE from SCons BUILD_FLAGS if present."""
    try:
        flags = env.get("BUILD_FLAGS", [])
        if isinstance(flags, str):
            flags = flags.split()
        for f in (flags or []):
            f = str(f)
            if "DEVICE_CONFIG_FILE" in f and "=" in f:
                val = f.split("=", 1)[1]
                # Strip all surrounding/embedded quotes and whitespace
                val = val.strip().strip("'").strip('"').strip("'").strip('"').strip()
                if val and not val.startswith("-") and "/" in val:
                    return val
    except Exception:
        pass
    return "devices/example.yml"


# ── YAML parsing ──────────────────────────────────────────────────────────────

def load_yaml(path):
    try:
        import yaml
    except ImportError:
        print("[gen_config] PyYAML not found. Install with: pip install pyyaml")
        sys.exit(1)

    if not os.path.exists(path):
        print(f"[gen_config] Device config not found: {path}")
        sys.exit(1)

    with open(path) as f:
        return yaml.safe_load(f)


# ── C++ code generation ───────────────────────────────────────────────────────

def c_str(s):
    """Emit a C string literal or nullptr."""
    if s is None:
        return "nullptr"
    return f'"{s}"'


def gen_gpio_config(gpio):
    pins = gpio.get("pins", [])
    roles = gpio.get("roles", [])
    count = len(pins)
    pins_init  = ", ".join(str(p) for p in pins) + (", -1" * (4 - count)) if pins else "-1, -1, -1, -1"
    roles_init = ", ".join(c_str(r) for r in roles) + (", nullptr" * (4 - len(roles))) if roles else "nullptr, nullptr, nullptr, nullptr"
    return f"{{{{ {pins_init} }}, {{ {roles_init} }}, {count}}}"


def gen_module_config(mod):
    gpio    = gen_gpio_config(mod.get("gpio", {}))
    params  = mod.get("params", {})
    params  = mod.get("params", {})
    param_lines = []
    for k, v in (params.items() if isinstance(params, dict) else []):
        param_lines.append(f'{{"{k}", "{v}"}}')
    param_count = len(param_lines)
    while len(param_lines) < 8:
        param_lines.append('{nullptr, nullptr}')
    params_init = ", ".join(param_lines)

    return (
        f"{{"
        f"{c_str(mod.get('type'))}, "
        f"{c_str(mod.get('id'))}, "
        f"{c_str(mod.get('name'))}, "
        f"{c_str(mod.get('topic_prefix'))}, "
        f"{mod.get('poll_interval_ms', 0)}, "
        f"{gpio}, "
        f"{{{params_init}}},"
        f"{param_count}"
        f"}}"
    )


def gen_bundle_config(bundle):
    modules = bundle.get("modules", [])
    mod_lines = [gen_module_config(m) for m in modules]
    # Pad to MAX_MODULES_PER_BUNDLE (8)
    while len(mod_lines) < 8:
        mod_lines.append("{nullptr, nullptr, nullptr, nullptr, 0, {{-1,-1,-1,-1},{nullptr,nullptr,nullptr,nullptr},0},{{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr}},0}")
    mod_init = ",\n        ".join(mod_lines)

    params = bundle.get("params", {})
    param_lines = []
    for k, v in (params.items() if isinstance(params, dict) else []):
        param_lines.append(f'{{"{k}", "{v}"}}')
    while len(param_lines) < 8:
        param_lines.append('{nullptr, nullptr}')
    params_init = ", ".join(param_lines)

    return (
        f"{{"
        f"{c_str(bundle.get('type'))}, "
        f"{c_str(bundle.get('id'))}, "
        f"{c_str(bundle.get('name'))}, "
        f"{c_str(bundle.get('topic_prefix'))}, "
        f"{{{mod_init}}}, "
        f"{len(modules)}, "
        f"{{{params_init}}}, "
        f"{len([k for k in params]) if isinstance(params, dict) else 0}"
        f"}}"
    )


def generate_header(yaml_data, output_path):
    device  = yaml_data.get("device", {})
    mqtt    = yaml_data.get("mqtt", {})
    bundles = yaml_data.get("bundles", [])
    standalone = yaml_data.get("standalone_modules", [])

    topic_root = mqtt.get("topic_root",
                          f"vanlab/{device.get('id', 'device')}")

    bundle_lines = [gen_bundle_config(b) for b in bundles]
    while len(bundle_lines) < 8:
        bundle_lines.append("{nullptr, nullptr, nullptr, nullptr, {}, 0, {}, 0}")
    bundles_init = ",\n    ".join(bundle_lines)

    standalone_lines = [gen_module_config(m) for m in standalone]
    while len(standalone_lines) < 16:
        standalone_lines.append("{nullptr, nullptr, nullptr, nullptr, 0, {{-1,-1,-1,-1},{nullptr,nullptr,nullptr,nullptr},0},{{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr},{nullptr,nullptr}},0}")
    standalone_init = ",\n    ".join(standalone_lines)

    header = f"""\
// generated_config.h — AUTO-GENERATED by scripts/gen_config.py
// DO NOT EDIT. Re-run 'pio run' or 'python scripts/gen_config.py' to regenerate.
#pragma once
#include "config/ConfigTypes.h"

static const DeviceConfig DEVICE_CONFIG = {{
    {c_str(device.get('id'))},
    {c_str(device.get('name'))},
    {c_str(device.get('board'))},
    // MQTTConfig
    {{
        {c_str(mqtt.get('broker_host'))},
        {mqtt.get('broker_port', 8883)},
        {c_str(mqtt.get('client_id'))},
        {mqtt.get('keepalive_s', 60)},
        {mqtt.get('socket_timeout_s', 15)},
        {c_str(topic_root)}
    }},
    // BundleConfig[MAX_BUNDLES]
    {{
    {bundles_init}
    }},
    {len(bundles)},
    // Standalone ModuleConfig[MAX_MODULES]
    {{
    {standalone_init}
    }},
    {len(standalone)}
}};
"""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w") as f:
        f.write(header)
    print(f"[gen_config] Generated {output_path}")


# ── Entry points ──────────────────────────────────────────────────────────────

def run_standalone(args):
    yaml_path   = args[0] if args else "devices/example.yml"
    output_path = os.path.join("src", "generated_config.h")
    yaml_data   = load_yaml(yaml_path)
    generate_header(yaml_data, output_path)


# When executed as a PlatformIO pre-script, SCons provides Import() as a
# builtin. Import("env") injects the SCons Environment into local scope.
# When run directly (python scripts/gen_config.py), Import is not defined.
try:
    Import("env")  # type: ignore  # noqa: F821
    # Running as PlatformIO pre-script
    project_dir = env["PROJECT_DIR"]          # type: ignore  # noqa: F821
    device_file = _get_device_file_from_env(env)  # type: ignore  # noqa: F821
    yaml_path   = os.path.join(project_dir, device_file)
    output_path = os.path.join(project_dir, "src", "generated_config.h")
    yaml_data   = load_yaml(yaml_path)
    generate_header(yaml_data, output_path)
except NameError:
    # Running standalone
    run_standalone(sys.argv[1:])
