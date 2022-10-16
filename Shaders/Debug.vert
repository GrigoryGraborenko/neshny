////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

in vec2			aPosition;

uniform mat4	uWorldViewPerspective;
uniform vec3	uPosA;
uniform vec3	uPosB;

void main(void) {

	vec3 world_pos = aPosition.x * uPosA + aPosition.y * uPosB;
    gl_Position = uWorldViewPerspective * vec4(world_pos.x, world_pos.y, world_pos.z, 1.0);
}
