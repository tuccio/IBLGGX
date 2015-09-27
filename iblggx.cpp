#include <DXUT.h>
#include <DXUTcamera.h>

#include <DirectXTex.h>
#include <DirectXMesh.h>

#include <DirectXTK/Effects.h>
#include <DirectXTK/Model.h>

#include "iblinteg.hpp"
#include "dxutils.hpp"

#include <algorithm>

#ifdef IBL_USE_ANTTWEAKBAR

#include <AntTweakBar.h>

#endif

#pragma warning( disable : 4100 )

using namespace DirectX;

#define IBL_INSTANCES_COUNT 11
#define IBL_INSTANCES_COUNT_STRING IBL_PP_EXPAND_STRINGIZE(IBL_INSTANCES_COUNT)

#define IBL_PROBE_SLOT            0
#define IBL_DFG_LUT_SLOT          1
#define IBL_LD_CUBE_SLOT          2
#define IBL_PROBE_IRRADIANCE_SLOT 3

static ID3D11Texture2D          * g_lightProbe;
static ID3D11ShaderResourceView * g_lightProbeSRV;

static ID3D11Texture2D          * g_lightProbeIrradiance;
static ID3D11ShaderResourceView * g_lightProbeIrradianceSRV;

static CFirstPersonCamera         g_camera;

static std::unique_ptr<Model>     g_model;

static std::unique_ptr<EffectFactory> g_effectFactory;

static ID3D11InputLayout  * g_renderMeshIL;
static ID3D11VertexShader * g_renderMeshVS;
static ID3D11PixelShader  * g_renderMeshPS;

static ID3D11VertexShader * g_renderLightProbeVS;
static ID3D11PixelShader  * g_renderLightProbePS;

static ID3D11SamplerState * g_linearSampler;

static ID3D11Buffer * g_cbPerFrame;
static ID3D11Buffer * g_cbPerObject;

static ID3D11DepthStencilState * g_depthOff;
static ID3D11DepthStencilState * g_depthOn;

static std::unique_ptr<IBLIntegrator> g_integrator;

static ID3D11Texture2D          * g_dfgLUT;
static ID3D11ShaderResourceView * g_dfgLUTSRV;

static ID3D11Texture2D          * g_ldCube;
static ID3D11ShaderResourceView * g_ldCubeSRV;

static XMFLOAT3 g_matBaseColor = XMFLOAT3(.8f, .9f, .9);
static float    g_matF0        = .75f;
static float    g_matF90       = .85f;
static float    g_matMetallic  = 1.f;

static int g_dfgLUTResolution = 128;
static int g_ldCubeResolution = 256;

struct cbPerFrame
{
	XMVECTOR cameraPosition;
	XMMATRIX cameraWorld;
	XMMATRIX cameraView;
	XMMATRIX cameraViewProj;
	XMMATRIX cameraInvViewProj;
};

struct cbPerObject
{
	
	XMMATRIX world;
	XMMATRIX worldView;
	XMMATRIX worldViewProj;

	XMFLOAT3 baseColor;
	float    F0;
	float    F90;
	float    metallic;

	float    __fill[2];

};

ID3D11Texture2D * g_iblDFG[IBL_INSTANCES_COUNT];
ID3D11Texture2D * g_lblLD[IBL_INSTANCES_COUNT];


bool LoadDDS(ID3D11Device * pd3dDevice, const wchar_t * filename, ID3D11Texture2D * & tex, ID3D11ShaderResourceView * & srv)
{

	ScratchImage image;

	return !FAILED(LoadFromDDSFile(filename, DDS_FLAGS_NONE, nullptr, image)) &&
	       !FAILED(CreateTextureEx(pd3dDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, (ID3D11Resource **) &tex)) &&
	       !FAILED(CreateShaderResourceView(pd3dDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &srv));

}


bool LoadLightProbe(ID3D11Device * pd3dDevice, ID3D11DeviceContext * pd3dImmediateContext, const wchar_t * probe, const wchar_t * irradiance)
{

	IBL_COM_RELEASE(g_lightProbe);
	IBL_COM_RELEASE(g_lightProbeSRV);

	IBL_COM_RELEASE(g_lightProbeIrradiance);
	IBL_COM_RELEASE(g_lightProbeIrradianceSRV);

	ID3D11ShaderResourceView * nullResource = nullptr;

	pd3dImmediateContext->PSSetShaderResources(IBL_PROBE_SLOT, 1, &nullResource);
	pd3dImmediateContext->PSSetShaderResources(IBL_PROBE_IRRADIANCE_SLOT, 1, &nullResource);

	bool success = LoadDDS(pd3dDevice, probe, g_lightProbe, g_lightProbeSRV) && LoadDDS(pd3dDevice, irradiance, g_lightProbeIrradiance, g_lightProbeIrradianceSRV);

	pd3dImmediateContext->PSSetShaderResources(IBL_PROBE_SLOT, 1, &g_lightProbeSRV);
	pd3dImmediateContext->PSSetShaderResources(IBL_PROBE_IRRADIANCE_SLOT, 1, &g_lightProbeIrradianceSRV);

	return success;

}


bool IntegrateDFG(ID3D11Device * pd3dDevice, ID3D11DeviceContext * pd3dImmediateContext, bool allocate)
{

	if (allocate)
	{
		IBL_COM_RELEASE(g_dfgLUTSRV);
		IBL_COM_RELEASE(g_dfgLUT);
	}

	ID3D11ShaderResourceView * nullResource = nullptr;

	pd3dImmediateContext->PSSetShaderResources(IBL_DFG_LUT_SLOT, 1, &nullResource);
	bool success = g_integrator->IntegrateDFG(pd3dDevice, pd3dImmediateContext, g_dfgLUT, g_dfgLUTSRV, allocate);
	pd3dImmediateContext->PSSetShaderResources(IBL_DFG_LUT_SLOT, 1, &g_dfgLUTSRV);

	return success;

}


bool IntegrateLD(ID3D11Device * pd3dDevice, ID3D11DeviceContext * pd3dImmediateContext, bool allocate)
{

	if (allocate)
	{
		IBL_COM_RELEASE(g_ldCubeSRV);
		IBL_COM_RELEASE(g_ldCube);
	}

	ID3D11ShaderResourceView * nullResource = nullptr;

	pd3dImmediateContext->PSSetShaderResources(IBL_LD_CUBE_SLOT, 1, &nullResource);
	bool success = g_integrator->IntegrateLD(pd3dDevice, pd3dImmediateContext, g_lightProbeSRV, g_ldCube, g_ldCubeSRV);
	pd3dImmediateContext->PSSetShaderResources(IBL_LD_CUBE_SLOT, 1, &g_ldCubeSRV);

	return success;

}


bool CreateILBIntegrator(ID3D11Device * pd3dDevice, ID3D11DeviceContext * pd3dImmediateContext)
{
	g_integrator = std::make_unique<IBLIntegrator>(pd3dDevice, g_dfgLUTResolution, g_ldCubeResolution);
	return IntegrateDFG(pd3dDevice, pd3dImmediateContext, true) && IntegrateLD(pd3dDevice, pd3dImmediateContext, true);
}


bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{

#ifdef _DEBUG
	pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	return true;
}

void CALLBACK __CreateIBLIntegratorCallback(void *)
{

	ID3D11Device        * pd3dDevice           = DXUTGetD3D11Device();
	ID3D11DeviceContext * pd3dImmediateContext = DXUTGetD3D11DeviceContext();

	CreateILBIntegrator(pd3dDevice, pd3dImmediateContext);

}

HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
									 void* pUserContext)
{

#ifdef IBL_USE_ANTTWEAKBAR

	if (!TwInit(TW_DIRECT3D11, pd3dDevice))
	{
		return -1;
	}

	TwBar * materialBar = TwNewBar("Material");

	TwAddVarRW(materialBar, "baseColor", TW_TYPE_COLOR3F, &g_matBaseColor.x, "colormode=hls");
	TwAddVarRW(materialBar, "F0", TW_TYPE_FLOAT, &g_matF0, "min=0.04 max=1 step=0.01 label='F0'");
	TwAddVarRW(materialBar, "F90", TW_TYPE_FLOAT, &g_matF90, "min=0.04 max=1 step=0.01 label='F90'");
	TwAddVarRW(materialBar, "metallic", TW_TYPE_FLOAT, &g_matMetallic, "min=0 max=1 step=0.01 label='Metallic'");

	TwBar * integratorBar = TwNewBar("Integrator");

	TwAddVarRW(integratorBar, "dfgRes", TW_TYPE_INT32, &g_dfgLUTResolution, "min=64  max=1024 step=32 label='DFG LUT resolution'");
	TwAddVarRW(integratorBar, "ldRes",  TW_TYPE_INT32, &g_ldCubeResolution, "min=128 max=1024 step=32 label='LD convolved resolution'");
	TwAddButton(integratorBar, "create", __CreateIBLIntegratorCallback, nullptr, "label='Create'");

	TwDefine(" TW_HELP visible=false ");


#endif

	D3D11_BUFFER_DESC cbPerObjectDesc = { 0 };

	cbPerObjectDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbPerObjectDesc.Usage          = D3D11_USAGE_DYNAMIC;
	cbPerObjectDesc.ByteWidth      = sizeof(cbPerObject);
	cbPerObjectDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_BUFFER_DESC cbPerFrameDesc = { 0 };

	cbPerFrameDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbPerFrameDesc.Usage          = D3D11_USAGE_DYNAMIC;
	cbPerFrameDesc.ByteWidth      = sizeof(cbPerFrame);
	cbPerFrameDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;


	D3D11_SAMPLER_DESC linearSamplerDesc;

	linearSamplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
	linearSamplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
	linearSamplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
	linearSamplerDesc.BorderColor[0] = 0;
	linearSamplerDesc.BorderColor[1] = 0;
	linearSamplerDesc.BorderColor[2] = 0;
	linearSamplerDesc.BorderColor[3] = 0;
	linearSamplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearSamplerDesc.MaxLOD         = FLT_MAX;
	linearSamplerDesc.MinLOD         = - FLT_MAX;
	linearSamplerDesc.MaxAnisotropy  = 1;
	linearSamplerDesc.MipLODBias     = 0;

	D3D11_DEPTH_STENCIL_DESC depthOnDesc  = { 0 };

	depthOnDesc.DepthEnable    = TRUE;
	depthOnDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthOnDesc.DepthFunc      = D3D11_COMPARISON_LESS;

	D3D11_DEPTH_STENCIL_DESC depthOffDesc = { 0 };

	D3D_SHADER_MACRO meshDef [] = {
		{ "IBL_INSTANCES_COUNT", IBL_INSTANCES_COUNT_STRING },
		{ "IBL_LD_MIPMAPS", IBL_LD_MIPMAPS_STRING },
		{ nullptr, nullptr }
	};

	if (FAILED(pd3dDevice->CreateBuffer(&cbPerObjectDesc, nullptr, &g_cbPerObject)) ||
		FAILED(pd3dDevice->CreateBuffer(&cbPerFrameDesc, nullptr, &g_cbPerFrame)) ||
		FAILED(pd3dDevice->CreateSamplerState(&linearSamplerDesc, &g_linearSampler)) ||
		FAILED(pd3dDevice->CreateDepthStencilState(&depthOffDesc, &g_depthOff)) ||
		FAILED(pd3dDevice->CreateDepthStencilState(&depthOnDesc, &g_depthOn)) ||
		!tdx::CompileShader(g_renderLightProbeVS, pd3dDevice, "./shaders/ibl_render_lightprobe_vs.hlsl", "iblLightProbeVS") ||
		!tdx::CompileShader(g_renderLightProbePS, pd3dDevice, "./shaders/ibl_render_lightprobe_ps.hlsl", "iblLightProbePS") ||
		!tdx::CompileShader(g_renderMeshVS, g_renderMeshIL, pd3dDevice, "./shaders/ibl_render_mesh_vs.hlsl", "iblMeshVS", meshDef) ||
		!tdx::CompileShader(g_renderMeshPS, pd3dDevice, "./shaders/ibl_render_mesh_ps.hlsl", "iblMeshPS", meshDef))
	{
		return -1;
	}

	g_effectFactory = std::make_unique<EffectFactory>(pd3dDevice);

	g_model = Model::CreateFromSDKMESH(pd3dDevice, L"./data/sphere.sdkmesh", *g_effectFactory.get());

	g_camera.SetViewParams(XMVectorSet(0, 0, -10, 1), XMVectorSet(0, 0, 0, 1));
	g_camera.SetScalers(.003f, 3.5f);
	g_camera.SetDrag(true);

	ID3D11DeviceContext * pd3dImmediateContext = DXUTGetD3D11DeviceContext();

	pd3dImmediateContext->CSSetSamplers(0, 1, &g_linearSampler);
	pd3dImmediateContext->PSSetSamplers(0, 1, &g_linearSampler);

	LoadLightProbe(pd3dDevice, pd3dImmediateContext, L"./data/pisa.dds", L"./data/pisa_irradiance.dds");

	CreateILBIntegrator(pd3dDevice, pd3dImmediateContext);

	return S_OK;

}


HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
										 const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{

	float ratio = (float) pBackBufferSurfaceDesc->Width / pBackBufferSurfaceDesc->Height;

	g_camera.SetProjParams(XM_PIDIV2, ratio, .1f, 100.f);
	g_camera.SetRotateButtons(false, false, true, false);

	return S_OK;
}


void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_camera.FrameMove(fElapsedTime);
}


void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
								 double fTime, float fElapsedTime, void* pUserContext)
{

	auto pRTV = DXUTGetD3D11RenderTargetView();
	auto pDSV = DXUTGetD3D11DepthStencilView();

	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::Black);
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	DXUTSetupD3D11Views(pd3dImmediateContext);

	XMVECTOR det;
	XMMATRIX world        = g_camera.GetWorldMatrix();
	XMMATRIX view         = g_camera.GetViewMatrix();
	XMMATRIX viewProj     = view * g_camera.GetProjMatrix();
	XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);

	{

		cbPerFrame cbPerFrameData = { g_camera.GetEyePt(), XMMatrixTranspose(world), XMMatrixTranspose(view), XMMatrixTranspose(viewProj), XMMatrixTranspose(invViewProj) };
		D3D11_MAPPED_SUBRESOURCE mappedBuffer;

		pd3dImmediateContext->Map(g_cbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
		memcpy(mappedBuffer.pData, &cbPerFrameData, sizeof(cbPerFrame));
		pd3dImmediateContext->Unmap(g_cbPerFrame, 0);

	}

	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_cbPerFrame);
	pd3dImmediateContext->PSSetConstantBuffers(0, 1, &g_cbPerFrame);

	/* Draw the light probe */

	pd3dImmediateContext->OMSetDepthStencilState(g_depthOff, 0x0);

	pd3dImmediateContext->VSSetShader(g_renderLightProbeVS, nullptr, 0);
	pd3dImmediateContext->PSSetShader(g_renderLightProbePS, nullptr, 0);

	pd3dImmediateContext->IASetInputLayout(nullptr);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pd3dImmediateContext->Draw(4, 0);

	/* Draw the sphere */

	pd3dImmediateContext->OMSetDepthStencilState(g_depthOn, 0x0);

	pd3dImmediateContext->IASetInputLayout(g_renderMeshIL);

	pd3dImmediateContext->VSSetShader(g_renderMeshVS, nullptr, 0);
	pd3dImmediateContext->PSSetShader(g_renderMeshPS, nullptr, 0);

	auto & part = g_model->meshes[0]->meshParts[0];

	cbPerObject cbPerObjectData = { XMMatrixIdentity(), XMMatrixTranspose(view), XMMatrixTranspose(viewProj), g_matBaseColor, g_matF0, g_matF90, g_matMetallic };
	D3D11_MAPPED_SUBRESOURCE mappedBuffer;

	pd3dImmediateContext->Map(g_cbPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &cbPerObjectData, sizeof(cbPerObject));
	pd3dImmediateContext->Unmap(g_cbPerObject, 0);

	pd3dImmediateContext->VSSetConstantBuffers(1, 1, &g_cbPerObject);
	pd3dImmediateContext->PSSetConstantBuffers(1, 1, &g_cbPerObject);

	pd3dImmediateContext->IASetPrimitiveTopology(part->primitiveType);

	pd3dImmediateContext->IASetIndexBuffer(part->indexBuffer.Get(), part->indexFormat, 0);
	pd3dImmediateContext->IASetVertexBuffers(0, 1, part->vertexBuffer.GetAddressOf(), &part->vertexStride, &part->vertexOffset);

	pd3dImmediateContext->DrawIndexedInstanced(part->indexCount, IBL_INSTANCES_COUNT, part->startIndex, 0, 0);

#ifdef IBL_USE_ANTTWEAKBAR

	TwDraw();

#endif

}


void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{

#ifdef IBL_USE_ANTTWEAKBAR

	TwTerminate();

#endif

	IBL_COM_RELEASE(g_lightProbeSRV);
	IBL_COM_RELEASE(g_lightProbe);

	IBL_COM_RELEASE(g_lightProbeIrradianceSRV);
	IBL_COM_RELEASE(g_lightProbeIrradiance);

	IBL_COM_RELEASE(g_renderLightProbeVS);
	IBL_COM_RELEASE(g_renderLightProbePS);

	IBL_COM_RELEASE(g_renderMeshIL);
	IBL_COM_RELEASE(g_renderMeshVS);
	IBL_COM_RELEASE(g_renderMeshPS);

	IBL_COM_RELEASE(g_linearSampler);

	IBL_COM_RELEASE(g_cbPerFrame);
	IBL_COM_RELEASE(g_cbPerObject);

	IBL_COM_RELEASE(g_depthOn);
	IBL_COM_RELEASE(g_depthOff);

	IBL_COM_RELEASE(g_dfgLUTSRV);
	IBL_COM_RELEASE(g_dfgLUT); 

	IBL_COM_RELEASE(g_ldCube);
	IBL_COM_RELEASE(g_ldCubeSRV);

	g_effectFactory.reset();
	g_integrator.reset();
	g_model.reset();

}


LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
						 bool* pbNoFurtherProcessing, void* pUserContext)
{

#ifdef IBL_USE_ANTTWEAKBAR

	if (TwEventWin(hWnd, uMsg, wParam, lParam))
	{
		return 0;
	}

#endif

	return g_camera.HandleMessages(hWnd, uMsg, wParam, lParam);

}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);

	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	DXUTInit(true, true, nullptr);
	DXUTSetCursorSettings(true, true);
	DXUTCreateWindow(L"IBL GGX");
	
	DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 1280, 720);

	DXUTMainLoop();
	DXUTShutdown();

	return DXUTGetExitCode();
}


