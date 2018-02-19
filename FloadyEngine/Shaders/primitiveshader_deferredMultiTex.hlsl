
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
Texture2D g_texture33 : register(t33);
Texture2D g_texture34 : register(t34);
Texture2D g_texture35 : register(t35);
Texture2D g_texture36 : register(t36);
Texture2D g_texture37 : register(t37);
Texture2D g_texture38 : register(t38);
Texture2D g_texture39 : register(t39);
Texture2D g_texture40 : register(t40);
Texture2D g_texture41 : register(t41);
Texture2D g_texture42 : register(t42);
Texture2D g_texture43 : register(t43);
Texture2D g_texture44 : register(t44);
Texture2D g_texture45 : register(t45);
Texture2D g_texture46 : register(t46);
Texture2D g_texture47 : register(t47);
Texture2D g_texture48 : register(t48);
Texture2D g_texture49 : register(t49);
Texture2D g_texture50 : register(t50);
Texture2D g_texture51 : register(t51);
Texture2D g_texture52 : register(t52);
Texture2D g_texture53 : register(t53);
Texture2D g_texture54 : register(t54);
Texture2D g_texture55 : register(t55);
Texture2D g_texture56 : register(t56);
Texture2D g_texture57 : register(t57);
Texture2D g_texture58 : register(t58);
Texture2D g_texture59 : register(t59);
Texture2D g_texture60 : register(t60);
Texture2D g_texture61 : register(t61);
Texture2D g_texture62 : register(t62);
Texture2D g_texture63 : register(t63);


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
	//result.normal = (result.normal + float4(1, 1, 1,1))/2.0f;
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
	if(matId == 0)
		output.color = g_texture0.Sample(g_sampler, input.uv);
	else if(matId == 1)
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
	else if(matId == 26)
		output.color = g_texture26.Sample(g_sampler, input.uv);	
	
	float4 normal = input.normal;	
	
	if(input.normalmatId != 99)
	{
		int normalMatId = input.normalmatId + 1;

		if(normalMatId == 0)
			normal = g_texture32.Sample(g_sampler, input.uv);
		else if(normalMatId == 1)
			normal = g_texture33.Sample(g_sampler, input.uv);	
		else if(normalMatId == 2)
			normal = g_texture34.Sample(g_sampler, input.uv);	
		else if(normalMatId == 3)
			normal = g_texture35.Sample(g_sampler, input.uv);	
		else if(normalMatId == 4)
			normal = g_texture36.Sample(g_sampler, input.uv);	
		else if(normalMatId == 5)
			normal = g_texture37.Sample(g_sampler, input.uv);	
		else if(normalMatId == 6)
			normal = g_texture38.Sample(g_sampler, input.uv);	
		else if(normalMatId == 7)
			normal = g_texture39.Sample(g_sampler, input.uv);
		else if(normalMatId == 8)
			normal = g_texture40.Sample(g_sampler, input.uv);	
		else if(normalMatId == 9)
			normal = g_texture41.Sample(g_sampler, input.uv);	
		else if(normalMatId == 10)
			normal = g_texture42.Sample(g_sampler, input.uv);	
		else if(normalMatId == 11)
			normal = g_texture43.Sample(g_sampler, input.uv);	
		else if(normalMatId == 12)
			normal = g_texture44.Sample(g_sampler, input.uv);	
		else if(normalMatId == 13)
			normal = g_texture45.Sample(g_sampler, input.uv);	
		else if(normalMatId == 14)
			normal = g_texture46.Sample(g_sampler, input.uv);	
		else if(normalMatId == 15)
			normal = g_texture47.Sample(g_sampler, input.uv);	
		else if(normalMatId == 16)
			normal = g_texture48.Sample(g_sampler, input.uv);	
		else if(normalMatId == 17)
			normal = g_texture49.Sample(g_sampler, input.uv);	
		else if(normalMatId == 18)
			normal = g_texture50.Sample(g_sampler, input.uv);	
		else if(normalMatId == 19)
			normal = g_texture51.Sample(g_sampler, input.uv);	
		else if(normalMatId == 20)
			normal = g_texture51.Sample(g_sampler, input.uv);	
		else if(normalMatId == 21)
			normal = g_texture53.Sample(g_sampler, input.uv);	
		else if(normalMatId == 22)
			normal = g_texture54.Sample(g_sampler, input.uv);	
		else if(normalMatId == 23)
			normal = g_texture55.Sample(g_sampler, input.uv);	
		else if(normalMatId == 24)
			normal = g_texture56.Sample(g_sampler, input.uv);	
		else if(normalMatId == 25)
			normal = g_texture57.Sample(g_sampler, input.uv);
		
		output.normal = normalize(input.normal + normal);
	}	
	
	output.normal = (output.normal + float4(1, 1, 1,1))/2.0f;
	
	//output.normal = normalize(input.normal + float4(0, normal.y, 0, 0));	
	//output.normal = normalize(float4(input.normal.x, (normal.y), input.normal.z, 0));	
	//output.color = normal;
	//output.color = output.normal;
	
	//if(matId == 20)
	//output.color = float4(1,0,0,1);
	
	return output;
}