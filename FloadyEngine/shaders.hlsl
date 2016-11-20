
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
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
	result.uv = uv;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return g_texture.Sample(g_sampler, input.uv);
	//return float4(input.uv.x,1,1, 1);
}