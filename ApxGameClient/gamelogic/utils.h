#pragma once
#include "vectors.h"

bool calculate_predict_angle(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle);

bool calculate_predict_angle_direct(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle);

bool calculate_predict_angle_directEx(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle);

bool calculate_predict_angleEx(vec3_t src, vec3_t dst, vec3_t player_v, vec3_t target_v, float v, float g, vec3_t* aim_angle);

vec3_t VectorToAngle(vec3_t src, vec3_t dst);

vec3_t AngleToVector(vec3_t angle);

bool world_to_screen(float* view_matrix, vec3_t world, vec2_t& screen, int resolutionWidth, int resolutionHeight);

bool OptimalPitch(float x, float y, float v, float g, float* out_pitch);

bool ComputeTrajectory(vec3_t offset, float v, float g, float* travel_time, vec3_t* out_angle);

int TargetRush(vec3_t aim_dir, vec3_t target_v);

float calc_apex_hipfire_hfov_16x9(float cl_fovscale);

float calc_ads_hfov_16x9(float m_targetZoomFOV_deg, float cl_fovscale);