
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : TEXCOORD2;
	float depth : TEXCOORD1;
	int matId : TEXCOORD3;
	int normalmatId : TEXCOORD4;
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
	float4x4 g_transform[64];
};

ConstantBuffer<MyData> myData : register(b0);
SamplerState g_sampler : register(s0);
Texture2D g_texture[64] : register(t0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float4 uv : TEXCOORD1, uint matId : TEXCOORD2, uint nmatId : TEXCOORD3, uint InstanceId : SV_InstanceID)
{
	PSInput result;

	result.position = float4(position.xyz, 1);
	result.position = mul(result.position, myData.g_transform[InstanceId]);
	result.position = mul(result.position, myData.g_viewProjMatrix);
	result.depth    = (result.position.z);
	
	result.uv = uv;
	result.normal = normal;
	result.normal.w = 0.0f;
	result.normal.xyz = mul(result.normal, (const float3x3)myData.g_transform[InstanceId]);
	result.normal = normalize(result.normal);
	//result.normal = (result.normal + float4(1, 1, 1,1))/2.0f; // vertex calculated normals
	result.matId = matId.x;
	result.normalmatId = nmatId.x;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = float4(1.0f,0,0,0);
	//output.color = input.normal; // debug normal draw
	output.normal = input.normal;
	output.depth	 = input.depth;
	output.depth = input.position.z;
	
	int matId = input.matId;
	
	matId = input.matId;
	output.color = g_texture[matId].Sample(g_sampler, input.uv);	
	
	float4 normal = input.normal;	
	
	if(input.normalmatId != 99)
	{
		normal = g_texture[31 + input.normalmatId].Sample(g_sampler, input.uv);		
		output.normal = normalize(input.normal + normal);
	}	
	
	output.normal = (output.normal + float4(1, 1, 1,1))/2.0f;
	
	return output;
}