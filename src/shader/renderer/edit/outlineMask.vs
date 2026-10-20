#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

invariant gl_Position;

void main()
{
    // Match geometry.vs: matrix reassociation introduces depth differences on
    // coplanar surfaces, punching flickering holes into the selection mask.
    vec4 WorldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * WorldPos;
}
