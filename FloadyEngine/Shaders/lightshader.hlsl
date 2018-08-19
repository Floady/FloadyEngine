
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
	float4 camPos;
};

struct Light
{
	float4x4 myLightViewProjMatrix;
	float4 myWorldPos;
	float4 myWorldDir;
	float4 myColor;
	float4 myLightType; // x = 1 = directional, 2 = spotlight   - spotlight packs angle and range is y and z
}; //32 * 4 = 128 bytes     // todo: can pack light type into pos/dir/color w component

struct MyLightArray
{
	Light myLights[32];
};
Texture2D<float4> g_colortexture : register(t0);
Texture2D<float4> g_normaltexture : register(t1);
Texture2D<float> g_depthTexture : register(t2);
Texture2D<float> g_SpecularTexture : register(t3);
Texture2D<float> g_shadowTexture[10] : register(t4);
SamplerState g_sampler : register(s0);

ConstantBuffer<MyData> myData : register(b0);
ConstantBuffer<MyLightArray> myLights : register(b1);

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
	
	//output.color = g_shadowTexture.Sample(g_sampler, input.uv) * 150;
	//return output;
	
	float4 colors = g_colortexture.Sample(g_sampler, input.uv);
	float4 normals = g_normaltexture.Sample(g_sampler, input.uv);
	
	float specularScalar = g_SpecularTexture.Sample(g_sampler, input.uv);
	//output.color = float4(specularScalar.xxx, 1.0f);
	//return output;
	
	bool receiveShadows = true;
	if(normals.w == 1.0f) // w float in normals is used to indicate if this pixel should be shadowculled
	{
		normals.w = 0;
		receiveShadows = false;
	}
	normals = normals * 2.0f;
	normals = normals - float4(1,1,1,1);
	
	// get linear depth
	float depth = g_depthTexture.Sample(g_sampler, input.uv);  // low precision here will cause banding in shadows and specular (currently aweful precision) rebnder this out and see :)
	float4 H = float4(input.uv.x * 2 - 1, (1.0f - input.uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, myData.g_invProjMatrix); 
	float4 worldPos = D / D.w;
	worldPos.w = 1.0f;
 	
	output.color = float4(0,0,0,0);
	
	if(receiveShadows)
	{
		[loop]
		for( int qh = 0; qh < 32; qh++ )
		{
			bool isLightMatrixValid = isLightMatrixValid = (myLights.myLights[qh].myLightViewProjMatrix._m03 + myLights.myLights[qh].myLightViewProjMatrix._m13 + myLights.myLights[qh].myLightViewProjMatrix._m23 + myLights.myLights[qh].myLightViewProjMatrix._m33) != 0;
				
			if(isLightMatrixValid)
			{		
				bool isDirectional = myLights.myLights[qh].myLightType == 1.0f;		
				bool isSpot = myLights.myLights[qh].myLightType == 2.0f;
				float shadowDepth = 0.0f;
				float projShadowDepth = 0.0f;
				if(receiveShadows)
				{	
					float4 projShadowMapPos = mul(worldPos, myLights.myLights[qh].myLightViewProjMatrix);			
					projShadowMapPos.x = projShadowMapPos.x / projShadowMapPos.w;
					projShadowMapPos.y = projShadowMapPos.y / projShadowMapPos.w;
					projShadowMapPos.x = (projShadowMapPos.x + 1.0f) / 2;
					projShadowMapPos.y = 1.0f - ((projShadowMapPos.y + 1.0f)/ 2);
					
					projShadowDepth = projShadowMapPos.z / projShadowMapPos.w;
					
					// todo: make this into array based
				
					shadowDepth = g_shadowTexture[qh].Sample(g_sampler, projShadowMapPos.xy);
							
					// get neighbor avg
					if(qh == 0 && false)
					{
						shadowDepth = 0.0f;
						float2 shadowuvstep = float2(1.0f, 1.0f) / float2(4096, 4096); // todo fixed resolution here
						int nrOfPixelsOut = 1;
						[loop]
						for( int i = -nrOfPixelsOut; i <= nrOfPixelsOut; i++ )
						{
							[loop]
							for( int j = -nrOfPixelsOut; j <= nrOfPixelsOut; j++ )
							{
								shadowDepth += g_shadowTexture[qh].Sample(g_sampler, projShadowMapPos.xy + float2(shadowuvstep.x * i, -shadowuvstep.y * j));
							}
						}
						
						shadowDepth /= ((2*nrOfPixelsOut+1)*(2*nrOfPixelsOut+1));
					}
				}
				
				// From the web: blinn-phong
				float3 LightDiffuseColor = float3(0.6,0.6,0.6); // intensity multiplier
				float3 LightSpecularColor = float3(0.7,0.7,0.7); // intensity multiplier
				float3 DiffuseColor = float3(1,1,1);
				float3 SpecularColor = float3(1,1,1) * specularScalar;
				float SpecularPower = 12.0f * specularScalar;

				// Phong relfection is ambient + light-diffuse + spec highlights.
				// I = Ia*ka*Oda + fatt*Ip[kd*Od(N.L) + ks(R.V)^n]
				// Ref: http://www.whisqu.se/per/docs/graphics8.htm
				// and http://en.wikipedia.org/wiki/Phong_shading
				
				// Get light direction for this fragment
				float4 lightPos = float4(myLights.myLights[qh].myWorldPos.xyz, 1);
				float distToLight = length(worldPos.xyz - lightPos.xyz);
						
				float3 lightDir = normalize(worldPos.xyz - lightPos.xyz); // per pixel diffuse lighting - point light / spot light type
				
				// directional has parallel beams
				if(isDirectional)
					lightDir = normalize(myLights.myLights[qh].myWorldDir.xyz);
				
				// Note: Non-uniform scaling not supported
				float diffuseLighting = saturate(dot(normals.xyz, -lightDir));
				
				// Calculate the amount of light on this pixel.
				float LightDistanceSquared = distToLight*distToLight;
				// Introduce fall-off of light intensity
				diffuseLighting *= ((LightDistanceSquared / dot(lightPos - worldPos, lightPos - worldPos)));
				
				float3 CameraPos = myData.camPos.xyz;
				// Using Blinn half angle modification for perofrmance over correctness
				float3 h = normalize(normalize(CameraPos - worldPos) - lightDir);
				float specLighting = pow(saturate(dot(h, normals.xyz)), SpecularPower);
				float4 texel = colors;
				
				float shadowBiasParam = 0.0001;
				float shadowBias = shadowBiasParam*tan(acos(saturate(dot(normals.xyz, -lightDir)))); // cosTheta is dot( n,l ), clamped between 0 and 1
				shadowBias = clamp(shadowBias, 0.0f, 0.1f);
				
				// a zero value here means its outside the frustum
				bool isOutOfLightZone = false;//shadowDepth == 0;
				
				// check for inside spotlight cone
				if(isSpot)
				{			
					float3 lightDir3 = normalize(myLights.myLights[qh].myWorldDir.xyz);
					float rad = acos(dot(lightDir3, lightDir));
					if(rad > myLights.myLights[qh].myLightType.y || distToLight > myLights.myLights[qh].myLightType.z)
					{	
						isOutOfLightZone = true;						
					}	
					else
					{
						float bandPercentage = 0.3;
						if(rad > myLights.myLights[qh].myLightType.y * (1.0f - bandPercentage))
						{
							float range = (myLights.myLights[qh].myLightType.y) * bandPercentage;
							float value = myLights.myLights[qh].myLightType.y - rad;
							float interp = ((value/range) * (value/range));
							//interp = (value/range);
							diffuseLighting = diffuseLighting * interp;
							specLighting = specLighting * interp;
						}
						isOutOfLightZone = false;
					}
					
					shadowBias = -0.000005f; // test for spotlights, better shadows
				}
				else // directional
				{
					if(distToLight > 400)			// arbitrary <X> cap for now for direcitonal light to avoid artifact in sky
						isOutOfLightZone = true;
				}
				
				// check for shadow culled
				if(receiveShadows) // todo hack: skybox sets receiveShadows to false - we shouldnt 'light' it either, just full ambient?
				{
					if(!isOutOfLightZone && shadowDepth != 0 && (projShadowDepth < shadowDepth - shadowBias))
					{
						isOutOfLightZone = true;
				
				//		output.color = float4(0.0f, 1.0f, 0.0f, 0.0f);	
					//	return output;
				}
				}
				
				if(!isOutOfLightZone)
				{
					float alpha = 1.0f;
					LightDiffuseColor = myLights.myLights[qh].myColor.xyz;
					LightSpecularColor = myLights.myLights[qh].myColor.xyz;
					alpha = myLights.myLights[qh].myColor.w;
					
					// light normally
					output.color += float4((alpha * 
					(texel.xyz * DiffuseColor * LightDiffuseColor * diffuseLighting * 0.6)	// Use light diffuse vector as intensity multiplier
					+ (SpecularColor * LightSpecularColor * specLighting * 0.5) // Use light specular vector as intensity multiplier
					), texel.w);
						
				}				
			}
		}
	}
	
	// fake some ambient lighting since we dont have GI
	float3 AmbientLightColor = float3(1,1,1) * 0.3f;
	
	if(!receiveShadows) // hack: skybox has this set to false, boost ambience to full since it doesnt receive shadows/lighting and should act as 'emissive'
		AmbientLightColor = float3(1,1,1) * 1.0f;
	
//	if(output.color.x > 1.0f || output.color.y > 1.0f || output.color.z > 1.0f)	
//	{
//		output.color = float4(0.0f, 1.0f, 0.0f, 0.0f);	
//		return output;
//	}
	output.color += colors * float4(AmbientLightColor, 1);

	return output;	
}

