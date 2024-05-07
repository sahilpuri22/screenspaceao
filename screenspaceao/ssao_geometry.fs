#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D textureDiffuse1;

uniform bool useTexture;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal);

    if (useTexture) {
        gAlbedo = texture(textureDiffuse1, TexCoords);
    } 
    else {
        gAlbedo = vec4(1, 1, 1, 1.0);
    }

}