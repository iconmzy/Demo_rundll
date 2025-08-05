#define FRAMEGUI_EXPORTS 
#include"framegui.h"
#include <d3d11.h>
#include <tchar.h>
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui.h"
#include "ImGui/implot.h"
#include <algorithm>
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "ImGui/ImGuizmo.h"



// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;





//static bool g_DataUpdated = true;  
static float g_X = 0.0f;
static float g_Y = 0.0f;
static float g_Z = 500.0f;
static int g_FrameRange = 100;
static int g_FrameCounter = 0;

static std::vector<std::string> g_ErrorFrames;
static std::vector<float> g_AltHistory;
static std::vector<float> g_LonHistory; 
static std::vector<float> g_LatHistory;  


static glm::mat4 earthMatrix = glm::mat4(1.0f); 
static glm::vec3 currentPos;                    
static std::vector<glm::vec3> historyPositions; 


     
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


std::mutex g_DataMutex;

//make sure auomic operate
std::atomic<bool> g_GuiRunning(false);

void UpdateData(float x, float y, float z) {
	std::lock_guard<std::mutex> lock(g_DataMutex);
	g_X = x;
	g_Y = y;
	g_Z = z;
	//g_DataUpdated = true;
}


glm::vec3 LatLonToSphere(float lat, float lon, float radius = 1.0f) {
    lat = glm::radians(lat);
    lon = glm::radians(lon);
    return glm::vec3(
        radius * cos(lat) * cos(lon),
        radius * sin(lat),
        radius * cos(lat) * sin(lon)
    );
}

void DrawEarth(ImDrawList* drawList, const glm::mat4& viewProj) {
	const float radius = 1.0f;
	const ImU32 earthColor = IM_COL32(100, 100, 255, 255);
	const ImU32 gridColor = IM_COL32(80, 80, 200, 255);


	const int segments = 24;
	for (int i = 0; i <= segments; ++i) {
		float lat = glm::pi<float>() * (i / (float)segments - 0.5f);
		for (int j = 0; j < segments; ++j) {
			float lon1 = 2 * glm::pi<float>() * j / segments;
			float lon2 = 2 * glm::pi<float>() * (j + 1) / segments;

			glm::vec3 p1 = LatLonToSphere(glm::degrees(lat), glm::degrees(lon1));
			glm::vec3 p2 = LatLonToSphere(glm::degrees(lat), glm::degrees(lon2));

			glm::vec4 sp1 = viewProj * glm::vec4(p1, 1.0f);
			glm::vec4 sp2 = viewProj * glm::vec4(p2, 1.0f);

			if (sp1.z > 0 && sp2.z > 0) {
				drawList->AddLine(
					ImVec2(sp1.x / sp1.w, sp1.y / sp1.w),
					ImVec2(sp2.x / sp2.w, sp2.y / sp2.w),
					(i % 6 == 0) ? gridColor : earthColor
				);
			}
		}
	}
}




ImVec2 ProjectOnPseudoSphere(float lat, float lon, ImVec2 center, float radius) {
	
	lat = glm::radians(lat);
	lon = glm::radians(lon);

	
	float x = center.x + radius * cos(lat) * sin(lon);
	float y = center.y - radius * sin(lat) * 0.8f; 

	
	float edge = 1.0f - (x * x + y * y) / (radius * radius);
	if (edge < 0) edge = 0;

	return ImVec2(
		x * edge + center.x,
		y * edge + center.y
	);
}

void DrawPseudo3DEarth(ImDrawList* drawList, ImVec2 center, float radius) {
	
	const ImU32 earthColor = IM_COL32(100, 150, 255, 255);
	const ImU32 shadowColor = IM_COL32(50, 100, 200, 255);

	
	drawList->AddCircleFilled(center, radius, earthColor);
	for (int r = radius; r > 0; r -= 2) {
		float ratio = (float)r / radius;
		ImU32 color = IM_COL32(
			100 * ratio + 50,
			150 * ratio + 50,
			255 * ratio,
			255
		);
		drawList->AddCircle(center, r, color);
	}

	
	const int segments = 24;
	for (int i = 0; i <= segments; i++) {
		
		float angle = 2 * IM_PI * i / segments;
		ImVec2 p1(center.x + radius * sin(angle), center.y - radius * 0.5f * cos(angle));
		ImVec2 p2(center.x + radius * sin(angle), center.y + radius * 0.5f * cos(angle));
		drawList->AddLine(p1, p2, IM_COL32(200, 200, 255, 80));

		
		if (i % 3 == 0) {
			float latRadius = radius * cos(angle / 2);
			drawList->AddEllipse(
				center,
				ImVec2(latRadius, latRadius * 0.3f),
				IM_COL32(200, 200, 255, 80),
				0, 0, 36
			);
		}
	}
}





int draw_gui() {
	g_GuiRunning = true;
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"FrameImGui", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"FrameGui With DirectX11", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	
	bool done = false;
	bool show_statistical_window = true;
	bool show_draw_window = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	
	while (!done) {
		
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}

		

			
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();


		
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
				::Sleep(10);
				continue;
		}

			
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
				CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
				g_ResizeWidth = g_ResizeHeight = 0;
				CreateRenderTarget();
		}

			

		{
			
			std::lock_guard<std::mutex> lock(g_DataMutex);
			g_LonHistory.push_back(g_X);
			g_LatHistory.push_back(g_Y);
			g_AltHistory.push_back(g_Z);
			currentPos = LatLonToSphere(g_Y, g_X);
            historyPositions.push_back(currentPos);
			g_FrameCounter++;

			
			while ((int)g_AltHistory.size() > g_FrameRange) {
				g_AltHistory.erase(g_AltHistory.begin());
				g_LonHistory.erase(g_LonHistory.begin());
				g_LatHistory.erase(g_LatHistory.begin());
				historyPositions.erase(historyPositions.begin());
			}
			// set current point forward screen
			glm::vec3 targetDir = -glm::normalize(currentPos);
            float angle = atan2(targetDir.x, targetDir.z);
            earthMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0));

			

		}


			
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			float half_width = viewport->Size.x * 0.5f;
			float height = viewport->Size.y;

			
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




			

			
			ImGui::Render();
			const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
			
			//const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.00f };
			g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
			g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());



			
			static auto last_frame_time = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time).count();
			if (elapsed < 16) { // ~60fps
				std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed));
			}
			last_frame_time = now;

			
			g_pSwapChain->Present(1, 0);
		

	}
	g_GuiRunning = false;
	
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

 
