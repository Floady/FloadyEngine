
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
Texture2D<float> g_shadowTexture : register(t3);
Texture2D<float4> g_combinedTexture : register(t4);
Texture2D<float4> scratchbuff : register(t5);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL, float4 uv : TEXCOORD)
{
	PSInput result;
	result.position = float4(position.xyz, 1);
	result.uv = uv.xy;
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{   
	PSOutput output;
	
	float2 uvStride = float2(1.0f/1600.0f, 1.0f/900.0f);
		
	output.color = scratchbuff.Sample(g_sampler, input.uv);
	
//	if(output.color.x > 1.0f || output.color.y > 1.0f || output.color.z > 1.0f)	
//	{
//		output.color = float4(1.0f, 1.0f, 0.0f, 0.0f);	
//		return output;
//	}
	
	bool doBlur = false;

	float3 dir = float3(0,0,0); // this is the direction across we want to blur after luma edge analysis
	float3 normalizedDir = float3(1,0,0);
	bool doFXAA = true;
	if(doFXAA)
	{
		// brightness
		float maxLuma = 0.0f; 
		float minLuma = 10000.0f; 
		float currentPixLuma = 0.0f; 
		
		// get all neighbour colors
		float4 colorM = g_combinedTexture.Sample(g_sampler, input.uv);
		float4 colorN = g_combinedTexture.Sample(g_sampler, input.uv - float2(0, uvStride.y));
		float4 colorS = g_combinedTexture.Sample(g_sampler, input.uv + float2(0, uvStride.y));
		float4 colorE = g_combinedTexture.Sample(g_sampler, input.uv + float2(uvStride.x, 0));
		float4 colorW = g_combinedTexture.Sample(g_sampler, input.uv - float2(uvStride.x, 0));
		float4 colorNW = g_combinedTexture.Sample(g_sampler, input.uv - float2(uvStride.x, uvStride.y));
		float4 colorNE = g_combinedTexture.Sample(g_sampler, input.uv + float2(uvStride.x, -uvStride.y));
		float4 colorSE = g_combinedTexture.Sample(g_sampler, input.uv + float2(uvStride.x, uvStride.y));
		float4 colorSW = g_combinedTexture.Sample(g_sampler, input.uv + float2(uvStride.x, -uvStride.y));
		
		// for calculating Luma, currently using perceived brightness weights
		float3 weights = float3(0.299, 0.587, 0.114); 
		
		// Get 2x2 Boxed filtered values and calculate max and min Luma
		float4 filteredColorNW = (colorN + colorM + colorW + colorNW) / 4.0;
		float lumaNW = weights.x*filteredColorNW.x + weights.y*filteredColorNW.y + weights.z*filteredColorNW.z;
		maxLuma = max(maxLuma, lumaNW); 
		minLuma = min(minLuma, lumaNW);
		
		float4 filteredColorNE = (colorN + colorM + colorNE + colorE) / 4.0;
		float lumaNE = weights.x*filteredColorNE.x + weights.y*filteredColorNE.y + weights.z*filteredColorNE.z;
		maxLuma = max(maxLuma, lumaNE); 
		minLuma = min(minLuma, lumaNE);
		
		float4 filteredColorSE = (colorM + colorS + colorSE + colorE) / 4.0;
		float lumaSE = weights.x*filteredColorSE.x + weights.y*filteredColorSE.y + weights.z*filteredColorSE.z;
		maxLuma = max(maxLuma, lumaSE); 
		minLuma = min(minLuma, lumaSE);
		
		float4 filteredColorSW = (colorM + colorS + colorSW + colorW) / 4.0;
		float lumaSW = weights.x*filteredColorSW.x + weights.y*filteredColorSW.y + weights.z*filteredColorSW.z;
		maxLuma = max(maxLuma, lumaSW); 
		minLuma = min(minLuma, lumaSW);		
		
		// Check contrast for pixel
		currentPixLuma = weights.x*colorM.x + weights.y*colorM.y + weights.z*colorM.z;
		float contrast = max(maxLuma, currentPixLuma) - min(minLuma, currentPixLuma);
		float minThreshold = 0.05f;
		float threshold = 0.4f;
		
		// this is the early out test - if pixel should get blurred or not, if so - calculate the direction and do a blur pass
		if(contrast > max(minThreshold, maxLuma * threshold))
		{
			dir.x = -((lumaNW+lumaNE) - (lumaSW+lumaSE));
			dir.y = ((lumaNW+lumaSW) - (lumaNE+lumaSE));
			dir.z = 0;
			normalizedDir = normalize(dir);
			
			doBlur = true;
			
			// debug draw luma edge detection
			//output.color = float4(abs(normalizedDir.x), abs(normalizedDir.y), 0.0f, 1.0f);
			/*
			if(normalizedDir.x > 0.8f)
				output.color = float4(0,0,1,1);
			else
				output.color = float4(1,0,0,1);
				
			output.color = float4(normalizedDir.xy / 2 + float2(0.5f, 0.5f), 0, 1);
			return output;
			//*/
		}
		
	}
	
	// FXAA blur over direction
	if(doBlur)
	{
		float4 totalColor = float4(0, 0, 0, 0); 
		const float maxBlurDist = 8.0f;
		float sharpness = 1.5f;
		float minDir = min(normalizedDir.x, normalizedDir.y) * sharpness;
		float2 newDir = normalizedDir.xy * 1.0f/minDir;
		float scale = 8.0f;
		
		totalColor += scratchbuff.Sample(g_sampler, input.uv - uvStride * scale * (normalizedDir.xy));
		totalColor += scratchbuff.Sample(g_sampler, input.uv + uvStride * scale * (normalizedDir.xy));
		
		output.color = totalColor / 2;
		
		// TEST
		//float4 colorFinal = scratchbuff.Sample(g_sampler, input.uv + float2(uvStride.x * dir.x, uvStride.y * dir.y));
		//colorFinal += scratchbuff.Sample(g_sampler, input.uv - float2(uvStride.x * dir.x, uvStride.y * dir.y));
		//output.color = colorFinal / 2.0;
	}	
	
	// tone mapping: gamma correction
	{
		float y = 2.2f;
		float A = 1.0f;
		output.color = A * pow(output.color, y);
	}
	
	// tone mapping: simple linear clamp
	// output.color = output.color / (output.color + 1.0f);
	
	return output;	
}
