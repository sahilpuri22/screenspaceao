#version 330 core

/*
Used LearnOpenGL SSAO implementation as a base (de Vries, 2014)
https://learnopengl.com/Advanced-Lighting/SSAO
SSAO Alchemy implementation done by Sahil Puri using
Alchemy Screen Space Ambeint Obscurance Algorithm (McGuire, 2011)
https://casual-effects.com/research/McGuire2011AlchemyAO/VV11AlchemyAO.pdf
*/
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

// Parameters 
uniform int kernelSize = 16;
uniform float radius = 1.7f; // Constant radius in world space
uniform float bias = 0.025f;
uniform float sigma = 1.7f; // Strength multiplier
uniform int k = 1; // Contrast multiplier
uniform float beta = 0.5f; // Shadow Bias
uniform float turns = 1.0f; // Turns parameter for sampling distribution
const float epsilon = 0.001f; // Avoids divide by zero

// Tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0 / 4.0, 600.0 / 4.0);

uniform mat4 projection;

const float PI = 3.14159265359;

// Generates a point on a disk
vec2 DiskPoint(float sampleRadius, float x, float y, float turns)
{
    float r = sampleRadius * sqrt(x); 
    float theta = y * (2.0 * PI) * turns; 
    return vec2(r * cos(theta), r * sin(theta));
}

// Generates a pseudorandom 2D vector based on a single float input
vec2 RandomHashValue(float randomValue)
{
    return fract(sin(vec2(randomValue, randomValue + 0.1)) * vec2(12.9898, 78.233)); //Commonly used pseudo-random number generator used for shaders
}

void main()
{
    // Random value updating based on screen coordinates
    float RANDOMVALUE = (TexCoords.x * TexCoords.y) * 64.0;

    // Normals and positions in view-space
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    // Create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float ao = 0.0;
    float screen_radius = radius * 0.75 / fragPos.z; // Ball around the point

    for (int i = 0; i < kernelSize; ++i)
    {
        vec2 RandomValue = RandomHashValue(RANDOMVALUE + float(i));
        vec2 disk = DiskPoint(1.0, RandomValue.x, RandomValue.y, turns);
        vec2 samplepos = TexCoords + (disk.xy) * screen_radius;

        // Check samplepos is within the texture coordinates range
        if (samplepos.x < 0.0 || samplepos.x > 1.0 || samplepos.y < 0.0 || samplepos.y > 1.0)
            continue;
        
        // Sample position
        vec3 samplePos = texture(gPosition, samplepos).xyz; 
        vec3 V = samplePos - fragPos;
        float distance = length(V);
        float rangeCheck = smoothstep(0.0, radius, distance);
        
        // Alchemy AO calculation
        float occlusionFactor = max(0.0, dot(V, normal + fragPos.z * beta)) / (dot(V, V) + epsilon);
        occlusionFactor *= rangeCheck;
        
        ao += occlusionFactor;
    }

    // Normalize AO
    ao = max(0.0, 1.0 - (2.0 * sigma / float(kernelSize)) * ao);
    ao = pow(ao, float(k));


    // Output AO value
    FragColor = ao;
}
