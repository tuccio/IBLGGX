#ifndef __IBL_CBUFFER__
#define __IBL_CBUFFER__

cbuffer cbPerFrame : register(b0)
{
	float3   g_cameraPosition;
	float4x4 g_cameraWorld;
	float4x4 g_cameraView;
	float4x4 g_cameraViewProj;
	float4x4 g_cameraInvViewProj;
};

cbuffer cbPerObject : register(b1)
{
	float4x4 g_world;
	float4x4 g_worldView;
	float4x4 g_worldViewProj;
	float3   g_matBaseColor;
	float    g_matF0;
	float    g_matF90;
	float    g_matMetallic;
};

#endif