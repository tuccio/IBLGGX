#ifndef __IBL_COMMON__
#define __IBL_COMMON__

#define PI     3.14159265f
#define INV_PI .318309886f

// http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
float GGX(float a, float NoV)
{
	float k = a * .5f;
	return NoV / (NoV * (1 - k) + k);
}

// http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
float G_Smith ( float Roughness, float NoV, float NoL )
{
	float a = Roughness * Roughness;
	return GGX(a, NoV) * GGX(a, NoL);
}

float radicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
 
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), radicalInverse_VdC(i));
}
 
float3 ImportanceSampleGGX( float2 Xi, float Roughness, float3 N )
{

	float a = Roughness * Roughness;
	
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a * a - 1) * Xi.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );
	
	float3 H;
	
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	
	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize( cross( UpVector, N ) );
	float3 TangentY = cross( N, TangentX );
	
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
	
}

float2 IntegrateBRDF( float Roughness, float NoV )
{

	float3 V;
	V.x = sqrt( 1.0f - NoV * NoV ); // sin
	V.y = 0;
	V.z = NoV; // cos
	
	float3 N = { 0, 0, 1 };
	
	float A = 0;
	float B = 0;
	
	const uint NumSamples = 1024;
	
	for( uint i = 0; i < NumSamples; i++ )
	{
	
		float2 Xi = Hammersley( i, NumSamples );
		float3 H = ImportanceSampleGGX( Xi, Roughness, N );
		float3 L = 2 * dot( V, H ) * H - V;
		
		float NoL = saturate( L.z );
		float NoH = saturate( H.z );
		float VoH = saturate( dot( V, H ) );
		
		if( NoL > 0 )
		{
		
			float G = G_Smith( Roughness, NoV, NoL );
			float G_Vis = G * VoH / (NoH * NoV);
			float Fc = pow( 1 - VoH, 5 );
			
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
			
		}
		
	}
	
	return float2( A, B ) / NumSamples;
	
}

#endif
