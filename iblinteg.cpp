#include "iblinteg.hpp"
#include "rgbe.h"

#include "dxutils.hpp"

using namespace DirectX;

// Number of threads in a group per dimension (grid is bidimensional)
#define IBL_DFG_GROUP_THREADS 16

#define IBL_DFG_GROUP_THREADS_STRING IBL_PP_EXPAND_STRINGIZE(IBL_DFG_GROUP_THREADS)
#define IBL_LD_GROUP_THREADS_STRING  IBL_PP_EXPAND_STRINGIZE(IBL_LD_GROUP_THREADS)

bool LoadHDRFromFile(const char * filename, int & width, int & height, std::vector<float> & data)
{

	FILE * f = fopen(filename, "rb");

	if (f)
	{

		rgbe_header_info info;

		if (!RGBE_ReadHeader(f, &width, &height, &info))
		{

			data.resize(3 * width * height);

			if (!RGBE_ReadPixels_RLE(f, &data[0], width, height))
			{
				return true;
			}

		}

	}

	return false;

}

IBLIntegrator::IBLIntegrator(ID3D11Device * device, int resDFG, int resLD) :
	m_resDFG(resDFG),
	m_resLD(resLD)
{

	std::string invDFG = std::to_string(1.f / resDFG);

	D3D_SHADER_MACRO dfgDefines[] = {
		{ "IBL_DFG_GROUP_THREADS", IBL_DFG_GROUP_THREADS_STRING },
		{ "IBL_DFG_STEP", invDFG.c_str() },
		{ "IBL_DFG_INTEGRATOR", "" },
		{ nullptr, nullptr }
	};

	if (!tdx::CompileShader(m_dfgIntegrator, device, "./shaders/ibl_integrator.hlsl", "iblDFGIntegratorCS", dfgDefines))
	{
		m_dfgIntegrator = nullptr;
	}

	if (!tdx::CompileShader(m_ldIntegratorVS, device, "./shaders/ibl_integrator.hlsl", "iblLDIntegratorVS"))
	{
		m_ldIntegratorVS = nullptr;
	}

	if (!tdx::CompileShader(m_ldIntegratorGS, device, "./shaders/ibl_integrator.hlsl", "iblLDIntegratorGS"))
	{
		m_ldIntegratorGS = nullptr;
	}

	D3D_SHADER_MACRO ldPSDefines[] = {
		{ "IBL_LD_ROUGHNESS", nullptr },
		{ "IBL_LD_INTEGRATOR_PS", "" },
		{ nullptr, nullptr }
	};

	for (int i = 0; i < IBL_LD_MIPMAPS; i++)
	{

		std::string roughness = std::to_string((float) i / (IBL_LD_MIPMAPS - 1));

		ldPSDefines[0].Definition = roughness.c_str();

		if (!tdx::CompileShader(m_ldIntegratorPS[i], device, "./shaders/ibl_integrator.hlsl", "iblLDIntegratorPS", ldPSDefines))
		{
			m_ldIntegratorPS[i] = nullptr;
		}

	}

	assert(m_dfgIntegrator && m_ldIntegratorVS && m_ldIntegratorGS && m_ldIntegratorPS[0] && "IBLIntegrator compilation failed");

}

IBLIntegrator::~IBLIntegrator(void)
{

	IBL_COM_RELEASE(m_dfgIntegrator);
	IBL_COM_RELEASE(m_ldIntegratorVS);
	IBL_COM_RELEASE(m_ldIntegratorGS);

	for (int i = 0; i < IBL_LD_MIPMAPS; i++)
	{
		IBL_COM_RELEASE(m_ldIntegratorPS[i]);
	}

}

bool IBLIntegrator::IntegrateDFG(ID3D11Device * device, ID3D11DeviceContext * context, ID3D11Texture2D * & dfgLUT, ID3D11ShaderResourceView * & dfgLUTSRV, bool allocate)
{

	ID3D11UnorderedAccessView * uav = nullptr;

	if (allocate)
	{


		D3D11_TEXTURE2D_DESC t2dDesc;

		t2dDesc.ArraySize      = 1;
		t2dDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		t2dDesc.CPUAccessFlags = 0;
		t2dDesc.Format         = DXGI_FORMAT_R16G16_FLOAT;
		t2dDesc.Width          = m_resDFG;
		t2dDesc.Height         = m_resDFG;
		t2dDesc.MipLevels      = 1;
		t2dDesc.MiscFlags      = 0;
		t2dDesc.SampleDesc     = { 1, 0 };
		t2dDesc.Usage          = D3D11_USAGE_DEFAULT;

		dfgLUT    = nullptr;
		dfgLUTSRV = nullptr;

		if (FAILED(device->CreateTexture2D(&t2dDesc, nullptr, &dfgLUT)) ||
		    FAILED(device->CreateUnorderedAccessView(dfgLUT, nullptr, &uav)) ||
		    FAILED(device->CreateShaderResourceView(dfgLUT, nullptr, &dfgLUTSRV)))
		{

			IBL_COM_RELEASE(dfgLUT);
			IBL_COM_RELEASE(dfgLUTSRV);
			IBL_COM_RELEASE(uav);

			return false;

		}

	}
	
	context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

	int numGroups = m_resDFG / IBL_DFG_GROUP_THREADS;

	context->CSSetShader(m_dfgIntegrator, nullptr, 0);
	context->Dispatch(numGroups, numGroups, 1);

	IBL_COM_RELEASE(uav);
	context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

	return true;

}

bool IBLIntegrator::IntegrateLD(ID3D11Device * device, ID3D11DeviceContext * context, ID3D11ShaderResourceView * lightProbe, ID3D11Texture2D * & ldCube, ID3D11ShaderResourceView * & ldCubeSRV, bool allocate)
{

	if (allocate)
	{

		D3D11_TEXTURE2D_DESC t2dDesc;

		t2dDesc.ArraySize      = 6;
		t2dDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		t2dDesc.CPUAccessFlags = 0;
		t2dDesc.Format         = DXGI_FORMAT_R16G16B16A16_FLOAT;
		t2dDesc.Width          = m_resLD;
		t2dDesc.Height         = m_resLD;
		t2dDesc.MipLevels      = IBL_LD_MIPMAPS;
		t2dDesc.MiscFlags      = D3D11_RESOURCE_MISC_TEXTURECUBE;
		t2dDesc.SampleDesc     = { 1, 0 };
		t2dDesc.Usage          = D3D11_USAGE_DEFAULT;

		

		ldCube    = nullptr;
		ldCubeSRV = nullptr;

		if (FAILED(device->CreateTexture2D(&t2dDesc, nullptr, &ldCube)) ||
			FAILED(device->CreateShaderResourceView(ldCube, nullptr, &ldCubeSRV)))
		{

			IBL_COM_RELEASE(ldCube);
			IBL_COM_RELEASE(ldCubeSRV);

			return false;

		}

	}
	
	ID3D11RenderTargetView * rtv[IBL_LD_MIPMAPS] = { 0 };

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;

	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Format        = DXGI_FORMAT_R16G16B16A16_FLOAT;

	rtvDesc.Texture2DArray.FirstArraySlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 6;

	bool success = true;

	for (int i = 0; i < IBL_LD_MIPMAPS; i++)
	{
		rtvDesc.Texture2DArray.MipSlice = i;
		success = success && !FAILED(device->CreateRenderTargetView(ldCube, &rtvDesc, rtv + i));
	}

	if (success)
	{

		D3D11_VIEWPORT viewport;

		viewport.Height   = m_resLD;
		viewport.Width    = m_resLD;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;

		context->VSSetShader(m_ldIntegratorVS, nullptr, 0);
		context->GSSetShader(m_ldIntegratorGS, nullptr, 0);

		context->PSSetShaderResources(0, 1, &lightProbe);

		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		for (int i = 0; i < IBL_LD_MIPMAPS; i++)
		{

			context->PSSetShader(m_ldIntegratorPS[i], nullptr, 0);

			context->RSSetViewports(1, &viewport);
			context->OMSetRenderTargets(1, rtv + i, nullptr);

			context->DrawInstanced(4, 6, 0, 0);

			viewport.Height *= .5f;
			viewport.Width   = viewport.Height;

		}

		context->VSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);

	}

	for (int i = 0; i < IBL_LD_MIPMAPS; i++)
	{
		IBL_COM_RELEASE(rtv[i]);
	}

	context->OMSetRenderTargets(0, rtv, nullptr);
	
	return success;

}