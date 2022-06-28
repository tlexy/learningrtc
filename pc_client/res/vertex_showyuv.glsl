#version 450 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec2 aTexCoord;
 
out vec2 TexCoord;
 
void main()
{
gl_Position = aPos;
TexCoord = aTexCoord;
}