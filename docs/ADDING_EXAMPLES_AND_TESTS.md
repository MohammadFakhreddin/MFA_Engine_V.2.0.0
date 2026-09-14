# Adding examples and tests

MFA Engine uses example applications for interactive rendering demonstrations and manual visual tests. Automated, non-interactive checks should be registered separately with CTest.

## Choose the right type

| Need | Location | Execution |
| --- | --- | --- |
| Rendering demo, GPU experiment, or visual regression check | `executables/<example_name>/` | Launch the example and inspect the result |
| Deterministic unit or integration check | `tests/<component>/` | Run it through CTest |

Do not turn an interactive Vulkan application into an automated unit test. Keep the rendering example, then extract deterministic logic into engine code that can be tested without a window when practical.

## Add an example application

Use the following naming scheme. For an example named "shell texturing":

| Item | Name |
| --- | --- |
| Directory | `executables/shell_texturing/` |
| Target and application class | `ShellTexturingApp` |
| Implementation | `ShellTexturingApp.cpp` |
| Header | `ShellTexturingApp.hpp` |
| Entry point | `ShellTexturingAppMain.cpp` |
| VS Code task | `Run Shell Texturing App (Debug)` |

### 1. Copy the template

Copy `executables/empty_app` to `executables/<example_name>`. Use a snake-case directory name. Leave the template unchanged.

Rename the copied source files to match the new `PascalCaseApp` name. Rename the class, constructors, method definitions, includes, include guard, logical-device application name, and README content as well.

Before continuing, this search should produce no stale template references:

```powershell
rg -n "EmptyApp|Empty project" executables/<example_name>
```

### 2. Define the target

Update the copied `CMakeLists.txt` so its target and source list use the new names:

```cmake
set(EXECUTABLE "ExampleNameApp")

list(
    APPEND EXECUTABLE_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ExampleNameAppMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExampleNameApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExampleNameApp.hpp"
)

add_executable(${EXECUTABLE} ${EXECUTABLE_RESOURCES})
```

Retain only the engine libraries the example uses. The empty app's link list is an acceptable starting point.

Register the directory in the root `CMakeLists.txt` near the other examples:

```cmake
add_subdirectory("${CMAKE_SOURCE_DIR}/executables/example_name")
```

Never add this line before the directory and its local `CMakeLists.txt` exist.

### 3. Add assets and documentation

Place example-specific shaders, models, textures, and configuration under `assets/<example_name>/`. Reusable shaders and resources may remain in their shared asset directories.

The example README must state:

- What the example demonstrates.
- How to run it.
- Its controls and reload shortcuts.
- Required assets or hardware features.
- What a successful render should look like.

### 4. Add a VS Code launch task

Add a task to `.vscode/tasks.json` and keep the existing tasks intact:

```json
{
    "label": "Run Example Name App (Debug)",
    "type": "process",
    "command": "${workspaceFolder}\\build\\executables\\example_name\\Debug\\ExampleNameApp.exe",
    "dependsOn": "Build All (Debug)",
    "dependsOrder": "sequence",
    "options": {
        "cwd": "${workspaceFolder}"
    },
    "presentation": {
        "reveal": "always",
        "panel": "dedicated",
        "clear": true
    },
    "problemMatcher": []
}
```

Validate the file after editing it:

```powershell
Get-Content -Raw .vscode/tasks.json | ConvertFrom-Json | Out-Null
```

### 5. Build and verify

Configure the project if needed, then build only the new target for a quick check:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target ExampleNameApp
```

Launch the app and confirm that it opens, resizes, renders the expected content, and closes cleanly. Report configuration or environment blockers instead of hiding a skipped check.

## Add an automated test

The repository does not currently contain a first-party automated test tree. The first automated test should introduce the following minimal CTest structure; later tests should reuse it.

### 1. Enable CTest once

Add this near the top-level project configuration in the root `CMakeLists.txt`:

```cmake
include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

Create `tests/CMakeLists.txt` and register each component test directory from there.

### 2. Create a test executable

Use `tests/<component>/CMakeLists.txt` with a target named `<Component>Tests`:

```cmake
add_executable(ComponentTests
    ComponentTests.cpp
)

target_link_libraries(ComponentTests PRIVATE ComponentLibrary)

add_test(
    NAME ComponentTests
    COMMAND ComponentTests
)
```

Use the project's existing assertion facilities or the standard library until a dedicated test framework is intentionally adopted. The executable must return `0` on success and a nonzero value on failure.

### 3. Keep tests reliable

- Give each test one clear behavior and failure message.
- Avoid network access, user-specific absolute paths, interactive windows, timing-sensitive sleeps, and shared mutable state.
- Use small checked-in fixtures. Resolve fixture paths through CMake or `ASSET_DIR`, not the current shell directory.
- Separate GPU/hardware-dependent checks from ordinary unit tests and label them:

```cmake
set_tests_properties(ComponentGpuTests PROPERTIES LABELS "integration;gpu")
```

### 4. Run tests

Build the test target and run CTest with failure output enabled:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug --target ComponentTests
ctest --test-dir build -C Debug --output-on-failure
```

To run one test or one label:

```powershell
ctest --test-dir build -C Debug -R ComponentTests --output-on-failure
ctest --test-dir build -C Debug -L integration --output-on-failure
```

## Review checklist

- Names are consistent across the directory, source files, target, class, and task.
- CMake references only directories and files that exist.
- The new example or test has focused documentation.
- JSON and CMake configuration are valid.
- The new target builds, or the exact external blocker is documented.
- Existing examples, tasks, user changes, and submodules remain untouched.
