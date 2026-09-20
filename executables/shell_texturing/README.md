# Shell Texturing

This example is the starting point for experiments with shell-based surface texturing. It currently contains the standard empty-app scene and UI scaffolding; rendering behavior can be added without changing the template application.

## Run

In VS Code, run the `Run Shell Texturing App (Debug)` task. The task builds the project and launches `ShellTexturingApp.exe` from the workspace root.

## Controls

- `F5` waits for the Vulkan device to become idle and reloads example resources as reload behavior is implemented.
- Resize the application or docked Scene window to recreate its render resources.
- Close the window to exit.

## Requirements and assets

A Vulkan-capable device, the Vulkan SDK, and the engine's shared fonts under `assets/fonts/` are required. This initial scaffold has no shell-texturing-specific assets yet; add them under `assets/shell_texturing/`.

## Expected result

The application should open a dockable UI containing Parameters and Scene windows. The Scene panel is intentionally empty until the shell-texturing render passes are implemented.
