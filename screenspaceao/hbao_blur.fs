#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D hbaoInput;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(hbaoInput, 0));
    
    // We'll use a simple box filter for blurring.
    // You can also experiment with more sophisticated filters like a Gaussian blur.
    float result = 0.0;
    
    // The kernel size is set to 4x4.
    // You could parameterize this and adjust the loop to try different blur sizes.
    
    for (int x = -2; x <= 2; ++x) 
    {
        for (int y = -2; y <= 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(hbaoInput, TexCoords + offset).r;
        }
    }

    // Kernel size: (2*2 + 1) * (2*2 + 1) = 25 texel samples
    FragColor = result / 25.0;
}  