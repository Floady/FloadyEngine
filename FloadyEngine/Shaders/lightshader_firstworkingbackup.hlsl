
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct PSOutput
{
	float4 color;
};

Texture2D<float4> g_colortexture : register(t0);
Texture2D<float4> g_normaltexture : register(t1);
Texture2D<float> g_depthTexture : register(t2);
SamplerState g_sampler : register(s0);

struct MyData
{
	float4x4 g_invProjMatrix;
    float4 lightPos;
};

ConstantBuffer<MyData> myData : register(b0);
float4 g_constData[4] : register(b1);

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD)
{
	float4 invProjMatrix = g_constData[0];
	PSInput result;

	result.position = float4(position, 1.0f);
	result.uv = uv;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	float4x4 invProjMatrix = myData.g_invProjMatrix;
    const float3 lightPos = float3(0, 0.0, 3.0f);
	PSOutput output;
	
	float4 colors = g_colortexture.Sample(g_sampler, input.uv);
	float4 normals = g_normaltexture.Sample(g_sampler, input.uv);
	
	// get linear depth
	float depth = g_depthTexture.Sample(g_sampler, input.uv);
	float zNear = 100.0;
	float zFar = 1.0;

    //depth = 2.0 * depth - 1.0;
    //depth = 2.0 * zNear * zFar / (zFar + zNear - depth * (zFar - zNear));
	//depth = 1.0f - depth;
	float4 H = float4(input.uv.x * 2 - 1, (1.0f - input.uv.y) * 2 - 1, depth , 1);
	float4 D = mul(H, invProjMatrix); 
	float4 worldPos = D / D.w;
 
	// try get positions
    float3 viewPos = worldPos.xyz;
	float distToLight = length(viewPos - lightPos);
	
	if(distToLight <= 1.1f)
	{
		output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);	
		return output;	
	}
    float3 lightDir = float3(0,-1,0);
	float lightIntensity = 1.0f;

    // Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(normals.xyz, -lightDir));

    // Determine the final amount of diffuse color based on the color of the pixel combined with the light intensity.
    output.color = saturate(colors * lightIntensity);

	//output.color = g_colortexture.Sample(g_sampler, input.uv);
	//output.color = float4(input.uv.x, input.uv.y, 0.0f, 1.0f);
	//output.color = g_normaltexture.Sample(g_sampler, input.uv);
	//output.color = float4(1.0f, 0.0f, 0.1f, 1.0f);
	output.color = float4(depth, depth, depth, 1.0f);	
	//output.color = float4(viewPos.xyz, 1.0f);	
	//output.color = float4(g_constData[0].xyz, 1.0f);	
	//output.color = float4(distToLight, distToLight, distToLight, 1.0f);	
	output.color = float4(worldPos.xyz, 1.0f);
	return output;	
}
