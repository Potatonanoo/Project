cbuffer ContantBuffer : register(b0)
{
	float4x4 WorldMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

struct GS_IN
{
	float4 pos		: POSITION;
	float2 texCoord : TEXCOORD;
	float4 normal	: NORMAL;
};

struct GS_OUT
{
	float4 pos_SV	: SV_POSITION;
	float4 pos_WS	: POSITION;
	float2 texCoord : TEXCOORD;
	float4 normal	: NORMAL;
};

[maxvertexcount(6)] // Max number of vertices
void GS_main( triangle GS_IN IN[3], inout TriangleStream< GS_OUT > output )
{
	//Primitive center
	for (int i = 0; i < 3; i++)
	{
		GS_OUT element;
		element.pos_SV = IN[i].pos;

		element.pos_SV = mul(WorldMatrix, element.pos_SV);
		element.pos_WS = element.pos_SV;
		element.pos_SV = mul(ViewMatrix, element.pos_SV);
		element.pos_SV = mul(ProjectionMatrix, element.pos_SV);

		element.texCoord = IN[i].texCoord;

		element.normal = mul(WorldMatrix, IN[i].normal);

		output.Append(element);
	}
	output.RestartStrip();
}