#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D hbaoInput;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(hbaoInput, 0));

    float result = 0.0;
    

    
    for (int x = -2; x <= 2; ++x) 
    {
        for (int y = -2; y <= 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(hbaoInput, TexCoords + offset).r;
        }
    }

    FragColor = result / 25.0;
}  