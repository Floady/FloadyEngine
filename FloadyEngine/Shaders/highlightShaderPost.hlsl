
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct PSOutput
{
	float4 color;
};

Texture2D<float4> g_scratchtexture : register(t0);
Texture2D<float4> g_combinedTexture : register(t1);

SamplerState g_sampler : register(s0);

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
	PSInput result;
	result.position = float4(position.xyz, 1);
	result.uv = uv.xy;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	PSOutput output;
	float2 uvStride = float2(1.0f/1600.0f, 1.0f/900.0f);
	
	// Edge detection test 1 - todo should use dot normals :)
	bool doEdgeDetection = true;
	float4 color = g_scratchtexture.Sample(g_sampler, input.uv);
	output.color = g_combinedTexture.Sample(g_sampler, input.uv);
	if(doEdgeDetection)
	{
		float threshold = 0.2f; 
		[loop]
		for( int i = -2; i < 2; i++ )
		{
			[loop]
			for( int j = -2; j < 2; j++ )
			{
				float neighbourColor = g_scratchtexture.Sample(g_sampler, input.uv - float2(uvStride.x * i, uvStride.y * j));
				if(abs(color.x-neighbourColor.x) > threshold)
				{
					output.color += float4(0.2, 0.2, 0.4 ,1);
					return output;
				}
			}
		}
	}
	
	return output;	
}
