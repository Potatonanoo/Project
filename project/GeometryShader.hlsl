/*
Albin Liljegren
PAACI16
970416-1117
*/

cbuffer BUFFER : register(b0)
{
	float4x4 world;
	float4x4 view;
	float4x4 projection;
};

struct GS_IN
{
	float4 Pos : SV_POSITION;
	//float3 Color : COLOR;
	float2 texCoord : TEXCOORD;
};

struct GS_OUT
{
	float4 Pos_world : POSITION;
	float4 Pos : SV_POSITION;
	//float3 Color : COLOR;
	float2 texCoord : TEXCOORD;
	float4 normal : NORMAL;
};

[maxvertexcount(6)] // Max number of vertices
void GS_main( triangle GS_IN IN[3], inout TriangleStream< GS_OUT > output )
{
	float3 normal;
	normal = cross(IN[1].Pos.xyz - IN[0].Pos.xyz, IN[2].Pos.xyz - IN[0].Pos.xyz);
	normal = normalize(normal);
	float4 normal4 = float4(normal, 0.0f);

	//Primitive center
	for (int i = 0; i < 3; i++)
	{
		GS_OUT element;
		element.Pos = IN[i].Pos;

		element.Pos = mul(world, element.Pos);
		element.Pos_world = element.Pos;
		element.Pos = mul(view, element.Pos);
		element.Pos = mul(projection, element.Pos);

		//element.Color = IN[i].Color;
		element.texCoord = IN[i].texCoord;

		element.normal = mul(world, normal4);

		output.Append(element);
	}

	output.RestartStrip();

	//Primitive infront
	for (int i = 0; i < 3; i++)
	{
		GS_OUT element;
		element.Pos = IN[i].Pos + normal4;

		element.Pos = mul(world, element.Pos);
		element.Pos_world = element.Pos;
		element.Pos = mul(view, element.Pos);
		element.Pos = mul(projection, element.Pos);

		//element.Color = IN[i].Color;
		element.texCoord = IN[i].texCoord;

		element.normal = mul(world, normal4);

		output.Append(element);
	}
}