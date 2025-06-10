#pragma once

#include "d3d12device.h"
#include <vector>

class D3D12ShaderModule final : public VgShaderModule_t
{
public:
	D3D12ShaderModule(D3D12Device& device, const void* data, uint64_t size);
	~D3D12ShaderModule();

	D3D12_SHADER_BYTECODE& GetBytecode() { return _bytecode; }
	const D3D12_SHADER_BYTECODE& GetBytecode() const { return _bytecode; }
private:
	vg::Vector<char> _data;
	D3D12_SHADER_BYTECODE _bytecode;
};