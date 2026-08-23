#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;

// texture samplers
uniform sampler2D texture1;
uniform bool useColor;
uniform vec3 solidColor;

uniform bool useLighting;
uniform vec3 lightDir;
uniform vec3 lightColor;

void main()
{
    vec4 baseColor;

    if (useColor)
        baseColor = vec4(solidColor, 1.0);
    else
        baseColor = texture(texture1, TexCoord);


    if (!useLighting) {
        FragColor = baseColor;
        return;
    }

    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);

    float diff = max(dot(N, L), 0.0);
    vec3 ambient = 0.3 * lightColor;

    vec3 lit = (ambient + diff * lightColor) * baseColor.rgb;
    FragColor = vec4(lit, baseColor.a);
}