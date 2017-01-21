
struct PSInput
{
	float4 position : SV_POSITION;
	float depth : TEXCOORD1;
};

struct PSOutput
{
	float4 color;
	float depth;
};

float4x4 g_offset : register(b0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	PSInput result;

	float4 newPos = position;
	result.position = mul(newPos, g_offset);
	result.depth    = (result.position.z / result.position.w);
	
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = float4(1.0f, 1.0f, 0.1f, 1.0f);	
	output.depth	 = input.depth;	
	return output;
}