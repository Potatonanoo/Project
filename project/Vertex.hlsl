struct VS_IN
{
	float3 Pos : POSITION;
	//float3 Color : COLOR;
	float2 texCoord : TEXCOORD;
};

struct VS_OUT
{
	float4 Pos : SV_POSITION;
	//float3 Color : COLOR;
	float2 texCoord : TEXCOORD;
};
//-----------------------------------------------------------------------------------------
// VertexShader: VSScene
//-----------------------------------------------------------------------------------------
VS_OUT VS_main(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;

	output.Pos = float4(input.Pos, 1);
	output.texCoord = input.texCoord;

	return output;
}