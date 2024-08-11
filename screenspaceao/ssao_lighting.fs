#version 330 core

/*
LearnOpenGL SSAO Lighting (de Vries, 2014)
https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao_lighting.fs
Adjusted for the needs of this application such as the addition of directional light
*/

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

struct DirectionalLight {
    vec3 Direction;
    vec3 Color;
};
uniform DirectionalLight dirLight;


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
    vec3 FragPosView = texture(gPosition, TexCoords).rgb; 
    vec3 FragPos = (invView * vec4(FragPosView, 1.0)).xyz; 
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb); 
    vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    float brightnessFactor = 1.2;

    // calculate ambient lighting
    vec3 ambient = vec3(0.5 * Diffuse * AmbientOcclusion); 
    vec3 totalLighting = ambient;

    // calculate directional light contribution
    vec3 viewDir  = normalize(viewPos - FragPos); 
    vec3 lightDir = normalize(-dirLight.Direction); 

    // Diffuse component
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * Diffuse * dirLight.Color;

    // Specular component
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0); 
    vec3 specular = dirLight.Color * spec;

    // Add directional light contribution to total lighting
    totalLighting += diffuse + specular;

    // calculate point lights contribution
    for (int i = 0; i < NR_LIGHTS; ++i) {
        vec3 lightDir = normalize(lights[i].Position - FragPos); 

        // Diffuse
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;

        // Specular
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

    FragColor = brightnessFactor * vec4(totalLighting, 1.0);
}
