#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float phase;
uniform float amplitude;
uniform float frequency;

void main()
{
    vec3 pos = aPos;

    pos.y = amplitude * sin(pos.x * frequency + phase);
    pos.y += amplitude * sin(pos.z * frequency + phase);

    FragPos = vec3(model * vec4(pos, 1.0));
    //Normal = mat3(transpose(inverse(model))) * aNormal;
    Normal = mat3(transpose(inverse(model))) * vec3(0, 1, 0);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}