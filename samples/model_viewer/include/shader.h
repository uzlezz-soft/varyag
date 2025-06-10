#pragma once

#include "common.h"
#include <filesystem>
#include <memory>

class Application;
class MeshShader
{
public:
	~MeshShader();

	static std::shared_ptr<MeshShader> From(Application& app, const std::filesystem::path& path);

	vg::Pipeline GetPipeline() const { return _pipeline; }
	vg::Pipeline GetPipelineTwoSided() const { return _pipelineTwoSided; }

private:
	vg::Pipeline _pipeline;
	vg::Pipeline _pipelineTwoSided;

	MeshShader(vg::Pipeline pipeline, vg::Pipeline pipeline2) : _pipeline(pipeline), _pipelineTwoSided(pipeline2) {}
};