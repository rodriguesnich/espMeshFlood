#!/usr/bin/env python3
"""
Proto build script for EFMP
Generates C code from .proto files using nanopb
"""
import os
import subprocess
import sys

try:
    Import("env")
except Exception:
    env = None

# Get the project directory in a PlatformIO-safe way
if env is not None:
    project_dir = env.subst("$PROJECT_DIR")
else:
    script_path = os.path.abspath(globals().get("__file__", os.path.join(os.getcwd(), "scripts", "proto_build.py")))
    project_dir = os.path.dirname(os.path.dirname(script_path))

proto_dir = os.path.join(project_dir, "proto")
generated_dir = os.path.join(project_dir, "src", "espMeshFlood", "generated")

# Ensure generated directory exists
os.makedirs(generated_dir, exist_ok=True)

# Find nanopb compiler
nanopb_bin = None
try:
    # Try to find in PlatformIO environment
    import platformio
    pioenv_path = os.path.expandvars("$PLATFORMIO_HOME")
    if os.path.exists(pioenv_path):
        nanopb_bin = os.path.join(pioenv_path, "packages", "framework-nanopb", "generator", "nanopb_generator.py")
except:
    pass

# Fallback: assume nanopb is in Python path
if not nanopb_bin or not os.path.exists(nanopb_bin):
    try:
        import nanopb_generator
        nanopb_bin = nanopb_generator.__file__
    except:
        print("Warning: nanopb_generator not found. Skipping proto compilation.")
        nanopb_bin = None

# Generate C files for all .proto files
if not nanopb_bin:
    pass
elif not os.path.isdir(proto_dir):
    print(f"Warning: proto directory not found: {proto_dir}")
else:
    proto_files = [f for f in os.listdir(proto_dir) if f.endswith(".proto")]

    for proto_file in proto_files:
        proto_path = os.path.join(proto_dir, proto_file)
        output_prefix = os.path.join(generated_dir, proto_file[:-6])  # Remove .proto extension
        
        print(f"Generating {proto_file}...")
        try:
            # Run nanopb generator
            result = subprocess.run(
                [sys.executable, nanopb_bin, "-D", generated_dir, proto_path],
                check=False,
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                print(f"Warning: Proto generation for {proto_file} had issues:")
                print(result.stderr)
        except Exception as e:
            print(f"Warning: Could not generate {proto_file}: {e}")

    print("Proto compilation complete.")
