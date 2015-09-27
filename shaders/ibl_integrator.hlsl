#include "ibl_common.hlsli"

#define IBL_SPECULAR .04f

TextureCube         g_lightProbe : register(t0);
RWTexture2D<float2> g_dfgLUT     : register(u0);
RWTexture3D<float>  g_ldCube     : register(u1);

SamplerState        g_linearSampler : register(s0);

#ifdef IBL_DFG_INTEGRATOR

[numthreads(IBL_DFG_GROUP_THREADS, IBL_DFG_GROUP_THREADS, 1)]
void iblDFGIntegratorCS(uint3 dispatchID : SV_DispatchThreadID)
{

	float2 cr = (dispatchID.xy + .5f) * IBL_DFG_STEP;
	
	g_dfgLUT[dispatchID.xy] = IntegrateBRDF(cr.x, cr.y);

}

#endif

struct VSInput
{
	uint   vertexID   : SV_VertexID;
	uint   instanceID : SV_InstanceID;
};

struct GSInput
{
	float4 positionCS : SV_Position;
	uint   instanceID : SV_InstanceID;
	float2 texcoord   : TEXCOORD;
	float3 direction  : DIRECTION;
};

struct PSInput
{
	float4 positionCS : SV_Position;
	uint   cubeFace   : SV_RenderTargetArrayIndex;
	float2 texcoord   : TEXCOORD;
	float3 direction  : DIRECTION;
};

GSInput iblLDIntegratorVS(VSInput input)
{

	GSInput output;
	
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
	
	output.direction  = float3(output.positionCS.xy, 1);
	output.instanceID = input.instanceID;

	return output;

}

[maxvertexcount(3)]
void iblLDIntegratorGS(triangle GSInput input[3], inout TriangleStream<PSInput> output )
{

	PSInput element;
	
	element.cubeFace = input[0].instanceID;
	
	[unroll]
	for (int i = 0; i < 3; i++)
	{

		element.positionCS = input[i].positionCS;
		element.texcoord   = input[i].texcoord;
		element.direction  = input[i].direction;
		
		output.Append(element);

	}

}

#ifdef IBL_LD_INTEGRATOR_PS

float3 PrefilterEnvMap( float Roughness, float3 R )
{

	float3 N = R;
	float3 V = R;
	
	float3 PrefilteredColor = 0;
	
	const uint NumSamples = 1024;
	
	float TotalWeight = 0.f;
	
	for( uint i = 0; i < NumSamples; i++ )
	{
	
		float2 Xi = Hammersley( i, NumSamples );
		float3 H = ImportanceSampleGGX( Xi, Roughness, N );
		float3 L = 2 * dot( V, H ) * H - V;
		
		float NoL = saturate( dot( N, L ) );
		
		if( NoL > 0 )
		{
			PrefilteredColor += g_lightProbe.SampleLevel( g_linearSampler , L, 0 ).rgb * NoL;
			TotalWeight += NoL;
		}
		
	}
	
	return (TotalWeight > 0.f ? PrefilteredColor / TotalWeight : PrefilteredColor);
	
}

float4 iblLDIntegratorPS(PSInput input) : SV_Target0
{

	float3x3 orientation[] = {
	
	// x

		{
			 0,  0, -1,
			 0,  1,  0,
			 1,  0,  0
		},

	// -x

		{
			 0,  0,  1,
			 0,  1,  0,
			-1,  0,  0
		},
		
	// y

		{
			 1,  0,  0,
			 0,  0, -1,
			 0,  1,  0
		},

	// -y

		{
			 1,  0,  0,
			 0,  0,  1,
			 0, -1,  0
		},
	
	// z

		{
			 1,  0,  0,
			 0,  1,  0,
			 0,  0,  1
		},

	// -z

		{
			-1,  0,  0,
			 0,  1,  0,
			 0,  0, -1
		}
		
	};

	
	float3 direction = mul(normalize(input.direction), orientation[input.cubeFace]);
	
	return float4(PrefilterEnvMap(IBL_LD_ROUGHNESS, direction), 1);
	
}

#endif
