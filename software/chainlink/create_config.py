#!/usr/bin/env python3
import sys
import os
import traceback # Import the traceback module

# --- Add the 'proto_gen' folder to our Python path ---
script_dir = os.path.dirname(os.path.abspath(__file__))
proto_gen_path = os.path.join(script_dir, 'proto_gen')
sys.path.append(proto_gen_path)

# --- Debugging: Check for both files ---
splitflap_file = os.path.join(proto_gen_path, 'splitflap_pb2.py')
nanopb_file = os.path.join(proto_gen_path, 'nanopb_pb2.py')

if not os.path.exists(splitflap_file):
    print(f"❌ Error: File NOT found:\n{splitflap_file}")
    sys.exit(1)

if not os.path.exists(nanopb_file):
    print(f"❌ Error: Dependency NOT found:\n{nanopb_file}")
    print("This file is also required. Please run the 'build_protos.py' script first.")
    sys.exit(1)

print(f"✅ Found required files in:\n{proto_gen_path}")

# --- Import the blueprint ---
try:
    import splitflap_pb2
except ImportError:
    print("\n--- ❌ AN IMPORT ERROR OCCURRED ---")
    # This will print the *actual* error (e.g., "cannot import name X")
    traceback.print_exc()
    print("---------------------------------")
    sys.exit(1)

# --- Configuration ---
NUM_MODULES = 6
NUM_FLAPS = 40  # 40 is a very common number. Change if you know yours is different.

# 1. Create the config object
config = splitflap_pb2.PersistentConfiguration()

# 2. Set default values
config.version = 1
config.num_flaps = NUM_FLAPS

# Add a default offset of 0 for all 6 modules
for _ in range(NUM_MODULES):
    config.module_offset_steps.append(0)

# 3. Save the binary file
output_filename = "config.pb"
# Save the file in the *current directory* (software/chainlink)
with open(output_filename, "wb") as f:
    f.write(config.SerializeToString())

print(f"\n✅ Success! Created '{output_filename}' in this directory.")
print(f"   (Configured for {NUM_MODULES} modules, {NUM_FLAPS} flaps each)")
print("\nNEXT STEP: Move this new 'config.pb' file into your 'firmware/data' folder.")