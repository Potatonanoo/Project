// Textures
Texture2D NormalTexture			: register (t0);
Texture2D DiffuseTexture		: register (t1);
texture2D PositionTexture		: register (t2);

// Sampler
SamplerState SampleType	: register(s0);

struct PS_IN
{
	float4 pos			: SV_POSITION;
	float3 normal		: NORMAL;
	float2 texCoord		: TEXCOORD;
};

float4 PS_main( in PS_IN input) : SV_Target0
{
	float3 normal = NormalTexture.Sample(SampleType, input.texCoord).xyz;

	if (length(normal) != 1)
	{
		float3 diffuse = DiffuseTexture.Sample(SampleType, input.texCoord).xyz;
		float3 position = PositionTexture.Sample(SampleType, input.texCoord).xyz;

		float3 lightPos = float3(0.f, 0.f, -1.f);
		float3 lightRay = normalize(lightPos - position);

		float lightIntensity = dot(normal, lightRay);

		return float4(diffuse, 1.0f) * lightIntensity;
	}
	
	return float4(0.f, 0.f, 1.f, 1.f);
}