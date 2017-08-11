#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 0) out vec2 outUv;

layout (binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 vulkanNDC;
	mat4 mvp;
	vec3 camPos;
	float roughness;
}ubo;

void main() 
{
	gl_Position = ubo.vulkanNDC * vec4(inPos.xyz, 1.0);
	gl_Position.z = -gl_Position.w;
	outUv = inUv;
}