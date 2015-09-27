#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <string>
#include <sstream>
#include <vector>

namespace tdx
{

	namespace detail
	{

		template <typename ShaderType>
		struct ShaderHelper { };

		template <typename ShaderType>
		bool CompileShader(ShaderType * & shader, ID3DBlob * & code, ID3DBlob * & errors, ID3D11Device * device, const char * filename, const char * entryPoint, const D3D_SHADER_MACRO * defines);

	}

	ID3D11InputLayout * CreateVSInputLayout(ID3D11Device * device, ID3D11VertexShader * shader, ID3DBlob * code);

	template <typename ShaderType>
	inline bool CompileShader(ShaderType * & shader, ID3D11Device * device, const char * filename, const char * entryPoint, const D3D_SHADER_MACRO * defines = nullptr)
	{

		using namespace detail;

		bool success = true;
		ID3DBlob * code, * errors;

		if (!CompileShader(shader, code, errors, device, filename, entryPoint, defines))
		{

			if (errors)
			{

				std::ostringstream ss;
				ss << "tdx::CompileShader ERROR: " << static_cast<const char*>(errors->GetBufferPointer()) << std::endl;

				OutputDebugStringA(ss.str().c_str());

				errors->Release();

			}

			success = false;

		}

		if (code)
		{
			code->Release();
		}

		return success;

	}

	inline bool CompileShader(ID3D11VertexShader * & shader, ID3D11InputLayout * & inputLayout, ID3D11Device * device, const char * filename, const char * entryPoint, const D3D_SHADER_MACRO * defines = nullptr)
	{

		bool success = true;
		ID3DBlob * code, * errors;

		if (!detail::CompileShader(shader, code, errors, device, filename, entryPoint, defines))
		{

			if (errors)
			{

				std::ostringstream ss;
				ss << "tdx::CompileShader ERROR: " << static_cast<const char*>(errors->GetBufferPointer()) << std::endl;

				OutputDebugStringA(ss.str().c_str());

				errors->Release();

			}

			success = false;

		}


		if (code)
		{
			inputLayout = CreateVSInputLayout(device, shader, code);
			code->Release();
		}

		return success;

	}

	namespace detail
	{

		template <typename ShaderType>
		bool CompileShader(ShaderType * & shader, ID3DBlob * & code, ID3DBlob * & errors, ID3D11Device * device, const char * filename, const char * entryPoint, const D3D_SHADER_MACRO * defines)
		{

			code   = nullptr;
			errors = nullptr;

			std::ifstream f(filename);

			if (!f)
			{

				std::ostringstream ss;
				ss << "tdx::CompileShader ERROR: Cannot open file \"" << filename << "\"." << std::endl;

				OutputDebugStringA(ss.str().c_str());

				return false;

			}

			std::string sourceCode = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

			UINT compileOptions = 0;

#ifdef _DEBUG

			compileOptions |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;

#endif

			return !FAILED(D3DCompile(sourceCode.c_str(), sourceCode.size(), filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, ShaderHelper<ShaderType>::target, compileOptions, 0, &code, &errors)) &&
			       !FAILED(ShaderHelper<ShaderType>::CreateShader(device, code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader));

		}

		template <>
		struct ShaderHelper<ID3D11VertexShader>
		{

			static const char * target;

			template <typename ... Args>
			static HRESULT CreateShader(ID3D11Device * device, Args && ... args)
			{
				return device->CreateVertexShader(std::forward<Args>(args) ...);
			}

		};

		template <>
		struct ShaderHelper<ID3D11PixelShader>
		{

			static const char * target;

			template <typename ... Args>
			static HRESULT CreateShader(ID3D11Device * device, Args && ... args)
			{
				return device->CreatePixelShader(std::forward<Args>(args) ...);
			}

		};

		template <>
		struct ShaderHelper<ID3D11GeometryShader>
		{

			static const char * target;

			template <typename ... Args>
			static HRESULT CreateShader(ID3D11Device * device, Args && ... args)
			{
				return device->CreateGeometryShader(std::forward<Args>(args) ...);
			}

		};

		template <>
		struct ShaderHelper<ID3D11ComputeShader>
		{

			static const char * target;

			template <typename ... Args>
			static HRESULT CreateShader(ID3D11Device * device, Args && ... args)
			{
				return device->CreateComputeShader(std::forward<Args>(args) ...);
			}

		};

	}


}