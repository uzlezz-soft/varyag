#include "d3d12shader_module.h"
#include <cstring>

D3D12ShaderModule::D3D12ShaderModule(D3D12Device& device, const void* data, uint64_t size)
{
	_data.resize(size);
	memcpy(_data.data(), data, size);
	_bytecode = {
		.pShaderBytecode = _data.data(),
		.BytecodeLength = size
	};
}

D3D12ShaderModule::~D3D12ShaderModule()
{
}
