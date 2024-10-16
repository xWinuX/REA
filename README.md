# REA
REA is a Noita inspired project based on my custom Vulkan game engine you can find here: https://github.com/xWinuX/SplitEngine

REA uses compute shaders to simulate thousands of pixels efficiently and contains the following features:
- An editor that can be used to experiment with all the pixel types that exist.
- An explorer mode that procedurally generates a simple world in which the player can move a character.
- Pixels can have the following features
  - Sand-like behaviour
  - Fluid/Gas-like behaviour
  - Temperature interactions
  - Electricity interactions
  - Acidity interactions
  - Be part of a rigidbody that moves all pixels in it with the Box2D physics engine

Known limitations/problems:
- Since this projects is based on a custom engine compatability may be a problem depending on your machine
- To run and compile the project you need to install the Vulkan SDK
- Rigidbodies are a bit buggy
- The application currently runs at an uncapped framerate so the simulation may be faster or slower depending on your PC's specs
- Shader code is a bit of a mess

If you want a closer look at the project without compiling it yourself go have a look at this video that showcases most of the features and even includes a benchmark.

[![Implementation of an interactive pixel simulation - Major Project](https://img.youtube.com/vi/Z3g9vX-LSzQ/0.jpg)](https://www.youtube.com/watch?v=Z3g9vX-LSzQ)
