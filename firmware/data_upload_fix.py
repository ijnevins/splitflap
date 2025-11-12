# type: ignore
Import("env")
import os
import sys

# Define the function to find the tool based on PlatformIO's internal package paths
def find_mkfatfs_tool(env):
    try:
        # Use PlatformIO's API to find the location of the package tool
        tool_package = env.PioPlatform().get_package("tool-mkfatfs")
        if tool_package:
            # The actual tool executable is usually inside the base package directory
            return os.path.join(tool_package.path, "mkfatfs")
    except Exception as e:
        sys.stderr.write("Failed to locate tool-mkfatfs package. Error: " + str(e) + "\n")
        return None
    return None

# Get the path to the mkfatfs executable
MKFATFS_TOOL_PATH = find_mkfatfs_tool(env)

if not MKFATFS_TOOL_PATH or not os.path.exists(MKFATFS_TOOL_PATH):
    # This block executes if the package is still not found/registered.
    # It attempts to fall back to the project's own environment lookup.
    env.Tool("mkfatfs")
else:
    # Tool is found, replace the upload command to use the explicit path
    env.Replace(
        MKEFS_IMAGE=MKFATFS_TOOL_PATH,
        UPLOADFS_APPLY=env.VerboseAction(
            lambda source, target, env: env.Execute(
                "\"$MKEFS_IMAGE$\" -c \"%s\" -s %s \"%s\""
                % (env.subst("$PROJECT_DIR/firmware/data"),
                   env.subst("$FS_SIZE"),
                   target[0].path)
            ),
            "Building FS image"
        )
    )