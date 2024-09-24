////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

in vec2			aPosition;

uniform mat4	uWorldViewPerspective;
uniform vec3	uPosA;
uniform vec3	uPosB;
uniform vec3	uPosC;

out vec2		oUV;

void main(void) {

	vec3 world_pos = mix(mix(uPosA, uPosB, aPosition.x), uPosC, aPosition.y);
    gl_Position = uWorldViewPerspective * vec4(world_pos.x, world_pos.y, world_pos.z, 1.0);
    oUV = aPosition;
}
