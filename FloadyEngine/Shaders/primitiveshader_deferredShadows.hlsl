
struct PSInput
{
	float4 position : SV_POSITION;
};

struct PSOutput
{
	float4 color;
};

struct MyData
{
	float4x4 g_viewProjMatrix;
	float4x4 g_transform;
};

ConstantBuffer<MyData> myData : register(b0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	PSInput result;
	result.position = position;
	result.position = mul(result.position, myData.g_transform);
	result.position = mul(result.position, myData.g_viewProjMatrix);
		
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = float4(1.0f, 1.0f, 0.1f, 1.0f);	
	return output;
}