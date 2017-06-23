
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
Texture2D<float4> scratchbuff : register(t2);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float4 uv : TEXCOORD)
{
	PSInput result;
	result.position = float4(position.xyz, 1);
	result.uv = uv.xy;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	PSOutput output;
	float2 uvStride = float2(1.0f/800.0f, 1.0f/600.0f);
	
	// Edge detection test 1 - todo should use dot normals :)
	bool doEdgeDetection = true;
	float4 color = g_scratchtexture.Sample(g_sampler, input.uv);
	if(doEdgeDetection)
	{
		float threshold = 0.2f; 
		[loop]
		for( int i = -1; i < 1; i++ )
		{
			[loop]
			for( int j = -1; j < 1; j++ )
			{
				float neighbourColor = g_scratchtexture.Sample(g_sampler, input.uv - float2(uvStride.x * i, uvStride.y * j));
				if(abs(color.x-neighbourColor.x) > threshold)
				{
					output.color = float4(1,1,1,1);
					return output;
				}
			}
		}
	}
	
	output.color = float4(0,0,0,1);
	return output;	
}
