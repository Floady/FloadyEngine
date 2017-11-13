
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
	float4x4 g_invViewProjMatrix;
	float4x4 g_projMatrix;
	float4x4 g_invProjMatrix;
	float4x4 g_viewMatrix;
	float4x4 g_InvViewMatrix;
};

Texture2D<float4> g_colortexture : register(t0);
Texture2D<float4> g_normaltexture : register(t1);
Texture2D<float> g_depthTexture : register(t2);
Texture2D<float> g_shadowTexture : register(t3);
Texture2D<float4> g_combinedTexture : register(t4);
Texture2D<float4> scratchbuff : register(t5);
SamplerState g_sampler : register(s0);
MyData constData : register(b0);

float3 getPosition(in float2 uv)
{	
	float depth = g_depthTexture.Sample(g_sampler, uv); 
	float4 H = float4(uv.x * 2 - 1, (uv.y) * 2 - 1, depth , 1);
	
	return H.xyz;
}

float3 getWorldPos(in float2 uv)
{	
	float depth = g_depthTexture.Sample(g_sampler, uv); 
	float4 H = float4(uv.x * 2 - 1, (uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, constData.g_invViewProjMatrix); 
	float4 worldPos = D / D.w;
 	
	return worldPos.xyz;
}

float3 getViewPos(in float2 uv)
{	
	/* // todo: ok to remove?
	float depth = g_depthTexture.Sample(g_sampler, float2(uv.x, uv.y)); 
	float4 H = float4(uv.x * 2 - 1, (uv.y) * 2 - 1, depth, 1);
	float4 D = mul(H, constData.g_invProjMatrix); 
	float4 viewPos = D / D.w;
	//return float3(uv.xy, viewPos.z / 400);
	viewPos.z /= 400;
	// viewPos.y  = 1.0f - viewPos.y; // this is for GL?
	//return viewPos.xyz;
	*/
	
	// world
	float depth = g_depthTexture.Sample(g_sampler, uv);  // low precision here will cause banding in shadows and specular (currently aweful precision) rebnder this out and see :)
	float4 H = float4(uv.x * 2 - 1, (1.0f - uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, constData.g_invViewProjMatrix); 
	float4 worldPos = D / D.w;
	worldPos.w = 1.0f;
	
	float4 viewSpacePos = mul(worldPos, constData.g_viewMatrix); 
	viewSpacePos = viewSpacePos / viewSpacePos.w;
	return viewSpacePos.xyz;
}

float3 getNormal(in float2 uv)
{
	return normalize(g_normaltexture.Sample(g_sampler, uv).xyz * 2.0f - 1.0f);
}

float3 getNormalViewSpace(in float2 uv)
{
	float4 H = float4(normalize(g_normaltexture.Sample(g_sampler, uv).xyz * 2.0f - 1.0f), 1);
	float4x4 modelViewProjection = mul(constData.g_viewMatrix, constData.g_projMatrix);
	float3 n = mul(H.xyz, (float3x3)constData.g_viewMatrix); 
	
	return normalize(n);
}

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float4 uv : TEXCOORD)
{
	PSInput result;
	result.position = float4(position.xyz, 1);
	result.uv = uv.xy;
	return result;
}

float pleudorandom( in float2 p )
{
    float2 K1 = float2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfondâ€“Schneider constant)
    );
    return frac( cos( dot(p,K1) ) * 12345.6789 );
}

float random(in float amin, in float amax, in float2 uv)
{
	return amin + (pleudorandom(uv) * (amax-amin));
}

float doAmbientOcclusion(in float2 tcoord,in float2 uv, in float3 p, in float3 cnorm)
{
	float g_intensity = 4.0;
	float g_scale = 2.0;
	float g_bias = 0.1;
	float3 diff = getViewPos(tcoord + uv) - p;
	const float3 v = normalize(diff);
	const float d = length(diff)*g_scale;
	return max(0.0,dot(cnorm,v)-g_bias)*(1.0/(1.0+d))*g_intensity;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	PSOutput output;
	
	const float2 vec[4] = {float2(1,0),float2(-1,0),
			float2(0,1),float2(0,-1)};

	float3 p = getViewPos(input.uv);
	float3 n = getNormalViewSpace(input.uv);

//	p = getViewPos(input.uv);
//	n = getNormalViewSpace(input.uv);

	float2 rand = normalize(float2(random(-1.0f, 1.0f, input.uv), random(-1.0f, 1.0f, 0.5f * input.uv)));

	float ao = 0.0f;
	float g_sample_rad = 1.4;
	float rad = g_sample_rad/p.z;

	float g_far_clip = 400.0f;
	int iterations = 4;
	iterations = lerp(6.0,2.0,p.z/g_far_clip);
	for (int j = 0; j < iterations; ++j)
	{
	  float2 coord1 = reflect(vec[j],rand)*rad;
	  float2 coord2 = float2(coord1.x*0.707 - coord1.y*0.707,
				  coord1.x*0.707 + coord1.y*0.707);
	  
	  ao += doAmbientOcclusion(input.uv,coord1*0.25, p, n);
	  ao += doAmbientOcclusion(input.uv,coord2*0.5, p, n);
	  ao += doAmbientOcclusion(input.uv,coord1*0.75, p, n);
	  ao += doAmbientOcclusion(input.uv,coord2, p, n);
	}
	ao/=(float)iterations*4.0;
	
	//output.color = float4(scratchbuff.Sample(g_sampler, input.uv).xyz, 1.0);
	output.color = float4(1.0f - ao.xxx, 1);
	//output.color = float4(ao.x*5, 0, 0, 1);
	//output.color += float4(tangent.xyz, 1);
	//output.color = float4(origin.xyz / 40, 1);
	//output.color = float4(n.xyz * 0.5 + 0.5, 1);
	//output.color = float4(rand, 0, 1);
	//output.color = float4(offset.xyz, 1);
	//output.color = float4(p.xyz, 1);
	//output.color = float4(g_depthTexture.Sample(g_sampler, input.uv) * 1000, 0, 0, 1);
	
	//if(ao.x > 0.1f)
	//	output.color = float4(1,0,0,1);
		
	return output;	
}
