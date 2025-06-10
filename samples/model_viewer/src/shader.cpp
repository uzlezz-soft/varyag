#include "shader.h"
#include "mesh.h"
#include "application.h"

#if _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <Windows.h>
#  include <wrl.h>
#  include <unknwn.h>
template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
#else
#include "WinAdapter.h"
template <class T>
using ComPtr = CComPtr<T>;
#endif

#include <dxcapi.h>

#include <fstream>

struct DxcInstance
{
	ComPtr<IDxcLibrary> Library;
	ComPtr<IDxcCompiler> Compiler;
	ComPtr<IDxcIncludeHandler> IncludeHandler;

	DxcInstance()
	{
		DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&Library));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
		Library->CreateIncludeHandler(&IncludeHandler);
	}

	~DxcInstance()
	{
	}
};

static std::vector<uint8_t> CompileShader(std::wstring_view path, std::wstring_view profile, std::wstring_view entryPoint, bool spirv = false)
{
	static DxcInstance dxc;

	std::ifstream shaderFile(path.data(), std::ios::binary);
	if (!shaderFile.is_open()) {
		std::wcerr << L"Path: " << path << L"\n";
		throw std::runtime_error("Cannot open shader on path");
	}
	std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	ComPtr<IDxcBlobEncoding> sourceBlob;
	if (FAILED(dxc.Library->CreateBlobWithEncodingFromPinned((LPBYTE)shaderSource.data(),
		(UINT32)shaderSource.size(), CP_UTF8, &sourceBlob))) {
		std::wcerr << L"Path: " << path << "L\n";
		throw std::runtime_error("Failed to create source blob");
	}

	std::filesystem::path shaderFilePath(path);
	std::wstring includeDir = shaderFilePath.parent_path().wstring();

	// Define compiler arguments
	std::vector<LPCWSTR> arguments = {
		path.data(),
		L"-E", entryPoint.data(),
		L"-T", profile.data(),
		L"-I", includeDir.c_str(),
		L"-Qstrip_reflect",      // Strip reflection data
		L"-Qstrip_debug"         // Strip debug information
	};
	if (spirv)
	{
		arguments.push_back(L"-spirv");
		// otherwise SV_InstanceID will become gl_InstanceIndex with different behavior
		arguments.push_back(L"-fvk-support-nonzero-base-instance");
		arguments.push_back(L"-fvk-bind-resource-heap");
		arguments.push_back(L"0");
		arguments.push_back(L"0");
		arguments.push_back(L"-fvk-bind-counter-heap");
		arguments.push_back(L"1");
		arguments.push_back(L"0");
		arguments.push_back(L"-fvk-bind-sampler-heap");
		arguments.push_back(L"2");
		arguments.push_back(L"0");
	}

	ComPtr<IDxcOperationResult> result;
	if (FAILED(dxc.Compiler->Compile(sourceBlob.Get(), path.data(), entryPoint.data(), profile.data(),
		arguments.data(), (UINT)arguments.size(), nullptr, 0, dxc.IncludeHandler.Get(), &result))) {
		std::wcerr << L"Path: " << path << "\n";
		throw std::runtime_error("Shader compilation failed");
	}

	HRESULT hrStatus;
	if (FAILED(result->GetStatus(&hrStatus)) || FAILED(hrStatus)) {
		ComPtr<IDxcBlobEncoding> errorBlob;
		if (SUCCEEDED(result->GetErrorBuffer(&errorBlob))) {
			std::wcerr << L"Shader compilation errors:\n"
				<< (const char*)errorBlob->GetBufferPointer() << std::endl;
		}
		throw std::runtime_error("<=================>");
	}

	ComPtr<IDxcBlob> shaderBlob;
	if (FAILED(result->GetResult(&shaderBlob))) {
		throw std::runtime_error("Failed to retrieve compiled shader");
	}

	std::vector<uint8_t> shader;
	shader.assign(
		(uint8_t*)shaderBlob->GetBufferPointer(),
		(uint8_t*)shaderBlob->GetBufferPointer() + shaderBlob->GetBufferSize()
	);
	return shader;
}

MeshShader::~MeshShader()
{
	if (!_pipeline) return;
	vg::Device device;
	_pipeline.GetDevice(&device);
	device.DestroyPipeline(_pipeline);
	if (_pipelineTwoSided != _pipeline)
	{
		device.DestroyPipeline(_pipelineTwoSided);
	}
}

std::shared_ptr<MeshShader> MeshShader::From(Application& app, const std::filesystem::path& path)
{
	vg::SwapChainDesc swapChainDesc;
	if (app.GetSwapChain().GetDesc(&swapChainDesc) != vg::Result::Success) return nullptr;
	vg::GraphicsApi api;

	auto device = app.GetDevice();
	device.GetGraphicsApi(&api);
	bool spirv = api == vg::GraphicsApi::Vulkan;

	auto shaderModule = CompileShader(path.generic_wstring(), L"vs_6_6", L"Vertex", spirv);
	vg::ShaderModule vertexShader;
	if (device.CreateShaderModule(shaderModule.data(), shaderModule.size(), &vertexShader) != vg::Result::Success)
	{
		std::cerr << "Unable to create vertex shader " << path.generic_string() << "\n";
		return nullptr;
	}
	shaderModule = CompileShader(path.generic_wstring(), L"ps_6_6", L"Pixel", spirv);
	vg::ShaderModule pixelShader;
	if (device.CreateShaderModule(shaderModule.data(), shaderModule.size(), &pixelShader) != vg::Result::Success)
	{
		std::cerr << "Unable to create pixel shader " << path.generic_string() << "\n";
		device.DestroyShaderModule(vertexShader);
		return nullptr;
	}

	std::array<vg::VertexAttribute, 4> attributes =
	{
		vg::VertexAttribute { vg::Format::R32g32b32Float, 0, 0, vg::AttributeInputRate::Vertex, 0 },
		vg::VertexAttribute { vg::Format::R32g32b32Float, 0, 1, vg::AttributeInputRate::Vertex, 0 },
		vg::VertexAttribute { vg::Format::R32g32b32Float, 0, 2, vg::AttributeInputRate::Vertex, 0 },
		vg::VertexAttribute { vg::Format::R32g32Float, 0, 3, vg::AttributeInputRate::Vertex, 0 },
	};
	vg::GraphicsPipelineDesc pipelineDesc = {
		vg::VertexPipeline::FixedFunction, vg::FixedFunctionState{
			attributes.size(), attributes.data(), vertexShader, nullptr, nullptr, nullptr
		}, vg::MeshShaderState{}, pixelShader, vg::PrimitiveTopology::TriangleList, false, 0u,
		vg::RasterizationState{vg::FillMode::Fill, vg::CullMode::Back, vg::FrontFace::Clockwise},
		vg::MultisamplingState{vg::SampleCount::e1}, vg::DepthStencilState{true, true, vg::CompareOp::Less},
		1u, {}, app.GetDepthBufferFormat(), vg::BlendState{false, vg::LogicOp::NoOp, {} }
	};
	pipelineDesc.colorAttachmentFormats[0] = swapChainDesc.format;
	pipelineDesc.blendState.attachments[0] = { false, {}, {}, {}, {}, {}, {},
		vg::ColorComponentFlags::R | vg::ColorComponentFlags::G | vg::ColorComponentFlags::B | vg::ColorComponentFlags::A };
	float blendConstants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	memcpy(pipelineDesc.blendState.blendConstants, blendConstants, sizeof(float) * 4);

	vg::Pipeline pipeline;
	if (device.CreateGraphicsPipeline(&pipelineDesc, &pipeline) != vg::Result::Success)
	{
		std::cerr << "Unable to create graphics pipeline\n";
		device.DestroyShaderModule(vertexShader);
		device.DestroyShaderModule(pixelShader);
		return nullptr;
	}

	pipelineDesc.rasterizationState.cullMode = vg::CullMode::None;
	vg::Pipeline pipelineTwoSided;
	if (device.CreateGraphicsPipeline(&pipelineDesc, &pipelineTwoSided) != vg::Result::Success) pipelineTwoSided = pipeline;

	pipeline.SetName(path.stem().generic_string().c_str());
	device.DestroyShaderModule(vertexShader);
	device.DestroyShaderModule(pixelShader);
	return std::shared_ptr<MeshShader>(new MeshShader(pipeline, pipelineTwoSided));
}
