

#pragma once



#ifdef FRAMEGUI_EXPORTS
#define FRAMEGUI_API __declspec(dllexport)
#else
#define FRAMEGUI_API __declspec(dllimport)
#endif




#include "ImGui/imconfig.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imstb_rectpack.h"
#include "ImGui/imstb_textedit.h"
#include "ImGui/imstb_truetype.h"
#include "ImGui/implot.h"
#include "ImGui/implot_internal.h"
#include "ImGui/implot3d.h"
#include "ImGui/implot3d_internal.h"
#include "ImGui/ImGuizmo.h"

#include <d3d11.h>
#include <mutex>
#include <atomic>
extern std::mutex g_DataMutex;
extern std::atomic<bool> g_GuiRunning;
#pragma comment(lib,"d3d11.lib")


extern "C" {
	FRAMEGUI_API int draw_gui();
	FRAMEGUI_API void UpdateData(float x, float y, float z);
	
}