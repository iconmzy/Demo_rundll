#define FRAMEGUI_EXPORTS
#include"framegui.h"
#include <d3d11.h>
#include <tchar.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui.h>
#include <algorithm>
#include <vector>
#include <string>
#include <chrono>


// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;





//static bool g_DataUpdated = true;  
static double g_X = 0.0f;
static double g_Y = 0.0f;
static double g_Z = 500.0f;
static double yaw_dur = 0.f;
static double pitch_dur = 0.f;
static double roll_dur = 0.f;

static int g_FrameRange = 100;
static int g_FrameCounter = 0;
static std::chrono::steady_clock::time_point g_CurrentSecondStartTime;
static bool g_FirstUpdate = true;
static int currnetFramePerSecond = 0;
static int currnetFramePerSecondRange = 10;
static double longitudeCurrentMin = 0.0f;
static double longitudeCurrentMax = 0.0f;
static double latitudeCurrentMin = 0.0f;
static double latitudeCurrentMax = 0.0f;






//to do,how to define error Frames
static std::vector<std::string> g_ErrorFrames;
static std::vector<int> historyFrameCountPerSecond;
static std::vector<float> g_AltHistory;
static std::vector<float> g_LonHistory;
static std::vector < float> g_LatHistory;  
     
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


FRAMEGUI_API std::mutex g_DataMutex;

//make sure auomic operate
FRAMEGUI_API std::atomic<bool> g_GuiRunning(false);

FRAMEGUI_API void UpdateFrameGUI_Data(double x, double y, double z,double yaw, double  pitch, double roll){
	std::lock_guard<std::mutex> lock(g_DataMutex);
	g_X = x;
	g_Y = y;
	g_Z = z;
	//g_DataUpdated = true;
    yaw_dur = yaw;
    pitch_dur = pitch;
    roll_dur = roll;
    
    auto current_time = std::chrono::steady_clock::now();

    
    if (g_FirstUpdate)
    {
        g_CurrentSecondStartTime = current_time;
        currnetFramePerSecond = 0;
        g_FirstUpdate = false;
    }

    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(current_time - g_CurrentSecondStartTime);
    double elapsed_seconds = double(duration.count());

    
    if (elapsed_seconds >= 1.0)
    {
        addFixVectorElement(currnetFramePerSecond, historyFrameCountPerSecond, currnetFramePerSecondRange);
        currnetFramePerSecond = 0;                
        g_CurrentSecondStartTime = current_time;   
    }

    currnetFramePerSecond++;
    g_FrameCounter++;
    // history windows
    g_LonHistory.push_back(float(g_X));
    g_LatHistory.push_back(float(g_Y));
    g_AltHistory.push_back(float(g_Z));
}





void AdjustDisplayRange(double currentValue, double& minRange, double& maxRange, double baseRange = 0.02f,double edgeRatio = 0.1f)
{
    const double range = maxRange - minRange;
    const double edgeThreshold = range * edgeRatio;   

    
    const double t = (currentValue - minRange) / range;

    
    if (t < edgeRatio)
    {
        minRange = currentValue - edgeThreshold;
        maxRange = minRange + baseRange;
    }
    
    else if (t > (1.0f - edgeRatio))
    {
        maxRange = currentValue + edgeThreshold;
        minRange = maxRange - baseRange;
    }
    
}


void DrawLongitudeGauge(ImDrawList* drawList, ImVec2 center, float radius, double longitude)
{
    std::lock_guard<std::mutex> lock(g_DataMutex);
    const double EPS = 1e-9;   
    //const double MIN_RANGE = 1e-6; 
    
    if (fabs(longitudeCurrentMin - longitudeCurrentMax) < EPS)
    {
        longitudeCurrentMin = longitude - 0.02f;
        longitudeCurrentMax = longitude + 0.02f;
    }

    
    AdjustDisplayRange(longitude, longitudeCurrentMin, longitudeCurrentMax);
     double minValue = longitudeCurrentMin;
     double maxValue = longitudeCurrentMax;
//      double range = maxValue - minValue;
//     if (range < MIN_RANGE){
//         double center_val = (minValue + maxValue) / 2.0;
//         latitudeCurrentMin = center_val - MIN_RANGE / 2.0;
//         latitudeCurrentMax = center_val + MIN_RANGE / 2.0;
//         minValue = longitudeCurrentMin;
//         maxValue = longitudeCurrentMax;
//         range = MIN_RANGE;
//     }



    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(255, 100, 100, 255);
    ImU32 edgeWarningColor = IM_COL32(255, 200, 0, 255);   
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

    const int tickCount = 50;
    for (int i = 0; i <= tickCount; ++i)
    {
        const float t = (float)i / tickCount;
        const float value = float(minValue + (maxValue - minValue) * t);
        const float angle = IM_PI * t;
        const bool isMajorTick = (i % 10) == 0;
        const float tickLength = isMajorTick ? 10.0f : 5.0f;

        
        const ImVec2 p1 = ImVec2(center.x + cosf(angle) * (radius - tickLength), center.y - sinf(angle) * (radius - tickLength));
        const ImVec2 p2 = ImVec2(center.x + cosf(angle) * radius, center.y - sinf(angle) * radius);

        
        ImU32 tickColor = isMajorTick ? majorTickColor : minorTickColor;
        if (t < 0.1f || t > 0.9f)
            tickColor = edgeWarningColor;

        drawList->AddLine(p1, p2, tickColor, isMajorTick ? 2.0f : 1.0f);

        
        if (isMajorTick)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.5f", value);
            const ImVec2 textSize = ImGui::CalcTextSize(buf);
            const ImVec2 textPos = ImVec2(center.x + cosf(angle) * (radius - 25) - textSize.x * 0.5f, center.y - sinf(angle) * (radius - 25) - textSize.y * 0.5f);
            drawList->AddText(textPos, textColor, buf);
        }
    }

    
    float needleAngle = float(((longitude - minValue) / (maxValue - minValue)) * IM_PI);
    const ImVec2 needleEnd = ImVec2(center.x + cosf(needleAngle) * (radius - 15), center.y - sinf(needleAngle) * (radius - 15));

    
    drawList->AddLine(center, needleEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(needleEnd, 5.0f, needleColor);

    
    drawList->AddLine(ImVec2(center.x + cosf(0) * (radius - 5), center.y - sinf(0) * (radius - 5)), ImVec2(center.x + cosf(0) * radius, center.y - sinf(0) * radius), edgeWarningColor, 2.0f);
    drawList->AddLine(ImVec2(center.x + cosf(IM_PI) * (radius - 5), center.y - sinf(IM_PI) * (radius - 5)), ImVec2(center.x + cosf(IM_PI) * radius, center.y - sinf(IM_PI) * radius), edgeWarningColor, 2.0f);
}




void DrawLatitudeGauge(ImDrawList* drawList, ImVec2 center, float radius, double latitude)
{
    //todo. here is will bug after longterm runing
    //info Expression: vector iterators in range are from different containers
    std::lock_guard<std::mutex> lock(g_DataMutex);
    const double EPS = 1e-9;
    //const double MIN_RANGE = 1e-6; 
    if (fabs(latitudeCurrentMin - latitudeCurrentMax) < EPS)
    {
        latitudeCurrentMin = latitude - 0.02f;
        latitudeCurrentMax = latitude + 0.02f;
    }

    
    AdjustDisplayRange(latitude, latitudeCurrentMin, latitudeCurrentMax);
     double minValue = latitudeCurrentMin;
     double maxValue = latitudeCurrentMax;

//      double range = maxValue - minValue;
//     if (range < MIN_RANGE)
//     {
//         double center_val = (minValue + maxValue) / 2.0;
//         latitudeCurrentMin = center_val - MIN_RANGE / 2.0;
//         latitudeCurrentMax = center_val + MIN_RANGE / 2.0;
//         minValue = latitudeCurrentMin;
//         maxValue = latitudeCurrentMax;
//         range = MIN_RANGE;
//     }

    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(100, 200, 255, 255);      
    ImU32 edgeWarningColor = IM_COL32(255, 200, 0, 255);   
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

    const int tickCount = 50;
    for (int i = 0; i <= tickCount; ++i)
    {
        const float t = (float)i / tickCount;
        const float value = float(minValue + (maxValue - minValue) * t);
        const float angle = IM_PI * t;  
        const bool isMajorTick = (i % 10) == 0;
        const float tickLength = isMajorTick ? 10.0f : 5.0f;

        
        const ImVec2 p1 = ImVec2(center.x + cosf(angle) * (radius - tickLength), center.y - sinf(angle) * (radius - tickLength));
        const ImVec2 p2 = ImVec2(center.x + cosf(angle) * radius, center.y - sinf(angle) * radius);

        
        ImU32 tickColor = isMajorTick ? majorTickColor : minorTickColor;
        if (t < 0.1f || t > 0.9f)
            tickColor = edgeWarningColor;

        drawList->AddLine(p1, p2, tickColor, isMajorTick ? 2.0f : 1.0f);

       
        if (isMajorTick)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.5f", value);
            const ImVec2 textSize = ImGui::CalcTextSize(buf);
            const ImVec2 textPos = ImVec2(center.x + cosf(angle) * (radius - 30) - textSize.x * 0.5f, center.y - sinf(angle) * (radius - 30) - textSize.y * 0.5f);
            drawList->AddText(textPos, textColor, buf);
        }
    }

    
     float needleAngle = float(((latitude - minValue) / (maxValue - minValue)) * IM_PI);
    const ImVec2 needleEnd = ImVec2(center.x + cosf(needleAngle) * (radius - 15), center.y - sinf(needleAngle) * (radius - 15));

    
    drawList->AddLine(center, needleEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(needleEnd, 5.0f, needleColor);

    
    drawList->AddLine(ImVec2(center.x + cosf(0) * (radius - 5), center.y - sinf(0) * (radius - 5)), ImVec2(center.x + cosf(0) * radius, center.y - sinf(0) * radius), edgeWarningColor, 2.0f);
    drawList->AddLine(ImVec2(center.x + cosf(IM_PI) * (radius - 5), center.y - sinf(IM_PI) * (radius - 5)), ImVec2(center.x + cosf(IM_PI) * radius, center.y - sinf(IM_PI) * radius), edgeWarningColor, 2.0f);
}





void DrawDetailLongitudeGauge(ImDrawList* drawList, ImVec2 center, float radius, float longitude)
{
    
    float range = 0.01f;  
    float minValue = longitude - range;
    float maxValue = longitude + range;

    //ImU32 ringColor = IM_COL32(80, 150, 255, 255);
    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(255, 100, 100, 255);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

    
    //drawList->PathArcTo(center, radius, IM_PI * 0.5f, IM_PI * 1.5f);
    //drawList->PathStroke(ringColor, false, 2.0f);

    const int tickCount = 50;
    for (int i = 0; i <= tickCount; ++i)
    {
        float t = (float)i / tickCount;
        float value = minValue + (maxValue - minValue) * t;

        float angle =  t * IM_PI;   
        bool isMajorTick = (i % 10) == 0;
        float tickLength = isMajorTick ? 10.0f : 5.0f;

        
        ImVec2 p1 = ImVec2(center.x + cosf(angle) * (radius - tickLength), center.y - sinf(angle) * (radius - tickLength));
        ImVec2 p2 = ImVec2(center.x + cosf(angle) * radius, center.y - sinf(angle) * radius);
        drawList->AddLine(p1, p2, isMajorTick ? majorTickColor : minorTickColor, isMajorTick ? 2.0f : 1.0f);

        
        if (isMajorTick)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.5f", value);
            ImVec2 textSize = ImGui::CalcTextSize(buf);
            ImVec2 textPos = ImVec2(center.x + cosf(angle) * (radius - 25) - textSize.x * 0.5f, center.y - sinf(angle) * (radius - 25) - textSize.y * 0.5f);
            drawList->AddText(textPos, textColor, buf);
        }
    }

    
    float needleAngle =  ((longitude - minValue) / (maxValue - minValue)) * IM_PI;
    ImVec2 needleEnd = ImVec2(center.x + cosf(needleAngle) * (radius - 15), center.y - sinf(needleAngle) * (radius - 15));
    drawList->AddLine(center, needleEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(needleEnd, 5.0f, needleColor);
}

void DrawDetailLatitudeGauge(ImDrawList* drawList, ImVec2 center, float radius, float latitude)
{
    
    float range = 0.01f;  
    float minValue = latitude - range;
    float maxValue = latitude + range;

    //ImU32 ringColor = IM_COL32(80, 150, 255, 255);
    ImU32 majorTickColor = IM_COL32(200, 200, 200, 220);
    ImU32 minorTickColor = IM_COL32(150, 150, 150, 180);
    ImU32 needleColor = IM_COL32(100, 200, 255, 255);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

   
    //drawList->PathArcTo(center, radius, 0.0f, IM_PI * 0.5f);
    //drawList->PathStroke(ringColor, false, 2.0f);

    const int tickCount = 50;
    for (int i = 0; i <= tickCount; ++i)
    {
        float t = (float)i / tickCount;
        float value = minValue + (maxValue - minValue) * t;

        float angle = IM_PI * t;   
        bool isMajorTick = (i % 10) == 0;
        float tickLength = isMajorTick ? 10.0f : 5.0f;

        
        ImVec2 p1 = ImVec2(center.x + cosf(angle) * (radius - tickLength), center.y - sinf(angle) * (radius - tickLength));
        ImVec2 p2 = ImVec2(center.x + cosf(angle) * radius, center.y - sinf(angle) * radius);
        drawList->AddLine(p1, p2, isMajorTick ? majorTickColor : minorTickColor, isMajorTick ? 2.0f : 1.0f);

        
        if (isMajorTick)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.5f", value);
            ImVec2 textSize = ImGui::CalcTextSize(buf);
            ImVec2 textPos = ImVec2(center.x + cosf(angle) * (radius - 30) - textSize.x * 0.5f, center.y - sinf(angle) * (radius - 30) - textSize.y * 0.5f);
            drawList->AddText(textPos, textColor, buf);
        }
    }

    
    float needleAngle = ((latitude - minValue) / (maxValue - minValue)) * IM_PI;
    ImVec2 needleEnd = ImVec2(center.x + cosf(needleAngle) * (radius - 15), center.y - sinf(needleAngle) * (radius - 15));
    drawList->AddLine(center, needleEnd, needleColor, 3.0f);
    drawList->AddCircleFilled(needleEnd, 5.0f, needleColor);
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
 			// g_FrameRange is editable
            std::lock_guard<std::mutex> lock(g_DataMutex); 
 			while (g_AltHistory.size() > g_FrameRange) {
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
					ImGui::Text("%.4f ", g_X);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Latitude");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.4f", g_Y);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Altitude");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.4f m", g_Z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("yaw_dur");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f ", yaw_dur);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("pitch_dur");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f ", pitch_dur);


                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("roll_dur");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f ", roll_dur);

					ImGui::EndTable();
				}

				ImGui::Spacing();
                ImGui::Text("Receive: %.d Frame in current second", currnetFramePerSecond);
                ImGui::NewLine();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Frame Rate History");
                ImGui::Text("Showing last %d seconds frame counts", historyFrameCountPerSecond.size());
                const int COLUMNS_PER_ROW = 5;   
                int totalCount = int(historyFrameCountPerSecond.size());
               
                if (totalCount > 0)
                {
                    
                    int totalRows = (totalCount + COLUMNS_PER_ROW - 1) / COLUMNS_PER_ROW;

                    
                    for (int row = 0; row < totalRows; ++row)
                    {
                        
                        int startIndex = row * COLUMNS_PER_ROW;
                        int columnsInRow = std::min(COLUMNS_PER_ROW, totalCount - startIndex);

                        
                        std::string tableId = "FrameHistoryRow" + std::to_string(row);
                        if (ImGui::BeginTable(tableId.c_str(), columnsInRow, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
                        {
                            
                            for (int col = 0; col < columnsInRow; ++col)
                            {
                                int dataIndex = startIndex + col;
                                int age = totalCount - 1 - dataIndex;   
                                std::string header = age == 0 ? "Now" : ("T-" + std::to_string(age));
                                ImGui::TableSetupColumn(header.c_str(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
                            }
                            ImGui::TableHeadersRow();

                             // todo,how to define error frame with config
                            ImGui::TableNextRow();
                            for (int col = 0; col < columnsInRow; ++col)
                            {
                                ImGui::TableSetColumnIndex(col);
                                int frameCount = historyFrameCountPerSecond[startIndex + col];
                                if (frameCount < 0)
                                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%d", frameCount);
                                else if (frameCount > 60)
                                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", frameCount);
                                else
                                    ImGui::Text("%d", frameCount);
                            }

                            ImGui::EndTable();
                        }
 
                        ImGui::Spacing();
                    }
                }
                else{
                    ImGui::Text("No history data available");
                }
                ImGui::Separator();
                ImGui::Text("Error Frames");
                ImGui::BeginChild("ErrorFramesChild", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
                for (const auto& frame : g_ErrorFrames)
                {
                    ImGui::TextUnformatted(frame.c_str());
                }
                ImGui::EndChild();
				ImGui::EndChild();
			}
			ImGui::End();






           ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + half_width, viewport->Pos.y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(half_width, height * 0.5f), ImGuiCond_Always);
            if (ImGui::Begin("Altitude Plot", &show_draw_window, ImGuiWindowFlags_MenuBar))
            {
                if (!g_AltHistory.empty())
                {
                    
                    float min_alt = *std::min_element(g_AltHistory.begin(), g_AltHistory.end());
                    float max_alt = *std::max_element(g_AltHistory.begin(), g_AltHistory.end());

                    
                    const float padding = 0.5f;   
                    if (max_alt - min_alt < 0.1f)
                    {
                        min_alt -= padding;
                        max_alt += padding;
                    }
                    else
                    {
                        min_alt -= padding;
                        max_alt += padding;
                    }

                    
                    ImVec2 plot_size(ImGui::GetContentRegionAvail().x *0.9f, ImGui::GetContentRegionAvail().y * 0.7f);   
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 60);                                                  

                   
                    ImGui::PlotLines("##AltitudePlot",   
                                     g_AltHistory.data(),
                                     (int)g_AltHistory.size(),
                                     0,
                                     nullptr,
                                     min_alt,
                                     max_alt,
                                     plot_size);

                    
                    ImVec2 plot_min = ImGui::GetItemRectMin();
                    ImVec2 plot_max = ImGui::GetItemRectMax();
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();

                    
                    const int y_tick_count = 5;   
                    for (int i = 0; i <= y_tick_count; ++i)
                    {
                        
                        float ratio = (float)i / y_tick_count;
                        float y_value = min_alt + (max_alt - min_alt) * (1.0f - ratio);   

                        
                        float y_pos = plot_min.y + (plot_max.y - plot_min.y) * ratio;

                       
                        draw_list->AddLine(ImVec2(plot_min.x, y_pos),
                                           ImVec2(plot_max.x, y_pos),
                                           IM_COL32(80, 80, 80, 255),   
                                           1.0f);

                        
                        char label[32];
                        snprintf(label, sizeof(label), "%.2f", y_value);
                        ImVec2 text_size = ImGui::CalcTextSize(label);
                        draw_list->AddText(ImVec2(plot_min.x - text_size.x - 5, y_pos - text_size.y / 2),
                                           IM_COL32(255, 255, 255, 255),   
                                           label);
                    }

                    
                    const int x_tick_count = 5;
                    for (int i = 0; i <= x_tick_count; ++i)
                    {
                        float ratio = (float)i / x_tick_count;
                        int frame_idx = (int)(g_AltHistory.size() * ratio);

                       
                        float x_pos = plot_min.x + (plot_max.x - plot_min.x) * ratio;

                        
                        draw_list->AddLine(ImVec2(x_pos, plot_min.y), ImVec2(x_pos, plot_max.y), IM_COL32(80, 80, 80, 255), 1.0f);

                        
                        char label[32];
                        snprintf(label, sizeof(label), "%d", frame_idx);
                        ImVec2 text_size = ImGui::CalcTextSize(label);
                        draw_list->AddText(ImVec2(x_pos - text_size.x / 2, plot_max.y + 5), IM_COL32(255, 255, 255, 255), label);
                    }

                    

                    ImGui::NewLine(); 
                    ImGui::Spacing();
                    ImGui::Text("Current Altitude: %.4f m", g_Z);
                    ImGui::Text("Frame Count: %d", g_FrameCounter);
                    ImGui::SliderInt("Frame Range", &g_FrameRange, 10, 200, "%d frames");
                }
                else
                {
                    ImGui::Text("Waiting for data...");
                }
            }
            ImGui::End();



            {
                ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + half_width, viewport->Pos.y + half_height), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(half_width / 2, half_height), ImGuiCond_Always);
                if (ImGui::Begin("Longitude", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
                    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
                    ImVec2 center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.5f);
                    float radius = std::min(canvas_sz.x, canvas_sz.y) * 0.5f;

                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y), IM_COL32(0, 0, 0, 255));

                    // Draw Longitude Gauge
                    DrawLongitudeGauge(draw_list, center, radius, g_X);

                    ImGui::End();
                }
            }

            {
                ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + half_width + half_width / 2, viewport->Pos.y + half_height), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(half_width / 2, half_height), ImGuiCond_Always);
                if (ImGui::Begin("Latitude", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
                    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
                    ImVec2 center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.5f);
                    float radius = std::min(canvas_sz.x, canvas_sz.y) * 0.5f;

                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y), IM_COL32(0, 0, 0, 255));

                    // Draw Latitude Gauge
                    DrawLatitudeGauge(draw_list, center, radius, g_Y);

                    ImGui::End();
                }
            }










			
			ImGui::Render();
			const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
			g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
			g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			
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

 
