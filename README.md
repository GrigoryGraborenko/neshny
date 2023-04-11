# Neshny
Neshny is an OpenGL/C++ library for games and simulations. It is designed to be unobtrusive - use as little or as much of it as you wish. It is not an engine, rather a tool to help make writing your own engine easier. The core feature is an entity system that runs mostly on the graphics card. There is an optional UI that can overlay your window and display valuable debugging information.

Currently it has dependencies on Qt >= v5.15, Dear ImGui >= v1.88, and Metastuff. There is an optional dependency on SDL as well for some features.

## Installation
Clone this repo to a location of your choice:
```
git clone https://github.com/GrigoryGraborenko/neshny.git
```
### Using cmake
Pick one of the starter projects in the **Examples** directory - for example, the **EmptyQT+SDLJumboBuild** will create a near-empty cmake based project optimized for Visual Studio. It also sets up a pattern for Jumbo builds - this is highly recommended, as this reduces all compilation speeds drastically down to seconds. 

Make a copy of the entire directory and place it wherever you like. Ensure you have cmake 3.18 or greater installed.

Edit the **UserSettings.cmake** to point to the correct directories where libraries are installed:
``` cmake
set(NESHNY_DIR "C:/Code/Neshny")
set(QT_DIR "C:/Qt/5.15.2/msvc2019_64")
set(SDL2_DIR "C:/SDL/SDL2-2.0.14")
set(SDL2_MIXER_DIR "C:/SDL/SDL2_mixer-2.0.4")
set(SHADER_PATH "src/shaders")
```
Also make sure to edit this line near the top of CMakeLists.txt:
```
set(PROJECT_NAME "EmptyQTJumboBuild") # change to your project name
```
Then navigate to the root of the copied project and run this command in the terminal:
``` 
cmake -G "Visual Studio 16 2019" -A x64 .
```
Replace `"Visual Studio 16 2019"` with whatever compiler you prefer. This will generate all the project files needed. Then open up the solution/project files and run it. You should see this:
![Screenshot of Empty QT SDL Project](/Documentation/empty_qt_sdl_jumbo.png)


### Manually
Clone this repo and point to the base dir in your include directories list. Then add
``` C++
// put this in your headers
#include <IncludeAll.h> // Neshny
// put this in your main.cpp or similar
#include <IncludeAll.cpp> // Neshny
using namespace Neshny;
```
This assumes you already have QT, ImGui and Metastuff installed.

## Features
### Viewing the editor overlay
Assuming you already have ImGui installed, call these functions each render cycle, preferrably near the end of the cycle:
``` C++
if (show_editor) {
    // optional rendering of debug visuals
    DebugRender::Render3DDebug(view_perspective_matrix, width, height);
    // reset for per-cycle debugging visuals
    DebugRender::Clear();
    // this will do all UI/UX for the editor
    Core::RenderEditor();
}
```
### Shader and buffer convenience classes
``` C++
GLShader shader;
QString err_msg;
// takes error string ref, function that loads the actual file given a path,
// and three paths for vertex, fragment and geometry shaders
// geometry shader is optional and can be left empty
shader.Init(err_msg, [] (QString path, QString& err_msg) -> QByteArray {
		QFile file(QString("shader_prefix\\%1").arg(path));
		if (!file.isOpen()) {
			err_msg = "File error - " + file.errorString();
			return QByteArray();
		}
		return file.readAll();
}, "shader.vert", "shader.frag", "shader.geom", "uniform int extra_uniform;");
shader.UseProgram();
glUniform1i(shader.GetUniform("extra_uniform"), 123);

```
### Shader loading and viewing
Here's an example of using a rendering shader:
``` C++
// dynamically load the shader that corresponds to Fullscreen.vert and Fullscreen.frag
// loads from "../src/Shaders/", set by cmake var SHADER_PATH in UserSettings.cmake
GLShader* prog = Neshny::GetShader("Fullscreen");
prog->UseProgram();
glUniform1i(prog->GetUniform("uniform_name"), 123);

GLBuffer* buff = Neshny::GetBuffer("Square"); // get a built-in model
buff->UseBuffer(prog); // attach the program to the buffer
buff->Draw(); // executes the draw call
```
Here's how you would run a compute shader:
``` C++
// dynamically load the shader that corresponds to Compute.comp
// loads from "../src/Shaders/", set by cmake var SHADER_PATH in UserSettings.cmake
GLShader* compute_prog = Neshny::GetComputeShader("Compute");
compute_prog->UseProgram();
// runs 64 instances
// second var is result of multiplying local sizes together
// usually set by layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
// will call glDispatchCompute multiple times if it exceeds 64x512 instances
// automatically populates uCount and uOffset integer uniform
Neshny::DispatchMultiple(compute_prog, 64, 512);
// or you could manage it yourself:
// glDispatchCompute(4, 4, 4);
```
### GPU entities and pipelines

``` C++
#pragma pack(push)
#pragma pack (1) // padding will break the GPU transfer if you don't have this

struct GPUProjectile {
	int				p_Id; // all entities should have an integer ID
	fVec3		    p_Pos;
	fVec3	    	p_Vel;
	int				p_Type;
	int				p_State;
};

#pragma pack(pop)

// gotta define all struct members to ensure correct storage
namespace meta {
	template<>
	inline auto registerMembers<GPUProjectile>() {
		return members(
			member("Id", &GPUBoid::p_Id),
			member("Pos", &GPUBoid::p_Pos),
			member("Vel", &GPUBoid::p_Vel),
			member("Type", &GPUBoid::p_Type),
			member("State", &GPUBoid::p_State)
		);
	}
}

// store this somewhere permanent
GPUEntity projectiles(
    "Projectile", // name used for debugging
    GPUEntity::DeleteMode::MOVING_COMPACT, // no gaps but cannot trust an index to it, may be moved
    // DeleteMode::STABLE_WITH_GAPS will stay put so you can reference it
    // will have holes in the data structure though
    &GPUProjectile::p_Id, // pointer to ID member
    "Id" // name of ID member
);
// should be done when you have an active OpenGL context
// add in the max number of these entities you will EVER have
// for performance reasons this GPU memory will be pre-allocated
projectiles.Init(100000);

for (int i = 0; i < 10; i++) {

    GPUProjectile proj{
        0, // id doesn't matter, gets assigned by system
        fVec3(100.0, 110.0, 120.0),
        fVec3(1.0, 0.0, 0.0),
        1,
        123
    };
    m_Projectiles.AddInstance((float*)&proj);
}

PipelineStage::ModifyEntity(
    projectiles // GPU entity
    ,"ProjectileMove" // name of compute shader
    ,true // do you want the main function to be written for you?
    ,{ "MACRO_VALUE 123" } // will set up a #define of these items
)
.AddCreatableEntity(some_other_entity)
.AddEntity(read_only_entity)
.AddSSBO("buffer_name", data_buffer, MemberSpec::T_INT)
.Run([delta_seconds](GLShader* prog) { // optional lambda that gets called right before dispatch
    glUniform1f(prog->GetUniform("uDeltaSeconds"), delta_seconds);
});

// when you wanna render the item
PipelineStage::RenderEntity(
    projectiles // GPU entity
    ,"ProjectileRender" // name of .frag, .vert and optionally .geom shader
    ,false, // do you want the main vertex function to be written for you?
    ,Neshny::GetBuffer("Square") // buffer to render with
    { "DEFINITION 1234" }
)
.Run([&vp](GLShader* prog) {
    glUniformMatrix4fv(prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, vp.data());
});
```
This is what the ProjectileMove compute shader might look like:
``` GLSL

uniform float uDeltaSeconds;

// if you said true to the third arg, use this pattern:
bool ProjectileMain(int item_index, Projectile proj, inout Projectile new_proj) {
    // proj is the input entity
    // new_proj is a copy of the entity - changes you make to it will be saved

	new_proj.State = proj.State - 1;
    new_proj.Pos = proj.Pos + proj.Vel * uDeltaSeconds;
    new_proj.Vel.y -= 0.98; // add gravity

    if(new_proj.State < 0) {
        return true; // deletes the entity
    }
    return false; // it lives to fight another iteration
}

```

TODO
### Entity data viewer with time-travel
TODO
### Cameras
TODO
### Resource system
The resource system uses threads to load and initialize resources in the background. Calling a resource via `Neshny::GetResource` the first time will initiate the loading process - every subsequent call will return either `PENDING`, `IN_ERROR` or 'DONE' for `m_State` [TODO change m_ to p_]. Once it is in the `DONE` state it will be cached and immediately return a pointer to the resource in question. This is designed to be used with a functional-style loop, much like ImGui. The call itself should be lightweight and block the executing thread for near-zero overhead. Each tick you get the same resource for as long as you require it, and it may be several or even hundreds of ticks later that the resource is resolved.
``` C++
	auto tex = Core::GetResource<Texture2D>("../images/example.png");
    if(tex.IsValid()) {
        glBindTexture(GL_TEXTURE_2D, tex->Get().GetTexture());
    } else if(tex.m_State == ResourceState::PENDING) {
        // show some loading info
    } else if(tex.m_State == ResourceState::IN_ERROR) {
        // show info about error using tex.m_Error
    }
```
Currently there are resources for `SoundFile` (using SDL), `Texture2D`, `TextureTileset` and `TextureSkybox`. Creating your own resource type is easy - simply subclass from `Resource` and implement the abstract function `virtual bool Init(QString path, QString& err)` and the two memory estimate functions like so:
``` C++
class CustomResource : public Resource {
public:
    // no need for constructor, all the work is done in Init
	virtual				~CustomResource(void) {}

	virtual bool		Init(QString path, QString& err) {
		// anything here will be run in an offline thread
        // opengl resource creation here will be shared
        for(int i = 0; i < 1000000; i++) {
            // some slow process
            cached_value++;
        }
        if(!cached_value) {
            err = "Reason for failure";
            return false; // resource will be "IN_ERROR"
        }
        return true; // resource will be "DONE"
	};
	virtual qint64		GetMemoryEstimate		( void ) const { return 0; }
	virtual qint64		GetGPUMemoryEstimate	( void ) const { return 0; }

private:
    int cached_value = 0; // whatever payloads/objects you like
};

```
If you want to create a resource based off data in a file, you have the convenience base class `FileResource` which requires you to implement the `virtual bool FileInit(QString path, unsigned char* data, int length, QString& err)` function that is called after the file in the path is loaded. `data` and `length` contain the contents of the file if it has been successfully loaded.
### 3D debug visualizer
TODO
### Scrapbook
TODO
### Serialization
TODO