#include "ibl_common.hlsli"
#include "ibl_cbuffer.hlsli"

struct PSInput
{
	float4 positionCS : SV_Position;
	float3 normal     : NORMAL;
	float2 texcoord   : TEXCOORD;
	float3 positionWS : POSITIONWS;
	uint   instanceID : SV_InstanceID;
};

TextureCube       g_lightProbe           : register(t0);
Texture2D<float2> g_dfgLUT               : register(t1);
TextureCube       g_ldCube               : register(t2);
TextureCube       g_lightProbeIrradiance : register(t3);
SamplerState      g_linearSampler        : register(s0);

 // Roughness -> mip func
float SelectLDMipmap(float Roughness)
{
	return lerp(0, IBL_LD_MIPMAPS - 1, Roughness);
}

float2 DFGLookup(float Roughness, float NoV)
{
	return g_dfgLUT.Sample(g_linearSampler, float2(Roughness, NoV));
}

float3 ApproximateSpecularIBL( float3 SpecularColor , float Roughness, float3 N, float3 V )
{

	float NoV = saturate( dot( N, V ) );
	float3 R = 2 * dot( V, N ) * N - V;
	
	float mipmap = SelectLDMipmap(Roughness);
	
	float3 PrefilteredColor = g_ldCube.SampleLevel(g_linearSampler, R, mipmap).rgb;
	float2 EnvBRDF          = DFGLookup(Roughness, NoV);
	
	return PrefilteredColor * ( SpecularColor * EnvBRDF.x + EnvBRDF.y );
	
}

float4 iblMeshPS(PSInput input) : SV_Target0
{

	float3 N = input.normal;
	float3 V = normalize(g_cameraPosition - input.positionWS);
	
	float NoV = saturate(dot(N, V));
	
	float4 color;
	
	float  Roughness = (float) input.instanceID / (IBL_INSTANCES_COUNT - 1);
	
	float2 DFGterms   = DFGLookup(Roughness, NoV);
	float3 irradiance = g_lightProbeIrradiance.Sample(g_linearSampler, N).rgb;
	
	float3 diffuse = (g_matF0 * DFGterms.x + g_matF90 * DFGterms.y) * g_matBaseColor;
	float  mipmap  = SelectLDMipmap(Roughness);
	
	float3 specular = diffuse * ApproximateSpecularIBL(g_matF0, Roughness, N, V);
	
	color.rgb = lerp(diffuse * irradiance, specular, g_matMetallic);
	color.a   = 1;
	
	return color;

}