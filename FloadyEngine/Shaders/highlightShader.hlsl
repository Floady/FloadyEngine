
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float depth : TEXCOORD1;
};

float4x4 g_offset : register(b0);

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD)
{
	PSInput result;

	result.position = float4(position, 1);
	result.position = mul(result.position, g_offset);
	result.uv = uv;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);		
}