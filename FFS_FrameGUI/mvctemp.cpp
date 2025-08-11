#include "implot3d.h"      // 引入 ImPlot3D 库头文件
#include <imgui.h>
#include <vector>
#include <mutex>

class FrameDataModel {
public:
    void Update(double lon, double lat, double alt, double yaw, double pitch, double roll) {
        std::lock_guard<std::mutex> lock(mtx_);
        longitude_ = lon;
        latitude_  = lat;
        altitude_  = alt;
        yaw_ = yaw; pitch_ = pitch; roll_ = roll;
        // 添加历史记录
        lon_history_.push_back((float)lon);
        lat_history_.push_back((float)lat);
        alt_history_.push_back((float)alt);
    }
    // 提供读取接口
    void Get(double& lon, double& lat, double& alt, double& yaw, double& pitch, double& roll) const {
        std::lock_guard<std::mutex> lock(mtx_);
        lon = longitude_; lat = latitude_; alt = altitude_;
        yaw = yaw_; pitch = pitch_; roll = roll_;
    }
    // 其他获取历史数据的方法...
private:
    mutable std::mutex mtx_;
    double longitude_ = 0, latitude_ = 0, altitude_ = 0;
    double yaw_ = 0, pitch_ = 0, roll_ = 0;
    std::vector<float> lon_history_, lat_history_, alt_history_;
};

// 全局模型实例（或使用智能指针管理）
static FrameDataModel g_Model;

// 控制器函数：更新模型
void UpdateFrameGUI_Data(double lon, double lat, double alt, double yaw, double pitch, double roll) {
    g_Model.Update(lon, lat, alt, yaw, pitch, roll);
}

int main() {
    // ... (初始化 Win32/DirectX/ImGui)
    ImGui::CreateContext();
    ImPlot3D::CreateContext(); // 必须在 ImGui 上下文创建后调用:contentReference[oaicite:7]{index=7}

    // 主渲染循环
    while (!done) {
        // ... 处理消息
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 示例：3D 地理球体窗口
        {
            ImGui::Begin("地理球体");
            if (ImPlot3D::BeginPlot("Globe")) {
                // 绘制单位球体网格（这里简化为绘制静态线框或使用Mesh）
                // 生成球面网格顶点和索引，然后：
                // ImPlot3D::PlotMesh("Earth", vertices.data(), indices.data(), (int)indices.size());

                // 绘制当前位置
                double lon, lat, alt, yaw, pitch, roll;
                g_Model.Get(lon, lat, alt, yaw, pitch, roll);
                float lat_rad = float(lat * M_PI / 180.0);
                float lon_rad = float(lon * M_PI / 180.0);
                float R = 1.0f; // 球半径
                float px = R * cosf(lat_rad) * cosf(lon_rad);
                float py = R * cosf(lat_rad) * sinf(lon_rad);
                float pz = R * sinf(lat_rad);
                ImPlot3D::PlotScatter("当前位置", &px, 1);

                ImPlot3D::EndPlot();
            }
            ImGui::End();
        }

        // 示例：传统统计信息窗口（视图从模型读取数据）
        {
            ImGui::Begin("帧统计");
            double lon, lat, alt, yaw, pitch, roll;
            g_Model.Get(lon, lat, alt, yaw, pitch, roll);
            ImGui::Text("Longitude: %.4f", lon);
            ImGui::Text("Latitude : %.4f", lat);
            ImGui::Text("Altitude : %.4f m", alt);
            ImGui::Text("Yaw: %.2f, Pitch: %.2f, Roll: %.2f", yaw, pitch, roll);
            ImGui::End();
        }

        // ... (其他窗口和逻辑)

        ImGui::Render();
        // ... 渲染 ImGui 绘制数据 ...
    }

    ImPlot3D::DestroyContext(); // 销毁 ImPlot3D 上下文:contentReference[oaicite:8]{index=8}
    ImGui::DestroyContext();
    return 0;
}
