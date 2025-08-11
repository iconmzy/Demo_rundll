#pragma once
#include "Model.h"
#include <implot3d.h>

class GlobeView {
public:
    void Render(const GlobeModel& model) {
        if (ImPlot3D::BeginPlot("3D Globe")) {
            // 绘制球面经纬线网格
            DrawGlobeWireframe(model.radius);

            // 绘制点
            for (auto& p : model.points) {
                ImVec3 pos = LatLonToXYZ(p.lat, p.lon, model.radius);
                ImPlot3D::PlotScatter3D("Points", &pos.x, &pos.y, &pos.z, 1);
            }

            ImPlot3D::EndPlot();
        }
    }

private:
    void DrawGlobeWireframe(float radius) {
        const int lat_steps = 18;
        const int lon_steps = 36;
        for (int i = 0; i <= lat_steps; ++i) {
            float lat = -90.0f + i * (180.0f / lat_steps);
            std::vector<ImVec3> line;
            for (int j = 0; j <= lon_steps; ++j) {
                float lon = -180.0f + j * (360.0f / lon_steps);
                line.push_back(LatLonToXYZ(lat, lon, radius));
            }
            ImPlot3D::PlotLine3D("LatLines", &line[0].x, &line[0].y, &line[0].z, line.size());
        }
        for (int j = 0; j <= lon_steps; ++j) {
            float lon = -180.0f + j * (360.0f / lon_steps);
            std::vector<ImVec3> line;
            for (int i = 0; i <= lat_steps; ++i) {
                float lat = -90.0f + i * (180.0f / lat_steps);
                line.push_back(LatLonToXYZ(lat, lon, radius));
            }
            ImPlot3D::PlotLine3D("LonLines", &line[0].x, &line[0].y, &line[0].z, line.size());
        }
    }
};
