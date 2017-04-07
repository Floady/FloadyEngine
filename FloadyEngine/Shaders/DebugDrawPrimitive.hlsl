
struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

float4x4 g_offset : register(b0);

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;
	result.position = mul(float4(position.xyz, 1), g_offset);	
	result.color = float4(color.xyz, 1.0f);

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
	//return float4(1.0f, 1.0f, 0.1f, 1.0f);		
}