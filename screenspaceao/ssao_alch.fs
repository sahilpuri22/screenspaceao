#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[16];

// parameters 
uniform int kernelSize = 16;
uniform float radius = 2.0f;
uniform float bias = 0.025f;

uniform float sigma = 0.004f; // example value, adjust as necessary
uniform int k = 1; // power factor in the SSAO algorithm
uniform float beta = 0.5f;

//uniform float ballRadiusScaling = 3500.0f; // Scaling factor


// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0 / 4.0, 600.0 / 4.0);

uniform mat4 projection;

void main()
{
    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // radius scaling
    //float depth = fragPos.z;
   // float scaledRadius = -radius * ballRadiusScaling / depth;

    
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i)
    {
        // Get sample position in view space
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius;
        //vec3 samplePos = fragPos + TBN * samples[i] * radius;
        
        // Project sample position to screen space
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(gPosition, offset.xy).z;
        
        vec3 v = samplePos - fragPos; // Vector from fragment to sample
        //float v_dot_normal = dot(v, normal); // Dot product with normal
        
        // Depth difference and range check
        float depthDiff = sampleDepth - fragPos.z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(depthDiff));
        
        // Range check & accumulate occlusion
        //float occlusionFactor = (v_dot_normal - bias) / (depthDiff * depthDiff + beta);
        float occlusionFactor = max(0.0, dot(v, normal + samplePos.z * beta) / (dot(v, v) + 0.01));
        occlusion += occlusionFactor;// * rangeCheck;
    }
    
    // Normalize occlusion
    occlusion = max(0, 1.f - 2.f * sigma / kernelSize * occlusion);
    occlusion = pow(occlusion, k);
    //occlusion = 1.0 - occlusion;

    FragColor = occlusion;
}
