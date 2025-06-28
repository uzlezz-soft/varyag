#pragma once

#include "d3d12device.h"

#if VG_D3D12_SUPPORTED

class D3D12Pipeline : public VgPipeline_t
{
public:
	virtual ~D3D12Pipeline() = default;

	void* GetApiObject() const override { return _state.Get(); }
	void SetName(const char* name) override { _state->SetName(ConvertToWideString(name).data()); }
	D3D12Device* Device() const override { return _device; }
	ComPtr<ID3D12PipelineState> GetPipelineState() const { return _state; }
protected:
	D3D12Device* _device;
	ComPtr<ID3D12PipelineState> _state;
};

class D3D12ComputePipeline : public D3D12Pipeline
{
public:
	D3D12ComputePipeline(D3D12Device& device, VgShaderModule computeModule);
	~D3D12ComputePipeline();
};

class D3D12GraphicsPipeline : public D3D12Pipeline
{
public:
	D3D12GraphicsPipeline(D3D12Device& device, const VgGraphicsPipelineDesc& desc);
	~D3D12GraphicsPipeline();

	bool PrimitiveRestart() const { return _primitiveRestart; }
	bool DepthBoundsTest() const { return _depthBoundsTest; }
	float DepthBoundsMin() const { return _depthBoundsMin; }
	float DepthBoundsMax() const { return _depthBoundsMax; }
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology() const { return _primitiveTopology; }

private:
	uint8_t _primitiveRestart : 1;
	uint8_t _depthBoundsTest : 1;
	float _depthBoundsMin;
	float _depthBoundsMax;
	D3D12_PRIMITIVE_TOPOLOGY _primitiveTopology;
};

#endif