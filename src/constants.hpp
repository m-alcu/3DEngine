#pragma once
constexpr float PI = 3.14159f;
constexpr float RAD = PI/180;
constexpr const char* RES_PATH = "resources/";
constexpr int SHADOW_MAP_SIZE = 512;
constexpr float SHADOW_BIAS_MIN = 0.01f;
constexpr float SHADOW_BIAS_MAX = 0.10f;
constexpr int SHADOW_PCF_RADIUS = 1;
constexpr int SHADOW_MAP_OVERVIEW_SIZE = 200;
constexpr float EFFECTIVE_LIGHT_RADIUS_FACTOR = 0.5f;

constexpr float CAMERA_DEFAULT_ZNEAR = 10.0f;
constexpr float CAMERA_DEFAULT_ZFAR = 10000.0f;
constexpr float CAMERA_DEFAULT_VIEW_ANGLE = 45.0f;

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
