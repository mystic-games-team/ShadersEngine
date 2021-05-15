///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;


struct Light
{
    unsigned int    type;
    vec3            color;
    vec3            direction;
    vec3            position;
    float           intensity;
    float           linear;
    float           quadratic;
};


layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    uniform mat4 uWorldMatrix;
    uniform mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec4 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
    gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);

    vPosition = uWorldMatrix * vec4(aPosition, 1.0);
    vNormal = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
    vTexCoord = aTexCoord;
    
    vViewDir = uCameraPosition - vPosition.xyz;
} 

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
    unsigned int    type;
    vec3            color;
    vec3            direction;
    vec3            position;
    float           intensity;
    float           linear;
    float           quadratic;
};

in vec2 vTexCoord;
in vec3 vNormal;
in vec4 vPosition;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3            uCameraPosition;
    unsigned int    uLightCount;
    Light           uLight[16];
};

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oPosition;

vec3 CalculateDirectionalLight(Light light, vec3 vNormal, vec3 vViewDir) 
{
    vec3 lightDirection = normalize(-light.direction);
    vec3 diffuse = light.color *  max(dot(lightDirection, vNormal), 0.0);
    return (diffuse + diffuse * pow(max(dot(vNormal, normalize(lightDirection + vViewDir)), 0.0), 0.0) * 0.01) * light.intensity;
}

vec3 CalculatePointLight(Light light, vec3 vNormal, vec3 vPosition, vec3 vViewDir) 
{
    vec3 lightDirection = normalize(light.position - vPosition);
    float distance = length(light.position - vPosition);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * distance * distance);      
	return (light.color * max(dot(vNormal, lightDirection), 0.0) + light.color * pow(max(dot(vNormal, normalize(lightDirection + vViewDir)), 0.0), 14.0)) * light.intensity * attenuation;
}

void main()
{
    vec3 finalColor = vec3(0.0);
    for (int i = 0; i < uLightCount; ++i)
    {
        switch (uLight[i].type)
        {
            case 0:
                finalColor += CalculateDirectionalLight(uLight[i], vNormal, normalize(vViewDir));
            break;
            case 1:
                finalColor += CalculatePointLight(uLight[i], vNormal, vPosition.xyz, normalize(vViewDir));
            break;
        }
    }

    oColor = vec4(finalColor, 1.0) + texture(uTexture, vTexCoord) * 0.2;
    oNormals = vec4(vNormal, 1.0);
    oAlbedo = texture(uTexture, vTexCoord);
    oPosition = vPosition;

    gl_FragDepth = gl_FragCoord.z - 0.1;
}

#endif
#endif

//////////////

#ifdef DEF

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormals;
layout(location=2) in vec2 aTexCoord;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
};

layout(binding = 1, std140) uniform LocalParams
{
    uniform mat4 uWorldMatrix;
    uniform mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec4 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
    gl_Position = uWorldViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);

    vPosition = uWorldMatrix * vec4(aPosition, 1.0);
    vNormal = mat3(transpose(inverse(uWorldMatrix))) * aNormals;
    vTexCoord = aTexCoord;
    
    vViewDir = uCameraPosition - vPosition.xyz;
} 

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vNormal;
in vec4 vPosition;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3            uCameraPosition;
};

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oPosition;

void main() {

	oColor = texture(uTexture, vTexCoord);
    oNormals = vec4(vNormal, 1.0);
    oAlbedo = texture(uTexture, vTexCoord);
    oPosition = vPosition;

    gl_FragDepth = gl_FragCoord.z - 0.1;
}

#endif
#endif