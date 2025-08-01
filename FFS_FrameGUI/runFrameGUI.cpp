
#include"framegui.h"
#include <d3d11.h>
#include <tchar.h>
#include "ImGui/imgui_impl_win32.h"
#include <algorithm>
#include <vector>
#include <string>



// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;





static bool g_DataUpdated = true;  
static float g_X = 0.0f;
static float g_Y = 0.0f;
static float g_Z = 0.0f;
static int g_FrameRange = 100;
static int g_FrameCounter = 0;

static std::vector<std::string> g_ErrorFrames;
static std::vector<float> g_AltHistory;
static std::vector<float> g_LonHistory; 
static std::vector<float> g_LatHistory;  
     
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



void UpdateData(float x, float y, float z) {
	g_X = x;
	g_Y = y;
	g_Z = z;
	g_DataUpdated = true;  // 标记数据已更新
}





// 
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam);
		g_ResizeHeight = (UINT)HIWORD(lParam);
		g_DataUpdated = true;
		return 0;
	case WM_MOVE:
	case WM_PAINT:
		g_DataUpdated = true;
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


int draw_gui() {
	// 创建窗口
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"FrameImGui", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"FrameGui With DirectX11", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	// 初始化D3D
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// 显示窗口
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// 初始化ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// 主循环变量
	bool done = false;
	bool show_statistical_window = true;
	bool show_draw_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 主循环
	while (!done) {
		// 处理消息
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}

		// 按需渲染逻辑
		if (g_DataUpdated) {
			g_DataUpdated = false;

			// 处理窗口最小化/遮挡
			if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
				::Sleep(10);
				continue;
			}

			// 处理窗口大小调整
			if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
				CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
				g_ResizeWidth = g_ResizeHeight = 0;
				CreateRenderTarget();
			}

			// 更新数据（示例用随机值，实际应替换为真实数据源）
			g_X = 0.0f + (rand() % 1800) / 10.0f;
			g_Y = -90.0f + (rand() % 1800) / 10.0f;
			g_Z = 500.0f + (rand() % 50) / 10.0f;

			// 记录历史数据
			g_LonHistory.push_back(g_X);
			g_LatHistory.push_back(g_Y);
			g_AltHistory.push_back(g_Z);
			g_FrameCounter++;

			// 移除旧数据
			while ((int)g_AltHistory.size() > g_FrameRange) {
				g_AltHistory.erase(g_AltHistory.begin());
				g_LonHistory.erase(g_LonHistory.begin());
				g_LatHistory.erase(g_LatHistory.begin());
			}

			// 开始新帧
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			// 构建界面
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			float half_width = viewport->Size.x * 0.5f;
			float height = viewport->Size.y;

			// 左侧统计窗口
			ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(half_width, height), ImGuiCond_Always);
			if (ImGui::Begin("Frame Statistics", &show_statistical_window, ImGuiWindowFlags_MenuBar)) {
				ImGui::BeginChild("StatsPanel", ImVec2(0, 0), true);
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Real-time location");
				ImGui::Separator();

				if (ImGui::BeginTable("PositionInfo", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Longitude");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.6f", g_X);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Latitude");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.6f", g_Y);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Altitude");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2f m", g_Z);

					ImGui::EndTable();
				}

				ImGui::Spacing();
				ImGui::Text("rate: %.1f FPS", ImGui::GetIO().Framerate);
				ImGui::EndChild();
			}
			ImGui::End();

			// 右侧高度图
			ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + half_width, viewport->Pos.y), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(half_width, height * 0.5f), ImGuiCond_Always);
			if (ImGui::Begin("Altitude Plot", &show_draw_window, ImGuiWindowFlags_MenuBar)) {
				if (!g_AltHistory.empty()) {
					struct Funcs {
						static float RescaleValue(void* data, int idx) {
							return (*static_cast<std::vector<float>*>(data))[idx];
						}
					};

					ImGui::PlotLines(
						"Altitude (m)",
						&Funcs::RescaleValue, &g_AltHistory,
						(int)g_AltHistory.size(),
						0,
						nullptr,
						FLT_MAX, FLT_MAX,
						ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.7f)
					);

					ImGui::Text("Current Altitude: %.2f m", g_Z);
					ImGui::Text("Frame Count: %d", g_FrameCounter);
					ImGui::SliderInt("Frame Range", &g_FrameRange, 10, 200, "%d frames");
				}
				else {
					ImGui::Text("Waiting for data...");
				}
			}
			ImGui::End();

			// 渲染
			ImGui::Render();
			const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
			g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
			g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			// 提交帧
			g_pSwapChain->Present(1, 0);
		}
		else {
			// 无数据更新时释放CPU
			::WaitMessage();
		}
	}

	// 清理资源
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}




bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}