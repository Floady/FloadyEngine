
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float depth : TEXCOORD1;
};

float4x4 g_offset : register(b0);
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 uv : TEXCOORD)
{
	PSInput result;

	float4 newPos = position;
	result.position = mul(newPos, g_offset);
	//result.position.z = 0.5f; // set temp test depth?
	//result.position.w = 0.5f; // set temp test depth?
	result.depth    = 1.0f - (result.position.z / result.position.w);
	
	result.uv = uv;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return g_texture.Sample(g_sampler, input.uv);
	//return float4(input.depth, input.depth, input.depth, 1.0f);		
}