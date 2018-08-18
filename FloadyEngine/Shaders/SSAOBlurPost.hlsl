
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
Texture2D<float4> g_combinedTexture : register(t4);
Texture2D<float4> scratchbuff : register(t5);
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
	
	float2 texelSize = float2(1.0f / 1600.0f, 1.0f / 900.0f);  //@todo: feed buffer dimensions here (render window width height, or actually buffer width heights)
	
	float4 blurVal = 0.0f;
	int nrOfSample = 2;
	for( int i = -nrOfSample; i <= nrOfSample; i++ )
	{
		for( int j = -nrOfSample; j <= nrOfSample; j++ )
		{
			float4 neighbour = scratchbuff.Sample(g_sampler, input.uv + float2(texelSize.x * i, texelSize.y * j));
			blurVal += neighbour;
		}
	}
	
	blurVal /= (2*nrOfSample+1)*(2*nrOfSample+1);
	
	output.color = blurVal;
	//output.color *= scratchbuff.Sample(g_sampler, input.uv);
	output.color *= g_combinedTexture.Sample(g_sampler, input.uv);
		
	return output;	
}
