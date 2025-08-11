// ==== Model ====
// 存储经纬度数据，并转换为球面坐标
struct GeoPoint {
    double lat; // 纬度
    double lon; // 经度
};

class GlobeModel {
public:
    std::vector<GeoPoint> points;

    // 经纬度 -> 三维球面坐标 (半径 = r)
    static ImVec3 LatLonToXYZ(double lat, double lon, double r = 1.0) {
        double radLat = lat * M_PI / 180.0;
        double radLon = lon * M_PI / 180.0;
        double x = r * cos(radLat) * cos(radLon);
        double y = r * sin(radLat);
        double z = r * cos(radLat) * sin(radLon);
        return ImVec3((float)x, (float)y, (float)z);
    }
};

// ==== Controller ====
// 控制数据更新
class GlobeController {
public:
    GlobeModel& model;
    GlobeController(GlobeModel& m) : model(m) {}

    void AddPoint(double lat, double lon) {
        model.points.push_back({ lat, lon });
    }
};

// ==== View ====
// 使用 ImPlot3D 绘制
class GlobeView {
public:
    static void DrawSphereWithPoints(const GlobeModel& model) {
        if (ImPlot3D::BeginPlot3D("3D Globe", ImVec2(-1, 400))) {
            // 绘制球体网格
            DrawSphereMesh(1.0f, 36, 18);

            // 绘制点
            for (auto& p : model.points) {
                ImVec3 pos = GlobeModel::LatLonToXYZ(p.lat, p.lon);
                ImPlot3D::PlotScatter3D("Points", &pos.x, &pos.y, &pos.z, 1);
            }
            ImPlot3D::EndPlot3D();
        }
    }

private:
    static void DrawSphereMesh(float radius, int segsLon, int segsLat) {
        for (int i = 0; i <= segsLat; ++i) {
            float lat0 = M_PI * (-0.5f + (float)(i - 1) / segsLat);
            float z0 = sin(lat0);
            float zr0 = cos(lat0);

            float lat1 = M_PI * (-0.5f + (float)i / segsLat);
            float z1 = sin(lat1);
            float zr1 = cos(lat1);

            for (int j = 0; j <= segsLon; ++j) {
                float lng = 2 * M_PI * (float)(j - 1) / segsLon;
                float x = cos(lng);
                float y = sin(lng);
                ImVec3 v0(radius * x * zr0, radius * z0, radius * y * zr0);
                ImVec3 v1(radius * x * zr1, radius * z1, radius * y * zr1);
                ImPlot3D::PlotLine3D("Sphere", &v0.x, &v0.y, &v0.z, 1);
                ImPlot3D::PlotLine3D("Sphere", &v1.x, &v1.y, &v1.z, 1);
            }
        }
    }
};








/*

void DrawUI() {
    static GlobeModel model;
    static GlobeController controller(model);

    // 仅初始化一次
    static bool initialized = false;
    if (!initialized) {
        controller.AddPoint(0, 0);        // 赤道 & 本初子午线
        controller.AddPoint(30, 120);     // 东半球
        controller.AddPoint(-45, -60);    // 南半球
        initialized = true;
    }

    GlobeView::DrawSphereWithPoints(model);
}





*/