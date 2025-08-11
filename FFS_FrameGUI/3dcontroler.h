#pragma once
#include "Model.h"
#include "View.h"

class GlobeController {
public:
    GlobeModel model;
    GlobeView view;

    GlobeController() {
        // 添加测试点：北京、纽约、悉尼
        model.AddPoint(39.9042f, 116.4074f);
        model.AddPoint(40.7128f, -74.0060f);
        model.AddPoint(-33.8688f, 151.2093f);
    }

    void Update() {
        // 
    }

    void Render() {
        view.Render(model);
    }
};
