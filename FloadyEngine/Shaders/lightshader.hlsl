
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct PSOutput
{
	float4 color;
};

Texture2D<float4> g_colortexture : register(t0);
Texture2D<float4> g_normaltexture : register(t1);
Texture2D<float> g_depthTexture : register(t2);
Texture2D<float> g_shadowTexture : register(t3);
SamplerState g_sampler : register(s0);

struct MyData
{
	float4x4 g_invProjMatrix;
	float4x4 g_ProjMatrixLight;
    float4 lightPos;
    float4 camPos;
};

ConstantBuffer<MyData> myData : register(b0);
float4 g_constData[4] : register(b1);

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD)
{
	float4 invProjMatrix = g_constData[0];
	PSInput result;

	result.position = float4(position, 1.0f);
	result.uv = uv;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	float4x4 invProjMatrix = myData.g_invProjMatrix;
    const float3 lightPos = float3(8.0, 0.0, 3.0f);
	PSOutput output;
	
	float4 colors = g_colortexture.Sample(g_sampler, input.uv);
	float4 normals = g_normaltexture.Sample(g_sampler, input.uv);
	normals = normals * 2.0f;
	normals = normals - float4(1,1,1,1);
	
	// get linear depth
	float depth = g_depthTexture.Sample(g_sampler, input.uv);
	float zNear = 100.0;
	float zFar = 1.0;

	float4 H = float4(input.uv.x * 2 - 1, (1.0f - input.uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, invProjMatrix); 
	float4 worldPos = D / D.w;
 
	float4 projShadowMapPos = mul(worldPos, myData.g_ProjMatrixLight);
	float3 projShadowMapPos2 = projShadowMapPos.xyz;
	
	projShadowMapPos2.x = projShadowMapPos2.x / projShadowMapPos.w;
	projShadowMapPos2.y = projShadowMapPos2.y / projShadowMapPos.w;
	projShadowMapPos2.x = (projShadowMapPos2.x + 1.0f) / 2;
	projShadowMapPos2.y = (projShadowMapPos2.y + 1.0f)/ 2;
	
	//projShadowMapPos2.x = 0.5f;
	//projShadowMapPos2.y = 0.5f;
	float projShadowDepth = projShadowMapPos2.z / projShadowMapPos.w;
	
	float shadowDepth = g_shadowTexture.Sample(g_sampler, projShadowMapPos2.xy);
	
 
	// try get positions
    float3 viewPos = worldPos.xyz;
	float distToLight = length(viewPos - lightPos);
	
	if(distToLight > 10.0)
	{
	//	output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);	
	//	return output;	
	}
    float3 lightDir = float3(0,-1,0);
	float lightIntensity = 1.0f;

    // Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(normals.xyz, -lightDir));

	// From the web: blinn-phong
	float3 LightDiffuseColor = float3(0.7,0.7,0.7); // intensity multiplier
	float3 LightSpecularColor = float3(0.7,0.7,0.7); // intensity multiplier
	float3 AmbientLightColor = float3(0.05,0.05,0.05);
	float3 DiffuseColor = float3(1,1,1);
	float3 SpecularColor = float3(1,1,1);
	float SpecularPower = 12.0f;

	// Phong relfection is ambient + light-diffuse + spec highlights.
	// I = Ia*ka*Oda + fatt*Ip[kd*Od(N.L) + ks(R.V)^n]
	// Ref: http://www.whisqu.se/per/docs/graphics8.htm
	// and http://en.wikipedia.org/wiki/Phong_shading
	
	// Get light direction for this fragment
	lightDir = normalize(worldPos - lightPos); // per pixel diffuse lighting
	
	// Note: Non-uniform scaling not supported
	float diffuseLighting = saturate(dot(normals.xyz, -lightDir));
	
	float LightDistanceSquared = distToLight*distToLight;
	// Introduce fall-off of light intensity
	diffuseLighting *= (LightDistanceSquared / dot(lightPos - worldPos, lightPos - worldPos));
	
	float3 CameraPos = myData.camPos.xyz;
	// Using Blinn half angle modification for perofrmance over correctness
	float3 h = normalize(normalize(CameraPos - worldPos) - lightDir);
	float specLighting = pow(saturate(dot(h, normals.xyz)), SpecularPower);
	float4 texel = colors;
	

	if(projShadowDepth - shadowDepth < -0.0001f) // hand tuned shadow bias
	{
		output.color = float4(saturate(texel * AmbientLightColor), texel.w);
		output.color = float4(0,0,0,0);
		return output;		
	}
	else
	{
		// light normally
		output.color = float4(saturate(
		texel * AmbientLightColor +
		(texel.xyz * DiffuseColor * LightDiffuseColor * diffuseLighting * 0.6) + // Use light diffuse vector as intensity multiplier
		(SpecularColor * LightSpecularColor * specLighting * 0.5) // Use light specular vector as intensity multiplier
		), texel.w);
			
	}
    // Determine the final amount of diffuse color based on the color of the pixel combined with the light intensity.
    //output.color = saturate(colors * lightIntensity);

	//output.color = g_colortexture.Sample(g_sampler, input.uv);
	//output.color = float4(input.uv.x, input.uv.y, 0.0f, 1.0f);
	//output.color = float4(g_normaltexture.Sample(g_sampler, input.uv).xyz, 1.0f);
	//output.color = float4(1.0f, 0.0f, 0.1f, 1.0f);
	//output.color = float4(depth, depth, depth, 1.0f);	
	//output.color = float4(viewPos.xyz, 1.0f);	
	//output.color = float4(g_constData[0].xyz, 1.0f);	
	//output.color = float4(distToLight, distToLight, distToLight, 1.0f);	
	//output.color = float4(worldPos.xyz, 1.0f);
	//output.color = float4(projShadowDepth,projShadowDepth,projShadowDepth, 1.0f) * 500.0f;
	float shadowmapval = g_shadowTexture.Sample(g_sampler, input.uv);
	shadowmapval = shadowDepth;
	//output.color = float4(shadowmapval,shadowmapval,shadowmapval,1) * 40.0f;
	//output.color = float4(projShadowMapPos2.xy, 0.0f, 1.0f);
	return output;	
}
