#version 450    //version of GLSL (4.5)

layout(location = 0) out vec3 fragColour; //output colour for vertex (location is required)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

void main()
{
    gl_Position = vec4(position.x,-position.y,position.z, 1.0f);
    fragColour = color;
}