#include "ibl_cbuffer.hlsli"

struct VSInput
{
	float3 position   : SV_Position;
	float3 normal     : NORMAL;
	float2 texcoord   : TEXCOORD;
	uint   instanceID : SV_InstanceID;
};

struct VSOutput
{
	float4 positionCS : SV_Position;
	float3 normal     : NORMAL;
	float2 texcoord   : TEXCOORD;
	float3 positionWS : POSITIONWS;
	uint   instanceID : SV_InstanceID;
};

VSOutput iblMeshVS(VSInput input)
{

	VSOutput output;
	
	float t = 2.5f * ((float) input.instanceID - (IBL_INSTANCES_COUNT >> 1));
	
	float4x4 world = g_world;
	
	world._41 = t;
	
	float4x4 worldViewProj = mul(world, g_cameraViewProj);
	
	output.positionCS = mul(float4(input.position, 1), worldViewProj);
	output.normal     = input.normal;
	output.texcoord   = input.texcoord;
	output.positionWS = mul(float4(input.position, 1), world);
	output.instanceID = input.instanceID;
	
	return output;

}