#pragma once

#include <DirectXMath.h>
#include <DirectXTex.h>

#include <vector>

#define IBL_LD_MIPMAPS 6

// Don't mind this, it's for the shader macro defines
#define IBL_PP_STRINGIZE(E) #E
#define IBL_PP_EXPAND_STRINGIZE(E) IBL_PP_STRINGIZE(E)

#define IBL_COM_RELEASE(x) { if (x) { x->Release(); x = nullptr; } }

#define IBL_LD_MIPMAPS_STRING IBL_PP_EXPAND_STRINGIZE(IBL_LD_MIPMAPS)

class IBLIntegrator
{

public:

	IBLIntegrator(ID3D11Device * device, int resDFG, int resLD);
	IBLIntegrator(const IBLIntegrator &) = delete;

	~IBLIntegrator(void);

	bool IntegrateDFG(ID3D11Device * device, ID3D11DeviceContext * context, ID3D11Texture2D * & dfgLUT, ID3D11ShaderResourceView * & dfgLUTSRV, bool allocate = true);
	bool IntegrateLD(ID3D11Device * device, ID3D11DeviceContext * context, ID3D11ShaderResourceView * lightProbe, ID3D11Texture2D * & ldCube, ID3D11ShaderResourceView * & ldCubeSRV, bool allocate = true);

private:

	int m_resDFG;
	int m_resLD;

	ID3D11ComputeShader  * m_dfgIntegrator;
	ID3D11VertexShader   * m_ldIntegratorVS;
	ID3D11GeometryShader * m_ldIntegratorGS;
	ID3D11PixelShader    * m_ldIntegratorPS[IBL_LD_MIPMAPS];

};

bool LoadHDRFromFile(const char * filename, int & width, int & height, std::vector<float> & data);