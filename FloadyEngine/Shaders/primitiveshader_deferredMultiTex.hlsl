
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : TEXCOORD2;
	float depth : TEXCOORD1;
	int matId : TEXCOORD3;
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
SamplerState g_sampler : register(s0);
Texture2D g_texture0 : register(t0);
Texture2D g_texture1 : register(t1);
Texture2D g_texture2 : register(t2);
Texture2D g_texture3 : register(t3);
Texture2D g_texture4 : register(t4);
Texture2D g_texture5 : register(t5);
Texture2D g_texture6 : register(t6);
Texture2D g_texture7 : register(t7);
Texture2D g_texture8 : register(t8);
Texture2D g_texture9 : register(t9);
Texture2D g_texture10 : register(t10);
Texture2D g_texture11 : register(t11);
Texture2D g_texture12 : register(t12);
Texture2D g_texture13 : register(t13);
Texture2D g_texture14 : register(t14);
Texture2D g_texture15 : register(t15);
Texture2D g_texture16 : register(t16);
Texture2D g_texture17 : register(t17);
Texture2D g_texture18 : register(t18);
Texture2D g_texture19 : register(t19);
Texture2D g_texture20 : register(t20);
Texture2D g_texture21 : register(t21);
Texture2D g_texture22 : register(t22);
Texture2D g_texture23 : register(t23);
Texture2D g_texture24 : register(t24);
Texture2D g_texture25 : register(t25);
Texture2D g_texture26 : register(t26);
Texture2D g_texture27 : register(t27);
Texture2D g_texture28 : register(t28);
Texture2D g_texture29 : register(t29);
Texture2D g_texture30 : register(t30);
Texture2D g_texture31 : register(t31);
Texture2D g_texture32 : register(t32);


PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float4 uv : TEXCOORD1, uint matId : TEXCOORD2)
{
	PSInput result;

	result.position = float4(position.xyz, 1);
	result.position = mul(result.position, myData.g_transform);
	result.position = mul(result.position, myData.g_viewProjMatrix);
	result.depth    = (result.position.z);
	
	result.uv = uv;
	result.normal = normal;
	result.normal.w = 0.0f;
	result.normal.xyz = mul(result.normal, (const float3x3)myData.g_transform);
	result.normal = normalize(result.normal);
	result.normal = (result.normal + float4(1, 1, 1,1))/2.0f;
	result.matId = matId.x;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = float4(0,0,0,0);
	//output.color = input.normal; // debug normal draw
	output.normal = input.normal;
	output.depth	 = input.depth;
	output.depth = input.position.z;
	
	int matId = input.matId;
	
	matId = input.matId + 1;
	if(matId == 0)
		output.color = g_texture0.Sample(g_sampler, input.uv);
	if(matId == 1)
		output.color = g_texture1.Sample(g_sampler, input.uv);	
	else if(matId == 2)
		output.color = g_texture2.Sample(g_sampler, input.uv);	
	else if(matId == 3)
		output.color = g_texture3.Sample(g_sampler, input.uv);	
	else if(matId == 4)
		output.color = g_texture4.Sample(g_sampler, input.uv);	
	else if(matId == 5)
		output.color = g_texture5.Sample(g_sampler, input.uv);	
	else if(matId == 6)
		output.color = g_texture6.Sample(g_sampler, input.uv);	
	else if(matId == 7)
		output.color = g_texture7.Sample(g_sampler, input.uv);	
	else if(matId == 8)
		output.color = g_texture8.Sample(g_sampler, input.uv);	
	else if(matId == 9)
		output.color = g_texture9.Sample(g_sampler, input.uv);	
	else if(matId == 10)
		output.color = g_texture10.Sample(g_sampler, input.uv);	
	else if(matId == 11)
		output.color = g_texture11.Sample(g_sampler, input.uv);	
	else if(matId == 12)
		output.color = g_texture12.Sample(g_sampler, input.uv);	
	else if(matId == 13)
		output.color = g_texture13.Sample(g_sampler, input.uv);	
	else if(matId == 14)
		output.color = g_texture14.Sample(g_sampler, input.uv);	
	else if(matId == 15)
		output.color = g_texture15.Sample(g_sampler, input.uv);	
	else if(matId == 16)
		output.color = g_texture16.Sample(g_sampler, input.uv);	
	else if(matId == 17)
		output.color = g_texture17.Sample(g_sampler, input.uv);	
	else if(matId == 18)
		output.color = g_texture18.Sample(g_sampler, input.uv);	
	else if(matId == 19)
		output.color = g_texture19.Sample(g_sampler, input.uv);	
	else if(matId == 20)
		output.color = g_texture20.Sample(g_sampler, input.uv);	
	else if(matId == 21)
		output.color = g_texture21.Sample(g_sampler, input.uv);	
	else if(matId == 22)
		output.color = g_texture22.Sample(g_sampler, input.uv);	
	else if(matId == 23)
		output.color = g_texture23.Sample(g_sampler, input.uv);	
	else if(matId == 24)
		output.color = g_texture24.Sample(g_sampler, input.uv);	
	else if(matId == 25)
		output.color = g_texture25.Sample(g_sampler, input.uv);	
	
	return output;
}