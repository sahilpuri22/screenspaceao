#version 330 core
in vec2 TexCoords;
out float FragColor;

// Uniforms with the information needed for HBAO
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise; // Noise texture used to randomize the kernel sample directions
uniform mat4 projection; // Projection matrix for clip-space conversion
uniform vec3 samples[64]; // Sample kernel points

const int kernelSize = 64;
const float radius = 0.1; // HBAO sampling radius
const float bias = 0.025; // HBAO bias term

const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0);


// Function to reconstruct view-space position from a depth value
vec3 ViewSpacePositionFromDepth(float depth, vec2 texCoords, mat4 invProjectionMatrix) {
    vec4 clipSpacePosition = vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = invProjectionMatrix * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    return viewSpacePosition.xyz;
}

// TBN matrix construction to transform samples from tangent-space to view-space
mat3 TBNMatrix(vec3 normal, vec3 texNoiseVec) {
    vec3 tangent = normalize(texNoiseVec - normal * dot(texNoiseVec, normal));
    vec3 bitangent = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}

// The HBAO occlusion calculation based on horizon angles
float ComputeHBAOOcclusion(vec3 origin, vec3 direction, float radius, vec3 normal, mat4 projection) {
    float occlusion = 0.0f;
    float totalWeight = 0.0f;

    for (int i = 0; i < kernelSize; ++i) {
        // Transform the sample point from tangent space to view space
        vec3 texNoiseVec = texture(texNoise, TexCoords * noiseScale).rgb;
        vec3 sample = TBNMatrix(normal, texNoiseVec) * samples[i] * radius + origin;

        // Convert the sample point to clip space coordinates
        vec4 sampleClipSpace = projection * vec4(sample, 1.0);
        sampleClipSpace.xyz /= sampleClipSpace.w;

        // Convert to texture coordinates and fetch the depth of the sample point from the gPosition texture
        vec2 sampleTexCoord = sampleClipSpace.xy * 0.5 + 0.5;
        float sampleDepth = texture(gPosition, sampleTexCoord).z;

        // If the sample point is behind the geometry, accumulate occlusion
        if (sampleDepth > origin.z + bias) {
            occlusion += 1.0f;
        }
        totalWeight += 1.0f; 
    }

    // Return the normalized occlusion factor
    return 1.0 - occlusion / totalWeight;
}

void main() {
    // Retrieve view-space position and normal for the current fragment
    float depth = texture(gPosition, TexCoords).r;
    vec3 position = ViewSpacePositionFromDepth(depth, TexCoords, inverse(projection));
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);

    // Fetch randomization vector from noise texture and scale by noise tile size
    vec3 randomVec = texture(texNoise, TexCoords * noiseScale).rgb;

    // Compute HBAO occlusion using the horizon-based occlusion function
    float occlusion = ComputeHBAOOcclusion(position, normal, radius, projection);

    // Write out the occlusion factor, to be used in the lighting pass
    FragColor = occlusion;
}