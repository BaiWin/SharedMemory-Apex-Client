#include "utils.h"
#include "ClientIncludes.h"

float AverageSpeed(float totalPixels)
{
	// 调整后的平均速度模型：v_avg = 500 * ln(m) - 500 (像素/秒)
	if (totalPixels < 100.0f) totalPixels = 100.0f;
	return 400.0f * log(totalPixels) - 600.0f; // 400, 600
}

// 30~
bool calculate_predict_angle(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle)
{
	InsertJunkCodeRND();
	float time = dst.distance_to(src) / v;
	if (time > 10.0f) return false;
	float min_time = time * 0.5f;
	float max_time = time * 2.0f;
	InsertJunkCodeRND();
	float time_step = 1.f / 256.f;
	for (float t = min_time; t <= max_time; t += time_step)
	{
		vec3_t target_pos = dst + target_v * (t + 0.040f); // * (0.9f + time * 10) // vec3_t target_pos = dst + target_v * t;
		vec3_t offset = target_pos - src;
		float travel_time;
		vec3_t out_angle;
		if (ComputeTrajectory(offset, v, g, &travel_time, &out_angle))
		{
			if (travel_time < t)
			{
				vec3_t angle = VectorToAngle(src, target_pos);
				*aim_angle = vec3_t(-out_angle.x * (180.0f / M_PI), angle.y, 0);
				return true;
			}
		}
	}
	return false;
}

// 10~30
bool calculate_predict_angle_direct(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle)
{
	InsertJunkCodeRND();
	float time = dst.distance_to(src) / v;
	vec3_t target_pos = dst + target_v * (time + 0.020f); //1.0f / 14.0f);
	vec3_t angle = VectorToAngle(src, target_pos);
	*aim_angle = angle;
	return true;
}

// 0~10
bool calculate_predict_angle_directEx(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle)
{
	InsertJunkCodeRND();
	float time = dst.distance_to(src) / v;
	vec3_t target_pos = dst + target_v * (time + 0.020f); // target_v * 1.0f / 80.0f;
	vec3_t angle = VectorToAngle(src, target_pos);
	*aim_angle = angle;
	return true;
}

bool calculate_predict_angleEx(vec3_t src, vec3_t dst, vec3_t player_v, vec3_t target_v, float v, float g, vec3_t* aim_angle)
{
	InsertJunkCodeRND();
	float time = dst.distance_to(src) / v;
	float min_time = time;
	float max_time = time * 2.0f;
	InsertJunkCodeRND();
	float time_step = 1.f / 256.f;
	for (float t = min_time; t <= max_time; t += time_step)
	{
		vec3_t target_pos = dst + target_v * t;
		vec3_t offset = target_pos - src; //  - player_v * t / 2)
		float travel_time;
		vec3_t out_angle;
		if (ComputeTrajectory(offset, v, g, &travel_time, &out_angle))
		{
			if (travel_time < t)
			{
				vec3_t angle = VectorToAngle(src, target_pos);
				*aim_angle = vec3_t(-out_angle.x * (180.0f / M_PI), angle.y, 0);
				return true;
			}
		}
	}
	return false;
}

vec3_t VectorToAngle(vec3_t src, vec3_t dst)
{
	vec3_t offset = src - dst;

	double temp = sqrt(pow(offset.x, 2) + pow(offset.y, 2));

	float x = atan(offset.z / temp) * (180.0f / M_PI);
	float y = atan(offset.y / offset.x) * (180.0f / M_PI);

	if (offset.x >= 0.0)
	{
		if (y < 0)
		{
			y += 180.0f;
		}
		else
		{
			y -= 180.0f;
		}
	}
	return vec3_t(x, y, 0);
}

vec3_t AngleToVector(vec3_t angle)
{
	float angle_xy = angle.y * M_PI / 180;
	float x = cos(angle_xy);
	float y = sin(angle_xy);
	float z = -tan(angle.x * M_PI / 180);

	return vec3_t(x, y, z);
}

bool world_to_screen(float* view_matrix, vec3_t world, vec2_t& screen, int resolutionWidth, int resolutionHeight)
{
	InsertJunkCodeRND();
	float* m_vMatrix = view_matrix;
	float w = m_vMatrix[12] * world.x + m_vMatrix[13] * world.y + m_vMatrix[14] * world.z + m_vMatrix[15];

	if (w < 0.01f)
		return false;

	screen.x = m_vMatrix[0] * world.x + m_vMatrix[1] * world.y + m_vMatrix[2] * world.z + m_vMatrix[3];
	screen.y = m_vMatrix[4] * world.x + m_vMatrix[5] * world.y + m_vMatrix[6] * world.z + m_vMatrix[7];

	float invw = 1.0f / w;
	screen.x *= invw;
	screen.y *= invw;

	float x = resolutionWidth / 2;      // 1920
	float y = resolutionHeight / 2;     // 1080

	x += 0.5 * screen.x * resolutionWidth + 0.5; // 1920
	y -= 0.5 * screen.y * resolutionHeight + 0.5; // 1080

	screen.x = x;
	screen.y = y;

	if (screen.x > resolutionWidth || screen.x < 0 || screen.y > resolutionHeight || screen.y < 0)   // 1920 1080
		return false;

	return true;
}

bool OptimalPitch(float x, float y, float v, float g, float* out_pitch)
{
	InsertJunkCodeRND();
	float root = v * v * v * v - g * (g * x * x + 2.0f * y * v * v);
	InsertJunkCodeRND();
	if (root >= 0.f)
	{
		*out_pitch = atan((v * v - sqrt(root)) / (g * x));
		return true;
	}
	return false;
}

bool ComputeTrajectory(vec3_t offset, float v, float g, float* travel_time, vec3_t* out_angle)
{
	InsertJunkCodeRND();
	float x = sqrt(offset.x * offset.x + offset.y * offset.y);
	float y = offset.z;
	float current_pitch = 0;
	InsertJunkCodeRND();
	if (!OptimalPitch(x, y, v, g, &current_pitch))
	{
		return false;
	}

	*travel_time = x / (cos(current_pitch) * v);
	*out_angle = vec3_t(current_pitch, atan2(y, x), 0);
	return true;
}

int TargetRush(vec3_t aim_dir, vec3_t target_v)
{
	aim_dir.z = 0;
	target_v.z = 0;
	float cos_angle_distance = aim_dir.normalized().dot(target_v.normalized());
	if (cos_angle_distance < -0.3f)
	{
		return -1;
	}
	else if (cos_angle_distance > 0.3f)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

float calc_apex_hipfire_hfov_16x9(float cl_fovscale)
{
	float fov43_deg = cl_fovscale * 70.0f;  // 步骤1
	float half_fov43_rad = fov43_deg * M_PI / 360.0f;
	float tan_half43 = tanf(half_fov43_rad);
	float aspect_ratio = (16.0f / 9.0f) / (4.0f / 3.0f);  // 1.333
	float half_hfov169_rad = atanf(tan_half43 * aspect_ratio);
	return 2.0f * half_hfov169_rad * 180.0f / M_PI;  // 步骤2
}

float calc_ads_hfov_16x9(float m_targetZoomFOV_deg, float cl_fovscale)
{
	float fov43_deg = m_targetZoomFOV_deg * cl_fovscale;  // 直接用！
	float half_fov43_rad = fov43_deg * M_PI / 360.0f;
	float tan_half43 = tanf(half_fov43_rad);
	float aspect_ratio = (16.0f / 9.0f) / (4.0f / 3.0f);  // 1.333
	float half_hfov169_rad = atanf(tan_half43 * aspect_ratio);
	return 2.0f * half_hfov169_rad * 180.0f / M_PI;
}

