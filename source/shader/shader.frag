#version 450

layout(location = 0) in vec3 fragColour;

layout(location = 0) out vec4 outColour; //final output colour (must also have location) indicates which attachment we are outputting to.

void main()
{
    outColour = vec4(fragColour, 1.0f);
}