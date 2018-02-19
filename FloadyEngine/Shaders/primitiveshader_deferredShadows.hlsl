
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
	float4x4 myLightProjMatrix[16];
	float4x4 myViewMatrixInstanced[16];
};

struct InstanceData
{
	uint myLightId;
};

ConstantBuffer<MyData> myData : register(b0);
ConstantBuffer<InstanceData> myInstanceData : register(b1);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, uint InstanceId : SV_InstanceID)
{
	PSInput result;
	result.position = position;
	result.position = mul(result.position, myData.myViewMatrixInstanced[InstanceId]);
	result.position = mul(result.position, myData.myLightProjMatrix[myInstanceData.myLightId]); // todo: send light transform id 
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	output.color = float4(1.0f, 1.0f, 0.1f, 1.0f);	
	return output;
}