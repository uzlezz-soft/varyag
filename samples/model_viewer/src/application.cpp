#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include "application.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "texture.h"
#include "shader.h"

#include <glm/gtx/projection.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Application::Application(int argc, char** argv)
{
	uint32_t numAdapters;
	vgCheck(vg::EnumerateAdapters(vg::GraphicsApi::Auto, &numAdapters, nullptr));
	std::vector<vg::Adapter> adapters(numAdapters);
	vgCheck(vg::EnumerateAdapters(vg::GraphicsApi::Auto, &numAdapters, adapters.data()));

	vgCheck(adapters.front().CreateDevice(&_device));

	vg::GraphicsApi graphicsApi;
	vgCheck(_device->GetGraphicsApi(&graphicsApi));

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_window = glfwCreateWindow(1920, 1080, "Varyag Model Viewer", nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);

	glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset)
		{ static_cast<Application*>(glfwGetWindowUserPointer(window))->OnScroll(yoffset); });
	glfwSetCursorPosCallback(_window, [](GLFWwindow* window, double xPos, double yPos)
		{ static_cast<Application*>(glfwGetWindowUserPointer(window))->OnMouseMove(xPos, yPos); });
	glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods)
		{ static_cast<Application*>(glfwGetWindowUserPointer(window))->OnMousePress(button, action, mods); });

	if (graphicsApi == vg::GraphicsApi::D3d12)
	{
		vgCheck(vg::CreateSurfaceD3D12(glfwGetWin32Window(_window), &_surface));
	}
	else
	{
		assert(false);
	}

	vg::SwapChainDesc swapChainDesc = { 1920, 1080, vg::Format::B8g8r8a8Unorm, 2, _surface };
	vgCheck(_device->CreateSwapChain(&swapChainDesc, &_swapChain));

	_frames.resize(swapChainDesc.bufferCount);
	for (uint32_t i = 0; i < swapChainDesc.bufferCount; i++)
	{
		vgCheck(_device->CreateFence(0, &_frames[i].renderingFence));
		vgCheck(_device->CreateCommandPool(vg::CommandPoolFlags::FlagTransient, vg::Queue::Graphics, &_frames[i].graphicsCommandPool));
		vgCheck(_frames[i].graphicsCommandPool->AllocateCommandList(&_frames[i].cmd));

		vg::AttachmentViewDesc desc = {
			swapChainDesc.format, vg::TextureAttachmentViewType::e2d, 0, 0, 1
		};
		vg::Texture backBuffer;
		vgCheck(_swapChain->GetBackBuffer(i, &backBuffer));
		vgCheck(backBuffer.CreateAttachmentView(&desc, &_frames[i].swapChainAttachmentView));
	}
	
	vgCheck(_device->CreateCommandPool(vg::CommandPoolFlags::FlagTransient, vg::Queue::Graphics, &_immediateCommandPool));

	_camera = { {-3, 1, 2}, 1.0f };
	_cameraData = std::make_unique<PerFrameConstantBuffer<CameraData>>(*_device, "CameraData");

	_depthBufferFormat = vg::Format::D32Float;

	vg::TextureDesc depthBufferDesc = { vg::TextureType::e2d, _depthBufferFormat, swapChainDesc.width, swapChainDesc.height,
		1, 1, vg::SampleCount::e1, vg::TextureUsageFlags::DepthStencilAttachment, vg::TextureTiling::Optimal,
		vg::TextureLayout::DepthStencil, vg::HeapType::Gpu };
	vgCheck(_device->CreateTexture(&depthBufferDesc, &_depthBuffer));

	vg::AttachmentViewDesc depthBufferAttachmentDesc = { depthBufferDesc.format, vg::TextureAttachmentViewType::e2d, 0, 0, 1 };
	vgCheck(_depthBuffer->CreateAttachmentView(&depthBufferAttachmentDesc, &_depthBufferAttachment));

	_pbr = MeshShader::From(*this, "shaders/PBR.hlsl");

	_model = Model::From(*this, "models/Bistro_v5_2/BistroExterior.fbx").value();
	_model2 = Model::From(*this, "models/Bistro_v5_2/BistroInterior.fbx").value();
	
	vg::MemoryStatistics stats;
	_device->GetMemoryStatistics(&stats);
	std::cout << "buffers: " << stats.numBuffers << ", textures: " << stats.numTextures << ", vram: " << stats.usedVram << "\n";

	Run();
}

Application::~Application()
{
	_device->WaitIdle();
	for (auto& frame : _frames)
	{
		_device->DestroyFence(frame.renderingFence);
	}

	_cameraData->destroy(*_device);
	_swapChain = nullptr;
	vg::GraphicsApi graphicsApi;
	vgCheck(_device->GetGraphicsApi(&graphicsApi));
	if (graphicsApi == vg::GraphicsApi::Vulkan)
	{
		vg::DestroySurfaceVulkan(_surface);
	}
	glfwDestroyWindow(_window);
}

void Application::SubmitImmediately(std::function<void(vg::CommandList)>&& action)
{
	vg::CommandList cmd;
	_immediateCommandPool->AllocateCommandList(&cmd);

	cmd.Begin();
	action(cmd);
	cmd.End();

	vg::SubmitInfo submitInfo = { 0, nullptr, 0, nullptr, 1, &cmd };
	_device->SubmitCommandLists(1, &submitInfo);

	vg::Queue queue;
	_immediateCommandPool->GetQueue(&queue);
	_device->WaitQueueIdle(queue);

	_immediateCommandPool->FreeCommandList(cmd);
}

void Application::Run()
{
	uint64_t frameIndex = 0;
	float lastFrameTime = glfwGetTime();

	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		float time = glfwGetTime();
		float deltaTime = time - lastFrameTime;
		lastFrameTime = time;
		ProcessInput(deltaTime);

		uint32_t backBufferImageIndex;
		vgCheck(_swapChain->AcquireNextImage(&backBufferImageIndex));
		vg::Texture backBuffer;
		vgCheck(_swapChain->GetBackBuffer(backBufferImageIndex, &backBuffer));

		auto& frameData = _frames[backBufferImageIndex];
		frameData.backBuffer = backBuffer;
		_device->WaitFence(frameData.renderingFence, frameData.fenceValue);

		frameData.graphicsCommandPool->Reset();
		auto cmd = frameData.cmd;
		cmd->Begin();

		DoFrame(frameIndex, frameData);

		cmd->End();

		frameData.fenceValue = frameIndex + 1;
		vg::FenceOperation renderingFenceSignal = { frameData.renderingFence, frameData.fenceValue };
		vg::SubmitInfo submit = { 0, nullptr, 1, &renderingFenceSignal, 1, &cmd };
		_device->SubmitCommandLists(1, &submit);

		vgCheck(_swapChain->Present(0, nullptr));
		frameIndex++;
	}
}

void Application::DoFrame(uint64_t frameIndex, const FrameData& frame)
{
	auto viewMatrix = _camera.GetViewMatrix();
	auto projectionMatrix = glm::perspectiveLH(glm::radians(90.0f), 1.6f / 0.9f, 0.01f, 100.0f);

	auto cameraMatrix = projectionMatrix * viewMatrix;
	cameraMatrix *= glm::mat4(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	auto jitter = glm::vec2(0.0f);

	_cameraData->data = { cameraMatrix, glm::vec4(_camera.GetForwardVector(), 0.0), jitter };
	_cameraData->upload(frameIndex);

	if (glfwGetKey(_window, GLFW_KEY_F1) != GLFW_PRESS)
	{
		_frustum = { cameraMatrix };
	}

	vg::TextureBarrier presentToColorAttachment = { vg::PipelineStageFlags::TopOfPipe,
			vg::AccessFlags::None,
			vg::PipelineStageFlags::ColorAttachmentOutput,
			vg::AccessFlags::ColorAttachmentWrite,
			vg::TextureLayout::Present,
			vg::TextureLayout::ColorAttachment,
			frame.backBuffer, { 0, 1, 0, 1 }
	};
	vg::DependencyInfo dependency = { 0, nullptr, 0, nullptr, 1, &presentToColorAttachment };
	frame.cmd->Barrier(&dependency);

	vg::AttachmentInfo swapChainAttachment = {
		frame.swapChainAttachmentView,
		vg::TextureLayout::ColorAttachment,
		VG_NO_VIEW, {}, {},
		vg::AttachmentOp::Clear,
		vg::AttachmentOp::Default,
		vg::ClearValue {vg::ClearColor(0.1f, 0.1f, 0.1f, 1.0f)}
	};
	vg::RenderingInfo renderingInfo = { 1, &swapChainAttachment, 
		{ _depthBufferAttachment, vg::TextureLayout::DepthStencil, VG_NO_VIEW, {},
		{}, vg::AttachmentOp::Clear, vg::AttachmentOp::Default, vg::ClearValue {vg::ClearDepth(1.0f)} } };
	frame.cmd->BeginRendering(&renderingInfo);

	vg::SwapChainDesc swapChainDesc;
	_swapChain->GetDesc(&swapChainDesc);

	vg::Viewport viewport{ 0, 0, static_cast<float>(swapChainDesc.width), static_cast<float>(swapChainDesc.height), 0, 1 };
	frame.cmd->SetViewport(0, 1, &viewport);
	vg::Scissor scissor{ 0, 0, swapChainDesc.width, swapChainDesc.height };
	frame.cmd->SetScissor(0, 1, &scissor);

	//frame.cmd->SetPipeline(_pbr->GetPipeline());

	auto worldMatrix = glm::mat4{ 1.0f };
	worldMatrix = glm::scale(worldMatrix, glm::vec3{ 0.01f });

	vg::Pipeline boundPipeline = nullptr;

	MeshBindData bindData;
	bindData.cameraData = _cameraData->getView(frameIndex);

	const auto drawModel = [&](std::shared_ptr<Model> model, const glm::mat4& worldMatrix)
	{
		auto normalMatrix = glm::mat3(transpose(inverse(worldMatrix)));
		if (model)
		{
			const auto& meshes = model->GetMeshes();
			for (auto& mesh : meshes)
			{
				if (!_frustum.IsAABBInside(mesh->GetAABB().TransformToWorld(worldMatrix))) continue;

				auto pipeline = mesh->GetMaterial().twoSided ? _pbr->GetPipelineTwoSided() : _pbr->GetPipeline();
				if (pipeline != boundPipeline)
				{
					frame.cmd->SetPipeline(pipeline);
					boundPipeline = pipeline;
				}

				memcpy(bindData.worldMatrix, glm::value_ptr(worldMatrix), sizeof(bindData.worldMatrix));
				memcpy(bindData.normalMatrix0, &normalMatrix[0], sizeof(glm::vec3));
				memcpy(bindData.normalMatrix1, &normalMatrix[1], sizeof(glm::vec3));
				memcpy(bindData.normalMatrix2, &normalMatrix[2], sizeof(glm::vec3));
				bindData.material = mesh->GetMaterial().baseColorTexture ? mesh->GetMaterial().baseColorTexture->GetSrv() : -1;
				frame.cmd->SetRootConstants(vg::PipelineType::Graphics, 0, sizeof(bindData) / sizeof(uint32_t), &bindData);

				std::array vertexBuffers = { mesh->GetPositionsVB(), mesh->GetNormalsVB(), mesh->GetTangentsVB(), mesh->GetUVsVB() };
				frame.cmd->SetVertexBuffers(0, static_cast<uint32_t>(vertexBuffers.size()), vertexBuffers.data());
				frame.cmd->SetIndexBuffer(vg::IndexType::Uint32, mesh->GetIndexOffset(), mesh->GetVertexBuffer());
				frame.cmd->DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
			}
		}
	};

	drawModel(_model, worldMatrix);
	drawModel(_model2, worldMatrix);

	frame.cmd->EndRendering();

	vg::TextureBarrier colorAttachmentToPresent = { vg::PipelineStageFlags::ColorAttachmentOutput,
			vg::AccessFlags::ColorAttachmentWrite,
			vg::PipelineStageFlags::BottomOfPipe,
			vg::AccessFlags::None,
			vg::TextureLayout::ColorAttachment,
			vg::TextureLayout::Present,
			frame.backBuffer, { 0, 1, 0, 1 }
	};
	dependency.textureBarriers = &colorAttachmentToPresent;
	frame.cmd->Barrier(&dependency);
}





void Application::OnScroll(double yOffset)
{
	_camera.Zoom(-yOffset);
}

void Application::OnMouseMove(double xPos, double yPos)
{
	_camera.ProcessMouseMovement(xPos, yPos);
}

void Application::OnMousePress(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS) _camera.rotating = true;
		else if (action == GLFW_RELEASE)
		{
			_camera.rotating = false;
			if (!_camera.panning) _camera.firstMouse = true;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS) _camera.panning = true;
		else if (action == GLFW_RELEASE)
		{
			_camera.panning = false;
			if (!_camera.rotating) _camera.firstMouse = true;
		}
	}
}

void Application::ProcessInput(float deltaTime)
{
	_camera.Update(deltaTime);
}