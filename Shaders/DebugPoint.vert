////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

in vec2			aPosition;

uniform mat4	uWorldViewPerspective;
uniform vec3	uPos;
uniform float   uSize;

void main(void) {

	vec3 world_pos = vec3(aPosition * uSize, 0.0) + uPos;
    gl_Position = uWorldViewPerspective * vec4(world_pos.x, world_pos.y, world_pos.z, 1.0);
}
