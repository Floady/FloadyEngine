
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

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	PSInput result;

	result.position = position;
	result.position = mul(result.position, myData.g_transform);
	result.position = mul(result.position, myData.g_viewProjMatrix);
	result.depth    = (result.position.z / result.position.w);
	
	result.uv = uv;
	result.normal = normal;
	result.normal = mul(result.normal, myData.g_transform);
	result.normal = (result.normal + float4(1, 1, 1,1))/2.0f;
	//result.normal = normalize(result.normal);
	
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = g_texture.Sample(g_sampler, input.uv);
	//output.color = input.normal; // debug normal draw
	output.normal = input.normal;
	output.depth	 = input.depth;
	//output.color = float4(1.0f, 1.0f, 0.1f, 1.0f);		
	return output;
}