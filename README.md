# Vulkan Shader Stash
The goal of this library is to provide easy to integrate solution for creation and storage of Vulkan shader modules.

This library exposes single class capable of loading shaders from hard drive, compiling them on the fly, and facilitating efficient querying of previously loaded shaders. Moreover, it supports hot reloading of shaders during runtime, enabling seamless updates and modifications.

## Library Dependencies
1. [**vulkan-hpp**](https://github.com/KhronosGroup/Vulkan-Hpp)
2. [**shaderc**](https://github.com/google/shaderc)

Both of them are shipped together with Vulkan SDK.

## Setup
Copy the shader-stash.hpp header file into your project and link to the shaderc_shared library which can be found in the *{VulkanSDK}/{SDK Version}/Lib* folder.

## Usage

##### Include the library:
```c++
#include "shader-stash.hpp"
```
##### Create ShaderStash object:
```c++
ShaderStash shaders {vulkanDevice, "/shaders"};
```
##### Add either particular shader files or entire shader folders:
```c++
shaders.add("example_shader.vert");
shaders.add("/other_shaders");
// or using method chaining:
shaders
    .add("example_shader.vert")
    .add("/other_shaders");
```

##### Turn on hot reload for on the fly shader updates:
```c++
shaders.setHotReload(true);
```

##### Retrieve stored shader module handles:
```c++
std::shared_ptr<vk::raii::ShaderModule> exampleShader = shaders.get("example_shader.vert");
```

## Roadmap
* [x] Shader runtime compilation
* [x] Shader hot reload
* [ ] Shader change callbacks