Texture2D shaderTexture		: register( t0 );
SamplerState SampleType		: register( s0 );

struct PS_IN
{
	float4 pos_SV	: SV_POSITION;
	float4 pos_WS	: POSITION;
	float2 texCoord : TEXCOORD;
	float4 normal	: NORMAL;
};

struct PS_OUT
{
	float4 normal			: SV_Target0;
	float4 diffuse			: SV_Target1;
	float4 pos				: SV_Target2;
};

PS_OUT PS_main( in PS_IN input)
{
	PS_OUT output;

	float4 normal = normalize( input.normal );

	// Output G-buffer values
	output.normal = normal;
	output.diffuse = shaderTexture.Sample(SampleType, input.texCoord);
	output.pos = input.pos_WS;

	return output;
};