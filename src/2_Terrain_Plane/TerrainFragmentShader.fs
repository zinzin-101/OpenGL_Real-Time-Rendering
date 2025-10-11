#version 330 core

out vec4 FragColor;
in float height;

void main()
{
    FragColor = vec4(height, height, height, 1.0); // set all 4 vector values to 1.0
}