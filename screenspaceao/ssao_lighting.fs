#version 330 core

/*
LearnOpenGL SSAO Lighting (de Vries, 2014)
https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao_lighting.fs
*/

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

struct Light {
    vec3 Position; // Must be in world space
    vec3 Color;
    float Linear;
    float Quadratic;
};
const int NR_LIGHTS = 14;
uniform Light lights[NR_LIGHTS];

uniform vec3 viewPos; // Camera's world-space position
uniform mat4 invView;

void main() {             
    // retrieve data from gbuffer
    vec3 FragPosView = texture(gPosition, TexCoords).rgb; // In view space
    vec3 FragPos = (invView * vec4(FragPosView, 1.0)).xyz; // In world space
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb); // In world space
    vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    float brightnessFactor = 1.2;

    // calculate lighting as usual
    vec3 ambient = vec3(0.5 * Diffuse * AmbientOcclusion); // Static ambient component
    vec3 totalLighting = ambient;

    vec3 viewDir  = normalize(viewPos- FragPos); // Correct view direction calculation

    for (int i = 0; i < NR_LIGHTS; ++i) {
        vec3 lightDir = normalize(lights[i].Position - FragPos); // Calculate the direction of each light from the fragment

        // Diffuse
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;

        // Specular - placeholder for Phong/Blinn-Phong model
        // Assuming you have the specular component calculation not shown
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
        vec3 specular = lights[i].Color * spec;

        // Attenuation
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);

        // Adding the light's impact to the total lighting
        diffuse *= attenuation;
        specular *= attenuation;

        totalLighting += diffuse + specular;
    }

    FragColor = brightnessFactor*vec4(totalLighting, 1.0);
}