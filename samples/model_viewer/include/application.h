#pragma once

#include "common.h"
#include "texture.h"
#include "shader.h"
#include "model.h"
#include "camera.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>

struct CameraData
{
	glm::mat4 viewProjection;
	glm::vec4 forward;
	glm::vec2 jitter;
};

class Application
{
public:
	Application(int argc, char** argv);
	~Application();

	vg::Device GetDevice() const { return _device.Get(); }
	vg::SwapChain GetSwapChain() const { return _swapChain.Get(); }
	
	vg::Format GetDepthBufferFormat() const { return _depthBufferFormat; }

	void SubmitImmediately(std::function<void(vg::CommandList)>&& action);

private:
	GLFWwindow* _window;
	vg::Ref<vg::Device> _device;
	vg::Surface _surface;
	vg::Ref<vg::SwapChain> _swapChain;

	vg::Ref<vg::CommandPool> _immediateCommandPool;

	vg::Format _depthBufferFormat;
	vg::Ref<vg::Texture> _depthBuffer;
	vg::AttachmentView _depthBufferAttachment;

	Camera _camera;
	Frustum _frustum;
	std::unique_ptr<PerFrameConstantBuffer<CameraData>> _cameraData;

	std::vector<FrameData> _frames;
	std::unordered_map<std::string, std::shared_ptr<Texture>> _textures;

	std::shared_ptr<MeshShader> _pbr;
	std::shared_ptr<Model> _model;
	std::shared_ptr<Model> _model2;

	void Run();
	void DoFrame(uint64_t frameIndex, const FrameData& frame);

	void OnScroll(double yOffset);
	void OnMouseMove(double xPos, double yPos);
	void OnMousePress(int button, int action, int mods);
	void ProcessInput(float deltaTime);
};