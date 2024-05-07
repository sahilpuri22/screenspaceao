#version 330 core


out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform float u_AORadius = 500000.f;
uniform float u_AOBias = 0.f;
uniform int u_AOSamples = 4;

uniform mat4 projection; // camera projection matrix
uniform mat4 view; // camera view matrix

const float INFINITY = 1.f/0.f;


float saturate(float a)
{
	return min(max(a,0),1);
}

vec2 calculateAO(vec3 normal, vec2 direction, vec2 screenSize, vec3 fragPos, float bias)
{	
	
	float minRadius = 3.f;
	float maxRadius = 100000.f;
	float RAD = length( direction*vec2(u_AORadius) / (vec2(abs(fragPos.z))*screenSize));
	RAD = clamp(RAD, minRadius, maxRadius);

	vec3 viewVector = normalize(fragPos);

	vec3 leftDirection = cross(viewVector, vec3(direction,0));
	vec3 projectedNormal = normal - dot(leftDirection, normal) * leftDirection;
	float projectedLen = length(projectedNormal);
	projectedNormal /= projectedLen;

	vec3 tangent = cross(projectedNormal, leftDirection);

	float tangentAngle = atan(tangent.z / length(tangent.xy));
	float sinTangentAngle = sin(tangentAngle + bias);
	

	vec2 texelSize = vec2(1.f,1.f) / screenSize;

	float highestZ = -INFINITY;
	vec3 foundPos = vec3(0,0,-INFINITY);
	
	for(int i=2; i<=4; i++) 
	{
		vec2 marchPosition = TexCoords + i*texelSize*direction;
		
		vec3 fragPosMarch = texture(gPosition, marchPosition).xyz;
		
		vec3 hVector = normalize(fragPosMarch-fragPos);

		float rangeCheck = 1 - saturate(length(fragPosMarch-fragPos) / RAD);

		hVector.z = mix(hVector.z, fragPos.z-RAD*2, rangeCheck);

		if(hVector.z > highestZ && length(fragPosMarch-fragPos) < RAD)
		{
			highestZ = hVector.z;
			foundPos = fragPosMarch;
		}
	}

	float rangeCheck = smoothstep(0.0, 1.0, 10*length(screenSize) / length(foundPos - fragPos));
	if(length(foundPos - fragPos) > length(screenSize)*100){rangeCheck=0;}

	vec3 horizonVector = (foundPos - fragPos);
	float horizonAngle = atan(horizonVector.z/length(horizonVector.xy));
	
	float sinHorizonAngle = sin(horizonAngle);	

	vec2 result = vec2(saturate((sinHorizonAngle - sinTangentAngle))/2, projectedLen);
	return result;
}

void main()
{
	vec2 screenSize = textureSize(gPosition, 0).xy;
	vec2 noiseScale = vec2(screenSize.x/4.0, screenSize.y/4.0);
	vec2 noisePos = TexCoords * noiseScale;

	vec3 fragPos   = texture(gPosition, TexCoords).xyz;
	float bias = (3.141592 / 360) * u_AOBias;
	if(fragPos.z == -INFINITY){FragColor = 1; return;}

    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);

	vec2 randomVec = normalize(texture2D(texNoise, noisePos).xy); 

	vec2 result = vec2(0,0);

	vec3 viewVector = normalize(fragPos);

	result += calculateAO(normal, vec2(randomVec), screenSize, fragPos,bias);
	result += calculateAO(normal, -vec2(randomVec), screenSize, fragPos, bias);
	result += calculateAO(normal, vec2(-randomVec.y,randomVec.x), screenSize, fragPos, bias);
	result += calculateAO(normal, vec2(randomVec.y, -randomVec.x), screenSize, fragPos, bias);
	
	result.x /= result.y;

	float darknessFactor = 1.2; 
    result.x *= darknessFactor;
	
	FragColor = 1 - result.x;
	
}