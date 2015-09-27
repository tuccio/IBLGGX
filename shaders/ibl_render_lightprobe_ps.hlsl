#include "ibl_cbuffer.hlsli"

struct PSInput
{
	float4 positionCS  : SV_Position;
	float2 texcoord    : TEXCOORD;
	float4 positionNDC : positionNDC;
};

TextureCube  g_lightProbe    : register(t0);
SamplerState g_linearSampler : register(s0);

float4 iblLightProbePS(PSInput input) : SV_Target0
{

	float4 x = mul(input.positionNDC, g_cameraInvViewProj);
	float3 rayDir = normalize(x.xyz / x.w - g_cameraPosition);

	float3 color = g_lightProbe.Sample(g_linearSampler, rayDir);
	
	return float4(color, 1);

}