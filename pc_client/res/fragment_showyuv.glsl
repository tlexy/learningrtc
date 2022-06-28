#version 450 core

out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
 
void main()
{
vec3 yuv;
vec3 rgb;
 
yuv.x = texture2D(texY, TexCoord).r;
yuv.y = texture2D(texU, TexCoord).r-0.5;
yuv.z = texture2D(texV, TexCoord).r-0.5;
 
//rgb = mat3(1.0, 1.0, 1.0,
//0.0, -0.3455, 1.779,
//1.4075, -0.7169, 0.0)*yuv;
rgb = mat3(1.0, 1.0, 1.0,
			0.0, -0.39465, 2.03211,
			1.13983, -0.58060, 0.0) * yuv;
 
FragColor = vec4(rgb, 1.0);
}