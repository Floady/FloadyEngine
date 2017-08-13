
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
	float4x4 g_projMatrix;
};

Texture2D<float4> g_colortexture : register(t0);
Texture2D<float4> g_normaltexture : register(t1);
Texture2D<float> g_depthTexture : register(t2);
Texture2D<float> g_shadowTexture : register(t3);
Texture2D<float4> g_combinedTexture : register(t4);
Texture2D<float4> scratchbuff : register(t5);
SamplerState g_sampler : register(s0);
MyData constData : register(b0);

static const int kernelSize = 16;
static const int noiseSize = 16;

float3 getPosition(in float2 uv)
{	
	float depth = g_depthTexture.Sample(g_sampler, uv);  // low precision here will cause banding in shadows and specular (currently aweful precision) rebnder this out and see :)
	float4 H = float4(uv.x * 2 - 1, (1.0f - uv.y) * 2 - 1, 1.0f - depth , 1);
	
	return H.xyz;
}

float3 getWorldPos(in float2 uv)
{	
	float depth = g_depthTexture.Sample(g_sampler, uv);  // low precision here will cause banding in shadows and specular (currently aweful precision) rebnder this out and see :)
	float4 H = float4(uv.x * 2 - 1, (1.0f - uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, constData.g_invProjMatrix); 
	float4 worldPos = D / D.w;
	worldPos.w = 1.0f;
 	
	return worldPos.xyz;
}

float3 getNormal(in float2 uv)
{
	return normalize(g_normaltexture.Sample(g_sampler, uv).xyz * 2.0f - 1.0f);
}

float2 getRandom(in float2 uv)
{
	return float2(0,1);
	float2 random_size = float2(1.0, 1.0);
	float2 g_screen_size = float2(1.0f, 1.0f);
	return normalize(g_normaltexture.Sample(g_sampler, g_screen_size * uv / random_size).xy * 2.0f - 1.0f);
}

float doAmbientOcclusion(in float2 tcoord,in float2 uv, in float3 p, in float3 cnorm)
{
	float g_intensity = 2.1;
	float g_scale = 10.1;
	float g_bias = 0.3;
	float3 diff = getPosition(tcoord + uv) - p;
	const float3 v = normalize(diff);
	const float d = length(diff)*g_scale;
	return max(0.0,dot(cnorm,v)-g_bias)*(1.0/(1.0+d))*g_intensity;
}

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float4 uv : TEXCOORD)
{
	PSInput result;
	result.position = float4(position.xyz, 1);
	result.uv = uv.xy;
	return result;
}

float pleudorandom( float2 p )
{
    float2 K1 = float2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfondâ€“Schneider constant)
    );
    return frac( cos( dot(p,K1) ) * 12345.6789 );
}

float random(in float amin, in float amax, in float2 uv)
{
	return amin + (pleudorandom(uv) * amax);
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	PSOutput output;
	float g_sample_rad = 0.1;
	
	const float2 vec[4] = {float2(1,0),float2(-1,0),
			float2(0,1),float2(0,-1)};

	float3 p = getPosition(input.uv);
	float3 n = getNormal(input.uv);

	// create kernel
	float3 kernel[100];
	for (int i = 0; i < kernelSize; ++i) {
	   kernel[i] = float3(
	   random(-1.0f, 1.0f, input.uv),
	   random(-1.0f, 1.0f, input.uv),
	   random(0.0f, 1.0f, input.uv));
	   normalize(kernel[i]);
	    float scale = float(i) / float(kernelSize);
		scale = lerp(0.1f, 1.0f, scale * scale);
		kernel[i] *= scale;
	}
	
	// generate noise map
	float3 noiseMap[100];
	for (int i = 0; i < noiseSize; ++i) {
		noiseMap[i] = float3(
			random(-1.0f, 1.0f, input.uv),
			random(-1.0f, 1.0f, input.uv),
			0.0f);
		normalize(noiseMap[i]);
	}

	float3 normal = n;
	float3 origin = p;
	float3 rvec = noiseMap[int((random(0.0f, 1.0f, input.uv) * 5.0))] * 2.0 - 1.0;
	float3 tangent = normalize(rvec - normal * dot(rvec, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 tbn = {tangent.x, tangent.y, tangent.z,
					bitangent.x, bitangent.y, bitangent.z,
					normal.x, normal.y, normal.z};
	
//	float3x3 tbn = { tangent.x, bitangent.x, normal.x,
//			tangent.y, bitangent.y, normal.y,
//			tangent.z, bitangent.z, normal.z };
	
	float occlusion = 0.0;
	int uSampleKernelSize = 16;
	float uRadius = 10.5f;
	for (int i = 0; i < uSampleKernelSize; ++i) 
	{
		// get sample position:
	   float3 sample = mul(kernel[i], tbn);
	   sample = sample * uRadius + origin;
	//   sample = origin; // test line to fix reprojection
	  
		// project sample position:
		float4 offset = float4(sample, 1.0);
		offset = mul(offset, constData.g_projMatrix);
		offset.y = 1.0f - offset.y;
		offset.xy *= offset.w;
		offset.xy = (offset.xy * 0.5) + float2(1.0f ,1.0f);
	  
		// get sample depth:
		float sampleDepth = g_depthTexture.Sample(g_sampler, offset.xy);  // low precision here will cause banding in shadows and specular (currently aweful precision) rebnder this out and see :)

		// range check & accumulate:
	   float rangeCheck= abs(origin.z - sampleDepth) < uRadius ? 1.0 : 0.0;
	   occlusion += (sampleDepth <= sample.z ? 1.0 : 0.0) * rangeCheck;
	
		//output.color = float4(offset.xy * 1.0f, 1,1);
		output.color = float4(sample.xyz, 1);
		//output.color = float4(origin.xy, origin.z, 1);
		return output;	
	}


	
	output.color = float4(occlusion.xxx / uSampleKernelSize, 1);
	//output.color = float4(scratchbuff.Sample(g_sampler, input.uv).xyz, 1.0);
	//output.color += float4(tangent.xyz, 1);
	//output.color = float4(origin.xyz / uSampleKernelSize, 1);
	return output;	
}
