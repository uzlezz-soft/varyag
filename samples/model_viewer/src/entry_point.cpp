#include "application.h"
#include <mimalloc.h>
#include <FreeImage.h>

#if _WIN32
extern "C" { __declspec(dllexport) extern const uint32_t D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

int main(int argc, char** argv)
{
	glfwInit();

	vg::Config cfg = {
		"Model Viewer", "Varyag",
		vg::InitFlags::Debug | vg::InitFlags::EnableMessageCallback | vg::InitFlags::UseProvidedAllocator,
		[](VgMessageSeverity severity, const char* msg)
		{
			std::cout << "VARYAG: (" << severity << ") " << msg << "\n";
		},
		{
			nullptr,
			[](void* userData, size_t size, size_t alignment)
			{
				return mi_malloc_aligned(size, alignment);
			},
			[](void* userData, void* original, size_t size, size_t alignment)
			{
				return mi_realloc_aligned(original, size, alignment);
			},
			[](void* userData, void* memory)
			{
				mi_free(memory);
			}
		}
	};
	vg::Library library(cfg);
	FreeImage_Initialise();

	Application application(argc, argv);
	
	FreeImage_DeInitialise();
	glfwTerminate();
	return 0;
}