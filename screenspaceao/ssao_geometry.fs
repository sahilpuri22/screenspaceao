#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// Uniform sampler for the diffuse texture
uniform sampler2D textureDiffuse1;

uniform bool useTexture;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);

    if (useTexture) {
        // Sample the color from the diffuse texture
        gAlbedo = texture(textureDiffuse1, TexCoords);
    } 
    else {
        // Use a default color (e.g., gray) when texturing is disabled
        gAlbedo = vec4(1, 1, 1, 1.0);
    }

}