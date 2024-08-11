#version 330 core

/*
LearnOpenGL SSAO Implementation (de Vries, 2014)
https://learnopengl.com/Advanced-Lighting/SSAO
Based on Starcraft II SSAO (Filion and McNaughton, 2008)
https://www.scribd.com/document/632296500/Starcraft-2-Effects-Techniques
*/
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;


uniform vec3 samples[16];

// parameters 
uniform int kernelSize = 16;
uniform float radius = 1.3f;
uniform float bias = 0.025f;

// tile noise texture over screen based on screen dimensions divided by noise size
//const vec2 noiseScale = vec2(1920.0/4.0, 1080.0/4.0);

uniform mat4 projection;
 
void main()
{
     // Calculate noise scale based on the texture size
    vec2 screenSize = textureSize(gPosition, 0).xy;
    vec2 noiseSize = textureSize(texNoise, 0).xy;
    vec2 noiseScale = screenSize / noiseSize;

    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius;
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    FragColor = occlusion;
}