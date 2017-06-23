
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct PSOutput
{
	float4 color;
};

struct MyData
{
	float4x4 g_invProjMatrix;
	float4x4 g_ProjMatrixLight;
	float4x4 g_ProjMatrixLightOrtho;
    float4 lightWorldPos[10];
    float4 camPos;
};

Texture2D<float4> g_colortexture : register(t0);
Texture2D<float4> g_normaltexture : register(t1);
Texture2D<float> g_depthTexture : register(t2);
Texture2D<float> g_shadowTexture : register(t3);
Texture2D<float> g_shadowTexture1 : register(t4);
Texture2D<float> g_shadowTexture2 : register(t5);
Texture2D<float> g_shadowTexture3 : register(t6);
Texture2D<float> g_shadowTexture4 : register(t7);
Texture2D<float> g_shadowTexture5 : register(t8);
Texture2D<float> g_shadowTexture6 : register(t9);
Texture2D<float> g_shadowTexture7 : register(t10);
Texture2D<float> g_shadowTexture8 : register(t11);
Texture2D<float> g_shadowTexture9 : register(t12);
SamplerState g_sampler : register(s0);

ConstantBuffer<MyData> myData : register(b0);
float4 g_constData[4] : register(b1);

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD)
{
	PSInput result;

	result.position = float4(position, 1.0f);
	result.uv = uv;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	PSOutput output;
	
	// to check if bound by shader performance
	//output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);	
	//return output;
	
	float4 colors = g_colortexture.Sample(g_sampler, input.uv);
	float4 normals = g_normaltexture.Sample(g_sampler, input.uv);
	normals = normals * 2.0f;
	normals = normals - float4(1,1,1,1);
	
	// get linear depth
	float depth = g_depthTexture.Sample(g_sampler, input.uv);  // low precision here will cause banding in shadows and specular (currently aweful precision) rebnder this out and see :)
	
	float testDepth = (depth*100.0f);
	float zNear = 0.01;
	float zFar = 60.0;
	
	float4 H = float4(input.uv.x * 2 - 1, (1.0f - input.uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, myData.g_invProjMatrix); 
	float4 worldPos = D / D.w;
	worldPos.w = 1.0f;
 	
	output.color = g_shadowTexture1.Sample(g_sampler, input.uv) * 160.0f;
//	return output;
	
	output.color = float4(0,0,0,0);
	
	[loop]
	for( int qh = 0; qh < 10; qh++ )
	{
		float4 lightPosTest = float4(myData.lightWorldPos[qh].xyz, 1.0f);
		float distToLight = length(worldPos.xyz - lightPosTest.xyz);
		float maxRange = myData.lightWorldPos[qh].w;
		bool isOrtho = maxRange == 0;
		
		//float coloredDist = distToLight / 10.0f;
					
		//if(distToLight < 9999 && (isOrtho || (distToLight < maxRange)) && (lightPosTest.x + lightPosTest.y + lightPosTest.z != 0))
		if(distToLight < 9999  && (lightPosTest.x + lightPosTest.y + lightPosTest.z != 0))
		{
			float4 projShadowMapPos;
			if(isOrtho)
				projShadowMapPos = mul(worldPos, myData.g_ProjMatrixLightOrtho);
			else
			{
				float4x4 test = myData.g_ProjMatrixLight;
				projShadowMapPos = mul(worldPos, test);	
			}
			
			float3 projShadowMapPos2 = projShadowMapPos.xyz;
				
			projShadowMapPos2.x = projShadowMapPos2.x / projShadowMapPos.w;
			projShadowMapPos2.y = projShadowMapPos2.y / projShadowMapPos.w;
			projShadowMapPos2.x = (projShadowMapPos2.x + 1.0f) / 2;
			projShadowMapPos2.y = 1.0f - ((projShadowMapPos2.y + 1.0f)/ 2);
			
			float projShadowDepth = projShadowMapPos2.z / projShadowMapPos.w;
			
			float shadowDepth = g_shadowTexture.Sample(g_sampler, projShadowMapPos2.xy);
			if(qh == 1)
				shadowDepth = g_shadowTexture1.Sample(g_sampler, projShadowMapPos2.xy);
				
			// get neighbor avg
			if(false)
			{
				shadowDepth = 0.0f;
				float2 shadowuvstep = float2(1.0f, 1.0f) / float2(800.0f, 600.0f); // todo fixed resolution here
				int nrOfPixelsOut = 2;
				[loop]
				for( int i = -nrOfPixelsOut; i <= nrOfPixelsOut; i++ )
				{
					[loop]
					for( int j = -nrOfPixelsOut; j <= nrOfPixelsOut; j++ )
					{
					
					if(qh == 1)
						shadowDepth += g_shadowTexture1.Sample(g_sampler, projShadowMapPos2.xy + float2(shadowuvstep.x * i, -shadowuvstep.y * j));
					else
						shadowDepth += g_shadowTexture.Sample(g_sampler, projShadowMapPos2.xy + float2(shadowuvstep.x * i, -shadowuvstep.y * j));
					}
				}
				
				shadowDepth /= ((2*nrOfPixelsOut+1)*(2*nrOfPixelsOut+1));
			}
			
			float lightIntensity = 1.0f;
			
			// From the web: blinn-phong
			float3 LightDiffuseColor = float3(0.6,0.6,0.6); // intensity multiplier
			float3 LightSpecularColor = float3(0.7,0.7,0.7); // intensity multiplier
			float3 AmbientLightColor = float3(0.1,0.1,0.1);
			float3 DiffuseColor = float3(1,1,1);
			float3 SpecularColor = float3(1,1,1);
			float SpecularPower = 12.0f;

			// Phong relfection is ambient + light-diffuse + spec highlights.
			// I = Ia*ka*Oda + fatt*Ip[kd*Od(N.L) + ks(R.V)^n]
			// Ref: http://www.whisqu.se/per/docs/graphics8.htm
			// and http://en.wikipedia.org/wiki/Phong_shading
			
			// Get light direction for this fragment
			float3 lightDir = normalize(worldPos.xyz - lightPosTest.xyz); // per pixel diffuse lighting - point light / spot light type
			
			if(qh == 0)
				lightDir = normalize(float3(0,-1,1)); // uncomment this line for directional lighting
			
			// Note: Non-uniform scaling not supported
			float diffuseLighting = saturate(dot(normals.xyz, -lightDir));
			
			// Calculate the amount of light on this pixel.
			lightIntensity = saturate(dot(normals.xyz, -lightDir));

			float LightDistanceSquared = distToLight*distToLight;
			// Introduce fall-off of light intensity
			diffuseLighting *= ((LightDistanceSquared / dot(lightPosTest - worldPos, lightPosTest - worldPos)));
			
			float3 CameraPos = myData.camPos.xyz;
			// Using Blinn half angle modification for perofrmance over correctness
			float3 h = normalize(normalize(CameraPos - worldPos) - lightDir);
			float specLighting = pow(saturate(dot(h, normals.xyz)), SpecularPower);
			float4 texel = colors;
			
			float shadowBiasParam = 0.00001f;
			float shadowBias = shadowBiasParam*tan(acos(saturate(dot(normals.xyz, -lightDir)))); // cosTheta is dot( n,l ), clamped between 0 and 1
			shadowBias = clamp(shadowBias, 0.0f, 0.1f);
			//shadowBias = -0.000005f;

			bool isOutOfLightZone = false;
			
			// check for inside spotlight cone
			if(qh == 1)
			{			
				float3 dirWorldToLight = worldPos.xyz - lightPosTest.xyz;
				dirWorldToLight = normalize(dirWorldToLight);
				float3 lightDir3 = mul(myData.g_ProjMatrixLight, float4(0,0,1,1)).xyz;	
				lightDir3 = normalize(lightDir3);
				if(dot(lightDir3, dirWorldToLight) < 0.85)
				{
					isOutOfLightZone = true;						
				}
				
				shadowBias = -0.000005f; // test for spotlights, better shadows
			}
			
			// check for shadow culled
			if(projShadowDepth < shadowDepth - shadowBias)
			{
				isOutOfLightZone = true;
			}
			
			if(isOutOfLightZone) // hand tuned shadow bias
			{
				output.color += float4(saturate(texel * AmbientLightColor), texel.w);
				//output.color = float4(1,0,0,0);
				//return output;		
			}
			else
			{
				// light normally
				output.color += float4(saturate(
				texel * AmbientLightColor +
				(texel.xyz * DiffuseColor * LightDiffuseColor * diffuseLighting * 0.6)	// Use light diffuse vector as intensity multiplier
				+ (SpecularColor * LightSpecularColor * specLighting * 0.5) // Use light specular vector as intensity multiplier
				), texel.w);
					
			}
		}
	}
	
    // Determine the final amount of diffuse color based on the color of the pixel combined with the light intensity.
    //output.color = saturate(colors * lightIntensity);

	//output.color = g_colortexture.Sample(g_sampler, input.uv);
	//output.color = float4(input.uv.x, input.uv.y, 0.0f, 1.0f);
	//output.color = float4(g_normaltexture.Sample(g_sampler, input.uv).xyz, 1.0f);
	//output.color = float4(1.0f, 0.0f, 0.1f, 1.0f);
	//output.color = float4(depth, depth, depth, 1.0f) * 100.0f;	
	//output.color = float4(viewPos.xyz, 1.0f);	
	//output.color = float4(normals.xyz, 1.0f);	
	//output.color = float4(g_constData[0].xyz, 1.0f);	
	//distToLight = distToLight * 0.01f;
	//output.color = float4(distToLight, distToLight, distToLight, 1.0f);	
	//output.color = float4(worldPos.xyz, 1.0f);
	//projShadowDepth = projShadowDepth * projShadowDepth;
	//output.color = float4(projShadowDepth,projShadowDepth,projShadowDepth, 1.0f) * 0.5f;
	float shadowmapval = 1.0f - g_shadowTexture.Sample(g_sampler, input.uv);
	
	//output.color = float4(input.uv.xy, 1.0f, 1.0f);
	//output.color = float4(shadowmapval, 0.0f, 0.0f, 1.0f);
	//output.color = g_shadowTexture1.Sample(g_sampler, input.uv) * 30.0f;
	return output;	
}
