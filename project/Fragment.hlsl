Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

struct PS_IN
{
	float4 Pos_world : POSITION;
	float4 Pos : SV_POSITION;
	//float3 Color : COLOR;
	float2 texCoord : TEXCOORD;
	float4 normal : NORMAL;
};

float4 PS_main(PS_IN input) : SV_Target
{
	float3 lightPos = float3(0.f, 0.f, -2.f); // CamPos
	float3 lightRay = normalize(lightPos - input.Pos_world.xyz);

	float lightIntensity = dot(input.normal, lightRay);

	// Sample the pixel color
	return shaderTexture.Sample(SampleType, input.texCoord) * lightIntensity;

	//return float4(input.Color, 1.0f);
};