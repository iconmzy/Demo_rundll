#pragma once
#include <vector>
#include <imgui.h>

struct GeoPoint {
    float lat;
    float lon;
};

class GlobeModel {
public:
    float radius = 1.0f;
    std::vector<GeoPoint> points;

    void AddPoint(float lat, float lon) {
        points.push_back({ lat, lon });
    }
};
