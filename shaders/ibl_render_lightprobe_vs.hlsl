#include "ibl_cbuffer.hlsli"

struct VSInput
{
	uint vertexID : SV_VertexID;
};

struct VSOutput
{
	float4 positionCS  : SV_Position;
	float2 texcoord    : TEXCOORD;
	float4 positionNDC : POSITIONNDC;
};

VSOutput iblLightProbeVS(VSInput input)
{

	VSOutput output;

	if (input.vertexID == 0)
	{
		output.positionCS = float4(-1, -1, .5f, 1);
		output.texcoord   = float2(0, 1);
	}
	else if (input.vertexID == 1)
	{
		output.positionCS = float4(-1, 1, .5f, 1);
		output.texcoord   = float2(0, 0);
	}
	else if (input.vertexID == 2)
	{
		output.positionCS = float4(1, -1, .5f, 1);
		output.texcoord   = float2(1, 1);
	}
	else
	{
		output.positionCS = float4(1, 1, .5f, 1);
		output.texcoord   = float2(1, 0);
	}
	
	output.positionNDC = output.positionCS;

	return output;

}