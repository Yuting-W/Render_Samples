#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace; //shadow------------------------------------------------------------

// material
uniform sampler2D texture_albedo;
uniform sampler2D texture_normal;
uniform sampler2D texture_metalness;
uniform sampler2D texture_roughness;
uniform sampler2D texture_emission;
uniform sampler2D texture_ambient_occlusion;
uniform sampler2D texture_specular;
uniform samplerCube irradianceMap;// IBL
uniform sampler2D shadowMap;  //shadow------------------------------------------------------------

uniform int has_texture_albedo;
uniform int has_texture_normal;
uniform int has_texture_metalness;
uniform int has_texture_roughness;
uniform int has_texture_emission;
uniform int has_texture_ambient_occlusion;
uniform int has_texture_specular;

uniform float ao;


//light -- 4�����Դ
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform vec3 DirectLightPos;  //shadow------------------------------------------------------------
uniform vec3 camPos;
uniform float lightArea;
//
const float PI = 3.14159265359;
//----------------   BRDF-FGD-------------------//
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float k);
float GeometrySmith(vec3 LightVec, vec3 camVec, vec3 normalVec, float roughness);
vec3 fresnelSchlick(float HdotN, vec3 F0);

//is shadowed?
float ShadowCalculation(vec4 fragPosLightSpace, float bias);
void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);
    vec3 R = reflect(-V, N); 
    vec3 diffuseColor = texture(texture_albedo, TexCoords).rgb;
    vec3 specularColor = texture(texture_specular, TexCoords).rgb;
    float colorDiff = length(specularColor - diffuseColor);
    float metallic = 1.0 - smoothstep(0.1, 0.3, colorDiff);
    //metallic = 0.0;
    float specularIntensity = dot(specularColor, vec3(0.2126, 0.7152, 0.0722));
    float roughness = 1.0 - specularIntensity;
    roughness = pow(roughness, 2.0); // ��ǿ�Աȶ�

    vec3 albedo = 5.0 * mix(diffuseColor, specularColor, metallic);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4 ; i++)
    {
        vec3 L = normalize(lightPositions[i] - FragPos);
        vec3 H = normalize(L + V);

        //calculate per-light radiance
        float distance = length(lightPositions[i] - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // compute BRDF
        float NDF = DistributionGGX(N, H, roughness);
        
        float G = GeometrySmith(L, V, N, roughness);
        float HdotN = max(0.0, dot(H, N));
        vec3 F = fresnelSchlick(HdotN, F0);

        vec3  numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N,L), 0.0) * max(dot(N,V), 0.0) + 0.0001;// + 0.0001 to prevent divide by zero
        vec3  specular = numerator/denominator;

        vec3 Ks = F; // kS is equal to Fresnel
        vec3 Kd = vec3(1.0) - Ks;

        Kd *= (1 - metallic);
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);   
        if(i == 2)
        {
            float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
            float shadow = ShadowCalculation(FragPosLightSpace,bias);  
            Lo += (1 - shadow) * (Kd * albedo / PI + specular) * radiance * NdotL;
        }
        else
        {
            Lo += (Kd * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again 
        }
        


    }
    // ambient lighting (we now use IBL as the ambient term)
    vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    //vec3 ambient = (kD * diffuse) * ao;

    // this ambient lighting with environment lighting
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient +  Lo;
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
   
}

//----------------   BRDF-FGD-------------------//
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a  * a ;
    float NdotH = max(dot(N,H), 0.0 );
    float NdotH2 = NdotH *NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom *denom;

    return a2/denom;
}
float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV/(NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 LightVec, vec3 camVec, vec3 normalVec, float roughness)
{
    float a = (roughness + 1.0);
    float k = a * a / 8.0;
    //vec3 h = (LightVec + normalVec)/2.0;
    float NdotL = max(0.0, dot(LightVec,normalVec));
    float ggx1 = GeometrySchlickGGX(NdotL, k);
    //vec3 h = (camVec + normalVec)/2.0;
    float NdotV = max(0.0, dot(camVec,normalVec));
    float ggx2 = GeometrySchlickGGX(NdotV, k);
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float HdotN, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotN, 0.0, 1.0), 5.0); //1 - HdotN �� [0,1] 
}

//is shadowed?
float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate the average blocker depth
    float samples = 3.0;
    float lightSizeUV  = lightArea; // 30 * 0.5;
    float blockArea =  lightSizeUV * (currentDepth - 0.05)/ currentDepth;
    float offset = blockArea;
    float averageDepth = 0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int currentSamples = 0;
    for(float x = -offset; x < offset; x += offset / samples)
    {
        for(float y = -offset; y < offset; y += offset / samples)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            if(pcfDepth < currentDepth - bias)
            {
                averageDepth += pcfDepth;     
                currentSamples++;
            }
               
        }    
    }
    averageDepth = currentSamples > 0 ? (averageDepth/currentSamples) : -1.0;
    // check whether current frag pos is in shadow
    
    float shadow;
    samples = 3.0;
    offset = (averageDepth < 0)? -1.0:((currentDepth - averageDepth)/averageDepth * lightSizeUV);
    
    if(projCoords.z > 1.0 || offset < 0.0)
    {
        shadow = 0.0;
    }    
    else
    {
        currentSamples = 0;
         for(float x = -offset; x < offset; x += offset / samples)
        {
            for(float y = -offset; y < offset; y += offset / samples)
            {
                vec2 sampleUV = projCoords.xy + vec2(x, y)* texelSize;
                
                float pcfDepth = texture(shadowMap, sampleUV).r; 

                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;      
                currentSamples++;
            }    
        }
        shadow /= currentSamples;
    }
    
    
    return shadow;
}


