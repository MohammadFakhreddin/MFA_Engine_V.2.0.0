# Shell Texturing

This example renders a UV sphere with instanced shell layers. The vertex shader expands each instance along the surface normal, while the fragment shader uses a deterministic per-vertex mask and height-based lighting to create the layered surface.

## Run

In VS Code, run the `Run Shell Texturing App (Debug)` task. The task builds the project and launches `ShellTexturingApp.exe` from the workspace root.

## Controls

- Drag with the left mouse button over the Scene window to orbit the camera.
- Use the mouse wheel over the Scene window to zoom.
- Adjust sphere slice/stack resolution, layer displacement, coverage radius, shell count, albedo, and lighting in the Parameters window.
- `F5` recompiles and reloads the shell-texturing vertex and fragment shaders.
- Resize the application or docked Scene window to recreate its render resources.
- Close the window to exit.

## Requirements and assets

A Vulkan-capable device, the Vulkan SDK (including `glslc`), the shaders under `assets/shaders/shell_texturing/`, and the shared fonts under `assets/fonts/` are required. The app generates its sphere in code and does not require a model asset.

## Expected result

The application should open a dockable UI containing Parameters and Scene windows. The Scene panel should show a blue, lit sphere with a layered shell silhouette; increasing displacement makes the layers spread farther apart, while coverage radius changes their density.
