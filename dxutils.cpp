
#include "dxutils.hpp"

#include <vector>

namespace tdx
{

	const char * detail::ShaderHelper<ID3D11VertexShader>::target   = "vs_5_0";
	const char * detail::ShaderHelper<ID3D11PixelShader>::target    = "ps_5_0";
	const char * detail::ShaderHelper<ID3D11GeometryShader>::target = "gs_5_0";
	const char * detail::ShaderHelper<ID3D11ComputeShader>::target  = "cs_5_0";

	static inline DXGI_FORMAT GetDXGIFormat(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE mask)
	{

		DXGI_FORMAT format;

		if (mask == 1)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (mask <= 3)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (mask <= 7)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32B32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32B32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (mask <= 15)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		return format;

	}

	ID3D11InputLayout * CreateVSInputLayout(ID3D11Device * device, ID3D11VertexShader * shader, ID3DBlob * code)
	{

		ID3D11InputLayout      * inputLayout = nullptr;
		ID3D11ShaderReflection * reflection;

		if (!FAILED(D3DReflect(code->GetBufferPointer(), code->GetBufferSize(), IID_ID3D11ShaderReflection, (void **) &reflection)))
		{

			D3D11_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);

			std::vector<D3D11_INPUT_ELEMENT_DESC> elements;

			for (int i = 0; i < shaderDesc.InputParameters; i++)
			{

				D3D11_SIGNATURE_PARAMETER_DESC inputParamDesc;
				reflection->GetInputParameterDesc(i, &inputParamDesc);

				D3D11_INPUT_ELEMENT_DESC elementDesc;

				elementDesc.SemanticName         = inputParamDesc.SemanticName;
				elementDesc.SemanticIndex        = inputParamDesc.SemanticIndex;
				elementDesc.InputSlot            = 0;
				elementDesc.AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
				elementDesc.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
				elementDesc.InstanceDataStepRate = 0;

				elementDesc.Format = GetDXGIFormat(inputParamDesc.ComponentType, inputParamDesc.Mask);

				elements.push_back(elementDesc);

			}
			
			HRESULT hr = device->CreateInputLayout(&elements[0], elements.size(), code->GetBufferPointer(), code->GetBufferSize(), &inputLayout);
			reflection->Release();

		}

		return inputLayout;

	}

}