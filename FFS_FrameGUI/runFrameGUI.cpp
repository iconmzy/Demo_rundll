#define FRAMEGUI_EXPORTS
#include"framegui.h"
#include <d3d11.h>
#include <tchar.h>
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui.h"
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
     
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


FRAMEGUI_API std::mutex g_DataMutex;

//make sure auomic operate
FRAMEGUI_API std::atomic<bool> g_GuiRunning(false);

FRAMEGUI_API void UpdateFrameGUI_Data(float x, float y, float z)
{
	std::lock_guard<std::mutex> lock(g_DataMutex);
	g_X = x;
	g_Y = y;
	g_Z = z;
	//g_DataUpdated = true;
}



void DrawLatLonGauge(ImDrawList* drawList, ImVec2 center, float radius, float latitude, float longitude)
{
    
    ImU32 ringColor = IM_COL32(80, 150, 255, 255);
    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(255, 100, 100, 255);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);
    ImU32 highlightColor = IM_COL32(255, 255, 0, 200);

    
    drawList->AddCircle(center, radius, ringColor, 128, 2.0f);

  
    for (int i = 0; i < 360; i += 1)
    {
        float angle = glm::radians((float)i);
        bool isMajorTick = (i % 5) == 0;
        float tickLength = isMajorTick ? 8.0f : 4.0f;
        float innerRadius = isMajorTick ? radius - tickLength : radius - tickLength;

        float x1 = center.x + cosf(angle) * innerRadius;
        float y1 = center.y + sinf(angle) * innerRadius;
        float x2 = center.x + cosf(angle) * radius;
        float y2 = center.y + sinf(angle) * radius;

        drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), isMajorTick ? majorTickColor : minorTickColor, isMajorTick ? 1.5f : 1.0f);

        
        if (isMajorTick && (i % 15) == 0)
        {
            ImVec2 textPos = ImVec2(center.x + cosf(angle) * (radius - 15), center.y + sinf(angle) * (radius - 15));
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", i);
            ImVec2 textSize = ImGui::CalcTextSize(buf);
            drawList->AddText(ImVec2(textPos.x - textSize.x * 0.5f, textPos.y - textSize.y * 0.5f), textColor, buf);
        }
    }

    
    float lonAngle = glm::radians(longitude);
    ImVec2 lonEnd = ImVec2(center.x + sin(lonAngle) * (radius - 5),   
                           center.y - cos(lonAngle) * (radius - 5));
    drawList->AddLine(center, lonEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(lonEnd, 4.0f, needleColor);  

   
    float latAngle = glm::radians(latitude + 90.0f);   
    ImVec2 latEnd = ImVec2(center.x + sin(latAngle) * (radius - 5), center.y - cos(latAngle) * (radius - 5));
    drawList->AddLine(center, latEnd, IM_COL32(100, 200, 255, 255), 3.0f);
    drawList->AddCircleFilled(latEnd, 4.0f, IM_COL32(100, 200, 255, 255));

    
    //char lonText[32], latText[32];
    //snprintf(lonText, sizeof(lonText), "Lon: %.4f°", longitude);
    //snprintf(latText, sizeof(latText), "Lat: %.4f°", latitude);

    // 在圆下方添加数值显示
    //ImVec2 lonTextSize = ImGui::CalcTextSize(lonText);
    //ImVec2 latTextSize = ImGui::CalcTextSize(latText);

    //drawList->AddText(ImVec2(center.x - lonTextSize.x * 0.5f, center.y + radius + 5), textColor, lonText);
    //drawList->AddText(ImVec2(center.x - latTextSize.x * 0.5f, center.y + radius + 25), textColor, latText);

    // 添加当前值附近的刻度高亮
    float highlightStart = glm::radians(longitude - 10);
    float highlightEnd = glm::radians(longitude + 10);
    drawList->PathArcTo(center, radius + 5, highlightStart, highlightEnd);
    drawList->PathStroke(highlightColor, false, 3.0f);
}




void DrawLongitudeGauge(ImDrawList* drawList, ImVec2 center, float radius, float longitude)
{
    // 颜色定义
    ImU32 ringColor = IM_COL32(80, 150, 255, 255);
    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(255, 100, 100, 255);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

    // 绘制半圆 (从90°到270°)
    drawList->PathArcTo(center, radius, IM_PI * 0.5f, IM_PI * 1.5f);
    drawList->PathStroke(ringColor, false, 2.0f);

    // 精细刻度 (每1°小刻度，每5°大刻度)
    for (int i = 0; i <= 180; i += 1)
    {
        float angle = glm::radians(static_cast<float>(i) + 90.0f);
        bool isMajorTick = (i % 5) == 0;
        float tickLength = isMajorTick ? 10.0f : 5.0f;

        // 计算刻度线位置
        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);
        ImVec2 innerPoint(center.x + cosAngle * (radius - tickLength), center.y + sinAngle * (radius - tickLength));
        ImVec2 outerPoint(center.x + cosAngle * radius, center.y + sinAngle * radius);

        drawList->AddLine(innerPoint, outerPoint, isMajorTick ? majorTickColor : minorTickColor, isMajorTick ? 2.0f : 1.0f);

        // 每10°添加数字标注
        if (isMajorTick && (i % 10) == 0)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", i);
            ImVec2 textSize = ImGui::CalcTextSize(buf);

            // 文本位置 (向圆心内偏移)
            ImVec2 textPos(center.x + cosAngle * (radius - 25), center.y + sinAngle * (radius - 25));

            // 调整文本位置避免重叠
            textPos.x -= textSize.x * 0.5f;
            textPos.y -= textSize.y * 0.5f;

            drawList->AddText(textPos, textColor, buf);
        }
    }

    // 经度指针 (转换为半圆角度)
    float lonAngle = glm::radians(longitude + 90.0f);
    ImVec2 lonEnd(center.x + cosf(lonAngle) * (radius - 15), center.y + sinf(lonAngle) * (radius - 15));

    drawList->AddLine(center, lonEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(lonEnd, 5.0f, needleColor);
}

// 纬度仪表盘绘制函数 (1/4圆)
void DrawLatitudeGauge(ImDrawList* drawList, ImVec2 center, float radius, float latitude)
{
    // 颜色定义
    ImU32 ringColor = IM_COL32(80, 150, 255, 255);
    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(100, 200, 255, 255);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

    // 绘制1/4圆 (从0°到90°)
    drawList->PathArcTo(center, radius, 0.0f, IM_PI * 0.5f);
    drawList->PathStroke(ringColor, false, 2.0f);

    // 精细刻度 (每1°小刻度，每5°大刻度)
    for (int i = 0; i <= 90; i += 1)
    {
        float angle = glm::radians(static_cast<float>(i));
        bool isMajorTick = (i % 5) == 0;
        float tickLength = isMajorTick ? 10.0f : 5.0f;

        // 计算刻度线位置
        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);
        ImVec2 innerPoint(center.x + cosAngle * (radius - tickLength), center.y - sinAngle * (radius - tickLength));
        ImVec2 outerPoint(center.x + cosAngle * radius, center.y - sinAngle * radius);

        drawList->AddLine(innerPoint, outerPoint, isMajorTick ? majorTickColor : minorTickColor, isMajorTick ? 2.0f : 1.0f);

        // 每5°添加数字标注
        if (isMajorTick)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", i);
            ImVec2 textSize = ImGui::CalcTextSize(buf);

            // 文本位置 (向圆心内偏移)
            ImVec2 textPos(center.x + cosAngle * (radius - 30), center.y - sinAngle * (radius - 30));

            // 调整文本位置避免重叠
            textPos.x -= textSize.x * 0.5f;
            textPos.y -= textSize.y * 0.5f;

            drawList->AddText(textPos, textColor, buf);
        }
    }

    // 纬度指针
    float latAngle = glm::radians(latitude);
    ImVec2 latEnd(center.x + cosf(latAngle) * (radius - 15), center.y - sinf(latAngle) * (radius - 15));

    drawList->AddLine(center, latEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(latEnd, 5.0f, needleColor);
}


FRAMEGUI_API int draw_gui()
{
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
		// handle message
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
			// history windows 
			std::lock_guard<std::mutex> lock(g_DataMutex);
			g_LonHistory.push_back(g_X);
			g_LatHistory.push_back(g_Y);
			g_AltHistory.push_back(g_Z);
			g_FrameCounter++;

			// g_FrameRange is editable
			while ((int)g_AltHistory.size() > g_FrameRange) {
				g_AltHistory.erase(g_AltHistory.begin());
				g_LonHistory.erase(g_LonHistory.begin());
				g_LatHistory.erase(g_LatHistory.begin());
			}

		}


			
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			float half_width = viewport->Size.x * 0.5f;
			float height = viewport->Size.y;
			float half_height = viewport->Size.y * 0.5f;

			// left window for Statistics
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

			// right side windows for plot
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




			{


  
                ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + half_width, viewport->Pos.y + half_height), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(half_width/2, half_height), ImGuiCond_Always);
                if (ImGui::Begin("Longitude", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
                    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
                    ImVec2 center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.7f);
                    float radius = std::min(canvas_sz.x, canvas_sz.y) * 0.4f;

                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y), IM_COL32(20, 20, 20, 255));
                    DrawLongitudeGauge(draw_list, center, radius, g_X);
                }
                ImGui::End();

               
                ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + half_width *1.5f, viewport->Pos.y + half_height), ImGuiCond_Always);
          
               ImGui::SetNextWindowSize(ImVec2(half_width/2, half_height), ImGuiCond_Always);
                if (ImGui::Begin("Latitude", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
                    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
                    ImVec2 center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.7f);
                    float radius = std::min(canvas_sz.x, canvas_sz.y) * 0.4f;

                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y), IM_COL32(20, 20, 20, 255));
                    DrawLatitudeGauge(draw_list, center, radius, g_Y);
                }
                ImGui::End();
            }









			
			ImGui::Render();
			const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
			g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
			g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());






// 			
// 			static auto last_frame_time = std::chrono::steady_clock::now();
// 			auto now = std::chrono::steady_clock::now();
// 			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time).count();
// 			if (elapsed < 16) { 
// 				std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed));
// 			}
// 			last_frame_time = now;
// 
// 			
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

 
