[**Build Status of Main Branch**](https://dev.azure.com/DFS2-C29/DFSII/_build/latest?definitionId=22&branchName=main)

----

# **Rahul's DirectX 12 Rendering Engine Directed Focus Study**

Rendering APIs now allow access to the hardware at a levle that was unseen before. The primary reason for this change is the demand from the gaming industry to provide a rendering SDK that gives more power and control to the graphics programmer. Vendor-specific driver implementations were often complex and imposed a CPU performance overhead that the developer had no control over. 

Morever the most famous of the new features added to DirectX 12 are **Mesh Shaders, Variable Rate Shading** and **Real Time Raytracing.** Setting up the DirectX 12 Pipeline allows access to exploration into these features that were a hot topic of discussion for the past decade and will continue to be so further.

For the purposes of this Directed Focus Study the major goal is to setup the DirectX 12 pipeline with stretch goal of setting up and testing the Raytracing pipeline.

----

## **Unique Features**
### Major Features - 
1. Creation of DirectX 12 Device
2. Creation of Deferred Contexts from an object pool
3. Creation of Swapchain
4. Creation of Fences, Command Queues, Command Lists and Events
5. Creation of Resource Bindings for swapchain
6. Binding the Swapchain
7. Clearing the Screen to a solid Single non-Black color
8. Creation of Fences, Command Lists and Command Queue bindings for buffers
9. Creating Vertex and Pixel Shader buffers
10. Rendering a traingle on the screen via a hard coded shader
11. Creating command buffers, lists and queues for Draw calls
12. Rendering a Quad on the screen in unlit mode
13. Creating Resource Bindings for Textures
14. Creating Texture Resources
15. Creating and Binding Textures
16. Rendering a Textured Quad
17. Testing a Pre-existing applications like Starship, Incursion, etc to run with the new Renderer
18. Refactoring and Testing for Dot3 Lighting as obtained from the old DirectX 11 Engine
19. Refactoring and Testing for Blinn Phong Lighting as obtained from the old DirectX 11 Engine
20. **_STRETCH_: -** Setting up the DirectX Raytracing (DXR) pipeline
20. **_STRETCH_: -** Testing the Raytracing pipeline with a single oriented cube on a plane

---

### Minor Features - 
 * GPU Synchronization - In DirectX 12 the graphics programmer must perform explicit GPU synchronization.
 * Fence - The Fence object is used to synchronize commands issued to the Command Queue.
 * Command List - A Command List is used to issue copy, compute (dispatch), or draw commands.
 * Command Queue - the commands in a command list are only run on the GPU after they have been executed on a command queue.
* [Memory Management](https://docs.microsoft.com/en-us/windows/win32/direct3d12/memory-management) - synchronization and management of memory residency. Managing memory residency means even more synchronization must be done.
* [Resource Binding](https://docs.microsoft.com/en-us/windows/win32/direct3d12/resource-binding) - resources such as textures, shaders, etc are not binded on the PGU as easily with a simple bind call and requires descriptor, descriptor tables, descriptor heaps, and a root signature.
* [Performance](https://docs.microsoft.com/en-us/windows/win32/direct3d12/performance-measurement) - with such levels of complexity and control it can be easy to do something inefficiently just to get it working first and then performing the necessary optimizations for overhead reduction and state change minimalization on the GPU.

----

## **Supported Platforms**
### **Windows 10**
#### **Prerequisites**
[**Visual Studio 2019 Community**](https://visualstudio.microsoft.com/downloads/) - **make sure you install the Game Development with C++ workload as shown in the following image.**

![**Game Development with C++ Workload**](https://www.3dgep.com/wp-content/uploads/2017/11/DirectX12Workload-1-768x480.png)

![** Visual Studio In built Graphics Debugger Module**](https://docs.unity3d.com/uploads/Main/InstallingGraphicsDebugger.jpg)

[**DirectX 12**]() - Since DirectX 12 only runs on the Windows 10 operating system, you must have access to a computer running Windows 10 in order to run the demo. The DirectX 12 SDK comes included with the Windows 10 SDK which is part of the Visual Studio installation.

[**HLSL Tools for Visual Studio**](https://marketplace.visualstudio.com/items?itemName=TimGJones.HLSLToolsforVisualStudio) - a free Visual studio plugin that enhancing exerience when working with HLSL shader files in visual studio. **This is a recommended plugin and not mandatory.**

---

_**NOTE: -**_ **In case you are unable to build and run or simply just running the program fails try the following** - 
* Press **Windows + R**
* Type **dxdiag**
* Then you should be able to **see** which **version of direct X** is installed on your computer. It should be **enabled if it is installed on your computer**. You might consider upgrading your hardware. 
* **You might also need to turn on graphics acceleration.**
* In case all of the above give the expected results please **open an issue** with the **exact error code and message along with a screenshot**.
----

### **Specific Hardware Setup**

#### **Development and Testing Hardware -** 
* **Processor:** Intel Core i7-9750H
* **Memory:** 32 GB DDR4
* **Video Card:** 8GB NVIDIA GeForce RTX 2070
* **Operating System:** Windows 10
* **Software:** Visual Studio 2019 COmmunity Edition IDE

----

## **Building**

**Method 1(for new users): -**

	1. Extract the zip folder to you desired location.
	2. Open the following path --> …\SD\Starship\Run 
	3. Double-click Starship_x64.exe to start the program.

**Method 2(for advanced users):-**

	1. Extract the zip folder to you desired location.
	2. Open the following path --> …\SD\Starship
	3. Open the Starship.sln using Visual Studio 2019.
 	4. make sure to st the build configuration to release.
	5. Press F7 key to build solution or go to Build --> Build Solution option using the Menu bar.
	6. Press Ctrl + F5 key to start the program without debugging or go to Debug --> Start without Debugging option using the Menu bar.

	NOTE:- 	
		* Visual Studio 2019 must be installed on the system.
		* In step 6 mentioned above, if you want you can execute the program with the debugger too.

----

## Debugging
* Never attach to running process since there may be errors at startup itself.
* Before using the below tools check the unit tests first.

### **Debugging Tools**
* [**RenderDoc**](https://renderdoc.org/) - it's a great tool for debugging graphics since we are in the development stage. 
* If you are unaware on **how to use RenderDoc** here are some basic videos for help - 
    * [**How to use renderDoc tool**](https://www.youtube.com/watch?v=ngz4NHiigIw)
* If you prefer to use the in built microsoft graphics debugger here are some helpful links - 
  * [**Visual Studio Graphics Diagnostics**](https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2015/debugger/visual-studio-graphics-diagnostics?view=vs-2015)
  * [**HLSL Shader Debugger**](https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2015/debugger/hlsl-shader-debugger?view=vs-2015)
  * [DirectX Graphics Debugging Tools](https://channel9.msdn.com/Events/Build/2013/3-141)


_**NOTE: -**_  
1. _The above mentioned links and the interface may differ due to version difference or the links being outdated._
2. _If you don't want to use any of the above mentioned tools you can always **write some debug visualisation shaders.** [**Here's a helpful link**](https://unitylist.com/p/q8p/Shader-s) read and understand the logic as syntax may differ a little bit._

----

## **Performance**

* [**Nvidia N-Sight**](https://developer.nvidia.com/nsight-visual-studio-edition) - A sampling based industry standard profiler developed by Nvidia. You can learn more about this on the internet.
* [**Tracy Profiler**](https://github.com/wolfpld/tracy) - You may think of Tracy as the [**RAD Telemetry**](http://www.radgametools.com/telemetry.htm) plus [**Intel VTune**](https://software.intel.com/content/www/us/en/develop/tools/oneapi/components/vtune-profiler.html), on overdrive.

----

## Testing
Simply Building the test case solution file should work but even if all the test are succeful there are quite a few tests that will have to hand checked and that's just because it's faster and more time effecient that way most test cases would only be checking for DirectX object creation and destruction. This is also a way to keep track of memory leaks.

---