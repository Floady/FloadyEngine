
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : TEXCOORD2;
	float depth : TEXCOORD1;
};

struct PSOutput
{
	float4 color;
	float4 normal;
	float depth;
};

struct MyData
{
	float4x4 g_viewProjMatrix;
	float4x4 g_transform;
};

ConstantBuffer<MyData> myData : register(b0);
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
	PSInput result;

	result.position = float4(position.xyz, 1);
	result.position = mul(result.position, myData.g_transform);
	result.position = mul(result.position, myData.g_viewProjMatrix);
	result.depth    = (result.position.z);
	
	result.uv = uv;
	result.normal = float4(normal,1);
	result.normal.w = 0.0f;
	result.normal.xyz = mul(result.normal, (const float3x3)myData.g_transform);
	result.normal = normalize(result.normal);
	result.normal = (result.normal + float4(1, 1, 1,1))/2.0f;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = g_texture.Sample(g_sampler, input.uv);
	//output.color = input.normal; // debug normal draw
	output.normal = input.normal;
	output.normal.w = 1.0f; // indicate we do not want shadowculling
	output.depth	 = input.depth;
	output.depth = input.position.z;
	//output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);		
	return output;
}