# Neshny
Neshny is an OpenGL/C++ library for games and simulations. It is designed to be unobtrusive - use as little or as much of it as you wish. The core feature is an entity system that runs mostly on the graphics card.

Currently it has dependencies on Qt >= v5.15, Dear ImGui >= v1.88, and Metastuff.

## Installation
*This is a work in progress - later on there will be example projects and cmake files.*

Clone this repo and point to the base dir in your include directories list. Then add
```
// put this in your headers
#include <IncludeAll.h> // Neshny
// put this in your main.cpp or similar
#include <IncludeAll.cpp> // Neshny
```
Currently it assumes you already have QT, ImGui and Metastuff installed.

## Features

### Shader loading and viewing
Here's an example of using a rendering shader:
``` C++
// dynamically load the shader that corresponds to Fullscreen.vert and Fullscreen.frag
// loads from "../src/Shaders/" (TODO: hardcoded for now)
GLShader* prog = Neshny::GetShader("Fullscreen");
prog->UseProgram();
glUniform1i(prog->GetUniform("uniform_name"), 123);

GLBuffer* buff = Neshny::GetBuffer("Square"); // get a built-in model
buff->UseBuffer(prog); // attach the program to the buffer
buff->Draw(); // executes the draw call
```
Here's how you would run a compute shader:
``` C++
// dynamically load the shader that corresponds to Compute.glsl
// loads from "../src/Shaders/" (TODO: hardcoded for now)
GLShader* compute_prog = Neshny::GetComputeShader("Compute");
compute_prog->UseProgram();
// runs 64 instances
// will call multiple times if it exceeds 32768 instances
// automatically populates uCount and uOffset integer uniform
Neshny::DispatchMultiple(compute_prog, 64);
// or you could manage it yourself:
// glDispatchCompute(4, 4, 4);
```
### GPU entities and pipelines

``` C++
#pragma pack(push)
#pragma pack (1) // padding will break the GPU transfer if you don't have this

struct GPUProjectile {
	int				p_Id; // all entities should have an integer ID
	QVector3D		p_Pos;
	QVector3D		p_Vel;
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
    GPUEntity::StoreMode::SSBO, // or TEXTURE - how the data is stored
    GPUEntity::DeleteMode::MOVING_COMPACT, // no gaps but cannot trust an index to it, may be moved
    // DeleteMode::STABLE_WITH_GAPS will stay put so you can reference it
    // will have holes in the data structure though
    &GPUProjectile::p_Id, // pointer to ID member
    "Id" // name of ID member
);
// should be done when you have an active OpenGL context
projectiles.Init();

for (int i = 0; i < 10; i++) {

    GPUProjectile proj{
        0, // id doesn't matter, gets assigned by system
        QVector3D(100.0, 110.0, 120.0),
        QVector3D(1.0, 0.0, 0.0),
        1,
        123
    };
    m_Projectiles.AddInstance((float*)&proj);
}

// store these two somewhere permanent
GLSSBO control, deathList;
PipelineStage(
    projectiles
    ,"ProjectileMove" // name of compute shader
    ,true // do you want the main function to be written for you?
    ,{ "MACRO_VALUE 123" } // will set up a #define of these items
    ,&control
    ,&deathList
)
.AddCreatableEntity(some_other_entity)
.AddEntity(read_only_entity)
.AddSSBO("buffer_name", data_buffer, MemberSpec::T_INT)
.Run([this, delta_seconds](GLShader* prog) { // optional lambda that gets called right before dispatch
    glUniform1f(prog->GetUniform("uDeltaSeconds"), delta_seconds);
});

// when you wanna render the item
EntityRender(projectiles, "ProjectileRender", {})
.Render(Neshny::GetBuffer("Square"), [&vp](GLShader* prog) {
    glUniformMatrix4fv(prog->GetUniform("uWorldViewPerspective"), 1, GL_FALSE, vp.data());
});
```

This is what the ProjectileMove compute shader might look like:
``` GLSL
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

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
TODO
### 3D debug visualizer
TODO
### Scrapbook
TODO
### Serialization
TODO