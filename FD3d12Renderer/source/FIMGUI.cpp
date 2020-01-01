#include "FIMGUI.h"

#include "FD3d12Renderer.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <d3d12.h>
#include "FJobSystem.h"

#include "FUtilities.h"

static int const                    NUM_FRAMES_IN_FLIGHT = 2;

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

std::vector<FDelegate2<void(void)>> FIMGUI::myRenderCallbacks;

FIMGUI::FIMGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	ImGui_ImplWin32_Init(FD3d12Renderer::GetInstance()->GetWindowHandle());
	ImGui_ImplDX12_Init(FD3d12Renderer::GetInstance()->GetDevice(), NUM_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		FD3d12Renderer::GetInstance()->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(),
		FD3d12Renderer::GetInstance()->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	ImGui::StyleColorsDark();
}
#pragma optimize("", off)
void FIMGUI::Update()
{
//	FLOG("active window %i", ::GetActiveWindow());
//	FLOG("my window %i", FD3d12Renderer::GetInstance()->GetWindowHandle());
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	     
	if (show_demo_window)
           ImGui::ShowDemoWindow(&show_demo_window);
	//{
	//	static float f = 0.0f;
	//	static int counter = 0;
	//	{
	//		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	//		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
	//		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
	//		ImGui::Checkbox("Another Window", &show_another_window);

	//		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
	//		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

	//		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
	//			counter++;
	//		ImGui::SameLine();
	//		ImGui::Text("counter = %d", counter);

	//		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	//		ImGui::End();
	//	}
	//}

	for (size_t i = 0; i < myRenderCallbacks.size(); i++)
	{
		myRenderCallbacks[i]();
	}

	ID3D12GraphicsCommandList* cmdList = FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);
	cmdList->Reset(FD3d12Renderer::GetInstance()->GetCommandAllocatorForWorkerThread(FJobSystem::ourThreadIdx), nullptr);

	ID3D12DescriptorHeap* descHeap = FD3d12Renderer::GetInstance()->GetSRVHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = FD3d12Renderer::GetInstance()->GetRTVHandle();
	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
	cmdList->SetDescriptorHeaps(1, &descHeap);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx));
	ID3D12CommandList* const ppCommandLists = { FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx) };
	FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx)->Close();
	FD3d12Renderer::GetInstance()->GetCommandQueue()->ExecuteCommandLists(1, &ppCommandLists);
}

FIMGUI::~FIMGUI()
{
}
