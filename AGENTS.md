# Repository contribution rules

These rules apply to the entire repository.

## Preserve the working tree

- Treat existing modifications and untracked files as user-owned work.
- Do not edit generated build output or third-party code in `submodules/` unless the task explicitly requires it.
- Do not commit generated shader binaries, build artifacts, or IDE output unless they are already intentionally tracked and the task requires updating them.

## Examples and visual tests

- Rendering demonstrations and manual visual tests belong in `executables/<example_name>/`.
- Use `snake_case` for the directory and `PascalCaseApp` for the CMake target, class, and source-file prefix.
- Start from `executables/empty_app` unless another example is a closer requested template. Copy the template; do not modify it while creating the new example.
- Rename all copied files, classes, constructors, includes, header guards, window titles, and CMake source entries. Do not leave template names in the new directory.
- Give every example its own README describing its purpose, controls, assets, and expected result.
- Register every example with `add_subdirectory(...)` in the root `CMakeLists.txt`.
- Add a `Run <Example> App (Debug)` entry to `.vscode/tasks.json`. It must depend on `Build All (Debug)`, run from `${workspaceFolder}`, and point to the target's Debug executable.
- Keep example-specific assets under a clearly named directory in `assets/`. Keep reusable engine assets in their existing shared locations.
- Validate `.vscode/tasks.json`, search the new directory for stale template names, and build the new target when the environment permits it.

## Automated tests

- First determine whether a requested test is automated or visual. A GPU/window-driven visual test is normally an example under `executables/`; a deterministic, non-interactive check is an automated test.
- Put first-party automated tests under `tests/<component>/` and name targets `<Component>Tests`.
- Register automated tests with CTest using `add_test`. If the repository has not yet enabled CTest, add `include(CTest)` and gate the test tree with `if(BUILD_TESTING)`.
- Tests must return a nonzero exit code on failure, avoid interactive input, and avoid depending on execution order.
- Keep unit tests small and deterministic. Label hardware-dependent or long-running tests as integration tests and document their prerequisites.
- Run the relevant test target and `ctest --test-dir build -C Debug --output-on-failure` when the environment permits it.

For the full workflow and templates, see `docs/ADDING_EXAMPLES_AND_TESTS.md`.
