#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal; 

uniform mat4 transform;
uniform mat4 projection;


void main()
{
    gl_Position = projection * transform * vec4(aPos, 1.0f);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);

    // Normale fissa per i quad 2D (guardano la camera)
    Normal = vec3(0.0, 0.0, 1.0);
}