#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define NUM_OF_SINE_WAVES 4

uniform float time;
uniform float amplitude[NUM_OF_SINE_WAVES];
uniform float wavelength[NUM_OF_SINE_WAVES];
uniform vec3 direction[NUM_OF_SINE_WAVES];
uniform float speed[NUM_OF_SINE_WAVES];

void main()
{
    vec3 pos = aPos;
    
    float height = 0.0;
    float dx = 0.0f;
    float dz = 0.0f;

    for (int i = 0 ; i < NUM_OF_SINE_WAVES; i++){
        vec3 dir = normalize(direction[i]);
        float frequency = 2.0 / wavelength[i];
        float phase = speed[i] * (2.0 / wavelength[i]);
        height += amplitude[i] * sin((dir.x * pos.x + dir.z * pos.z) * frequency + time * phase);
        dx += frequency * dir.x * amplitude[i] * cos((dir.x * pos.x + dir.z * pos.z) * frequency + time * phase);
        dz += frequency * dir.z * amplitude[i] * cos((dir.x * pos.x + dir.z * pos.z) * frequency + time * phase);
    }

    vec3 normal = normalize(vec3(-dx, 1.0, -dz));
    pos.y = height;
    FragPos = vec3(model * vec4(pos, 1.0));
    //Normal = mat3(transpose(inverse(model))) * aNormal;
    Normal = mat3(transpose(inverse(model))) * normal;
    //Normal = mat3(transpose(inverse(model))) * vec3(0,1,0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}