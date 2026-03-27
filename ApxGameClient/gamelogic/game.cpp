#include "game.h"
#include <iostream>
#include <vectors.h>
#include "arduino.h"

#define PI 2 * acos(0.0)
#define smoothness 12.0f

uintptr_t base_address = 0;

std::vector<EntityOverlayInfo> new_stored_overlay_info;
std::vector<EntityOverlayInfo> old_stored_overlay_info;
vec2_t predict_aim_screen_pos;
vec2_t lock_target_screen_pos;

std::vector<DistanceAnglePair> distanceAngleConfig =
{
	{0, 10, 45},
	{10, 20, 40}, // 90DU // 40du 0.23F
	{20, 200, 4}, // 50DU
	{200, 1000, 2}, // 12du
};

bool ReadLocalPlayer(LocalPlayer* localPlayer)
{
	MemoryResult<uintptr_t> local_player = ReadAbsolute<uintptr_t>(base_address, Offset::localPlayer);
	if (local_player.Value > 0x4000000000000000)
	{
		std::cout << "local_player-------" << std::hex << local_player.Value << std::endl;
	}
	MemoryResult<int> m_iTeamNum = Read<int>(local_player, Offset::m_iTeamNum);
	MemoryResult<int> m_lifeState = Read<int>(local_player, Offset::m_lifeState);
	MemoryResult<int> m_bleedoutState = Read<int>(local_player, Offset::m_bleedoutState);
	MemoryResult<vec3_t> local_origin = Read<vec3_t>(local_player, Offset::m_vecAbsOrigin);
	MemoryResult<vec3_t> view_angle = Read<vec3_t>(local_player, Offset::viewAngle);

	//std::cout << "local_player-------" << std::hex << local_player.Value << std::endl;
	//std::cout << "m_iTeamNum-------" << std::dec << m_iTeamNum.Value << std::endl;
	//std::cout << "m_lifeState-------" << std::dec << m_lifeState.Value << std::endl;    // = 0 活着 = 1 死
	//std::cout << "m_bleedoutState-------" << std::dec << m_lifeState.Value << std::endl;
	//std::cout << "view_angle---------" << std::dec << view_angle.Value.x << "," << view_angle.Value.y << "," << view_angle.Value.z << std::endl;  // x-垂直方向-(90~-90) y-水平方向-(180~-180)
	localPlayer->address = local_player.Value;
	localPlayer->m_iTeamNum = m_iTeamNum.Value;
	localPlayer->m_lifeState = m_lifeState.Value;
	localPlayer->m_bleedoutState = m_bleedoutState.Value;
	localPlayer->local_origin = local_origin.Value;
	localPlayer->view_angle = view_angle.Value;

	MemoryResult<uintptr_t> view_renderer = ReadAbsolute<uintptr_t>(base_address, Offset::ViewRender);
	if (!view_renderer.Value)
		return false;

	MemoryResult<uintptr_t> view_matrix_ = Read<uintptr_t>(view_renderer, Offset::ViewMatrix);
	if (!view_matrix_.Value)
		return false;
	MemoryResult<view_matrix_t> view_matrix = Read<view_matrix_t>(view_matrix_, 0x0);

	localPlayer->view_matrix_ = view_matrix.Value;

	/*std::cout << "view_matrix_---------";
	for (int i = 0; i < 16; i++)
	{
		std::cout << std::dec << localPlayer->view_matrix_.matrix[i] << ",";
	}
	std::cout << "----end---------" << std::endl;*/

	return true;
	
}

void TestFOV(LocalPlayer* localPlayer)
{
	uintptr_t latest_primary_weapons = ReadAbsolute<uintptr_t>(localPlayer->address, Offset::m_latestPrimaryWeapons).Value;   //m_latestPrimaryWeapons
	//std::cout << latest_primary_weapons << std::endl;
	latest_primary_weapons &= 0xffff;
	uintptr_t weapon_entity = ReadAbsolute<uintptr_t>(base_address + Offset::cl_entitylist, latest_primary_weapons << 5).Value;
	//std::cout << weapon_entity << std::endl;
	float m_targetZoomFOV = ReadAbsolute<float>(weapon_entity, Offset::m_targetZoomFOV).Value;
	float m_curZoomFOV = ReadAbsolute<float>(weapon_entity, Offset::m_curZoomFOV).Value;
	float cl_fovScale = ReadAbsolute<float>(base_address, Offset::cl_fovScale).Value;
	int weaponindex = ReadAbsolute<int>(weapon_entity, Offset::m_weaponNameIndex).Value;
	std::cout << "--- " << m_curZoomFOV << std::endl;
	std::cout << weaponindex << std::endl;
	uint16_t zoom = ReadAbsolute<uint16_t>(localPlayer->address, Offset::m_bZooming).Value;
	std::cout << m_targetZoomFOV << std::endl;
	std::cout << "cl_fovScale : " << cl_fovScale << std::endl;

	float projectile_speed = ReadAbsolute<float>(weapon_entity, Offset::m_flProjectileSpeed).Value;   //CWeaponX!m_flProjectileSpeed
	std::cout << projectile_speed << std::endl;
}

void TestSensitivy(LocalPlayer* localPlayer)
{
	static float a = 1;
	static float y = 0;
	int b = 100;
	a += 2;  // 20 - 20/17(1.176) -17  // 5 - 5/11(0.455) -11 // 1- 1/9(0.111) -9
	if (a > 90)
	{
		getchar();
	}
	int sent = (int)b;
	float differ = localPlayer->view_angle.y - y;
	Sleep(100);
	SendFixMouseMovement(serial, sent, 0 , 0);// -9.09091  // -9.09092  //-12.0945   // -22.1572 // // -34.3372 // -46.3013 // -70.0101 // -93.6087 // -117.162
	
	y = localPlayer->view_angle.y;
	if (differ != 0)
	{
		std::cout << "sent--differ--k" << std::dec << sent << "," << differ << "," << sent / differ << std::endl;
	}
}

bool specialKey = false;
float specialKeyTimer = 0;
uintptr_t currentLock = 0;
uintptr_t targetToKill = 0;
void GameMain(ULONG pid)
{
	base_address = GetModuleBase(pid).Value;
	//std::cout << std::hex << base_address << std::endl;
	if (!base_address) return;

	LocalPlayer local_player;

	if (!ReadLocalPlayer(&local_player)) return;

	//TestFOV(&local_player);
	//TestSensitivy(&local_player);
	//FillChunkToSize(40);
	//WriteAbsolute<float>(local_player.address, Offset::viewAngle, 0);   // y direction

	if (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000)
	{
		specialKey = !specialKey;
		specialKeyTimer = 2.0f;
	}

	if (!SearchForEntities(&local_player) && currentLock == 0) return; // 当前有锁定，将继续

	//
	if (GetAsyncKeyState(VK_LMENU) & 0x8000) // press key
	{
		//SendFixMouseMovement(serial, 100, 100, 0);
		if (currentLock == 0)           // if not target then find a target
		{
			uintptr_t lockedEntity = LockTarget(stored_entities_read_data);
			currentLock = lockedEntity;
		}

		if (currentLock != 0 && (GetAsyncKeyState('X') & 0x8000))
		{
			targetToKill = currentLock;
		}
	}
	else if (GetAsyncKeyState('X') & 0x8000)
	{
		targetToKill = 0;
	}
	else       // if dont press key, then clear target
	{
		currentLock = 0;
	}
	
	if (currentLock)
	{
		PerformLock(&local_player, currentLock);
	}
	else
	{
		predict_aim_screen_pos = vec2_t(0, 0);
		lockRadius = 0;
	}
}

std::vector<LockPlayer> stored_entities_read_data;
bool SearchForEntities(LocalPlayer* localPlayer)
{
	old_stored_overlay_info = new_stored_overlay_info;
	stored_entities_read_data.clear();
	new_stored_overlay_info.clear();
	uintptr_t entity_list = base_address + Offset::cl_entitylist;
	for (int i = 0; i < 100; i++) // 100
	{
		MemoryResult<uintptr_t> entity = ReadAbsolute<uintptr_t>(entity_list, ((uintptr_t)i << 5));
		//uintptr_t m_iname = Read<uintptr_t>(entity, Offset::m_iname).Value; // m_iname != 125780153691248
		int m_iTeamNum = Read<int>(entity, Offset::m_iTeamNum).Value;
		int m_lifeState = Read<int>(entity, Offset::m_lifeState).Value;
		int m_bleedoutState = Read<int>(entity, Offset::m_bleedoutState).Value;
		vec3_t entity_origin = Read<vec3_t>(entity, Offset::m_vecAbsOrigin).Value;
		//std::cout << std::hex << entity.Value << "," << m_iname << "," << m_iTeamNum << "," << m_lifeState << "," << m_bleedoutState << std::endl;
		if (entity.Value == 0)
			continue;
		if (entity.Value == localPlayer->address)
			continue;
		if (entity_origin.IsEmpty())
			continue;
		//if (m_iname != 125780153691248)       //m_iname m_iname != 125780153691248
		//	continue;
		if (m_iTeamNum == localPlayer->m_iTeamNum)   //m_iTeamNum  (TeamID % 2 == 0 other mode)
			continue;
		if ((m_lifeState != 0 || m_bleedoutState > 0) && entity.Value != targetToKill)   //m_lifeState, >0 = dead , m_bleedoutState, >0 = knocked
			continue;

		float dist = entity_origin.distance_to(localPlayer->local_origin) * 0.01905f;

		vec2_t entity_pos_2d{};
		if (!world_to_screen(localPlayer->view_matrix_.matrix, entity_origin, entity_pos_2d, 2560, 1600)) continue;

		EntityOverlayInfo info;
		info.entity = entity.Value;
		info.screenPos = entity_pos_2d;
		info.dist = dist;
		info.TargetToKill = (entity.Value == targetToKill) ? true : false;

		new_stored_overlay_info.push_back(info);

		/*float dist = entity_origin.distance_to(localPlayer->local_origin);

		if (dist * 0.01905f > 400)
			continue;*/

		if (localPlayer->m_lifeState != 0 || localPlayer->m_bleedoutState > 0) return false;
		vec3_t aim_angle = VectorToAngle(localPlayer->local_origin, entity_origin);
		vec3_t aim_dir = entity_origin - localPlayer->local_origin;
		vec3_t view_dir = AngleToVector(localPlayer->view_angle);
		float cos_angle_distance = aim_dir.normalized().dot(view_dir.normalized()); //cos a  // -1 ~ 1
		//float angle_min = abs(cos_angle_distance - 1);
		if (cos_angle_distance > cos(90 * M_PI / 180))   //90
		{
			LockPlayer data;
			data.address = entity.Value;
			data.angleToLocalPlayer = cos_angle_distance;
			data.dist = dist;
			stored_entities_read_data.push_back(data);
		}
	}
	if (stored_entities_read_data.size() == 0)
	{
		return false;
	}
	return true;
}

vec3_t get_bone_pos_new(uintptr_t entity, vec3_t origin_pos, int id)
{
	MemoryResult<uintptr_t> bones_array = ReadAbsolute<uintptr_t>(entity, Offset::m_nForceBone);   //m_nForceBone + 0x48
	vec3_t bone_pos;

	uintptr_t boneloc = (id * 0x30);
	Bone bo = {};
	bo = Read<Bone>(bones_array, boneloc).Value;
	bone_pos.x = bo.x + origin_pos.x;
	bone_pos.y = bo.y + origin_pos.y;
	bone_pos.z = bo.z + origin_pos.z;

	if (bo.z < 1)
	{
		if (id == 2) bo.z = 40;
		else if (id == 8) bo.z = 60;

		bone_pos.z = bo.z + origin_pos.z;
	}
	
	return bone_pos;
}

float LockMaxAngle = 0;
float lockRadius = 0;
uintptr_t LockTarget(std::vector<LockPlayer> datas)
{
	if (datas.empty()) return 0;

	for (const auto& config : distanceAngleConfig)
	{
		float tempIndex = -1;
		float tempCosAngle = 0; // cos 90 = 0
		float configAngle = 0;
		for (int i = 0; i < datas.size(); i++)
		{
			const auto& entity = datas[i];
			float distance = entity.dist;
			float cos_angle = entity.angleToLocalPlayer;
			if (distance >= config.minDistance && distance < config.maxDistance && cos_angle > cos(config.angle * M_PI / 180))
			{
				if (cos_angle > tempCosAngle)
				{
					tempCosAngle = cos_angle;
					tempIndex = i;

					configAngle = config.angle;
					//std::cout << distance << std::endl;
				}
			}
		}
		if (tempIndex != -1)
		{
			LockMaxAngle = configAngle;
			return datas[tempIndex].address;
		}
	}
	return 0;
}


vec3_t velocity = vec3_t(0, 0, 0);
float smoothTime = 0.001f;
vec2_t velocity_v2 = vec2_t(0, 0);
float smoothTime_v2 = 0.001f;
void PerformLock(LocalPlayer *localplayer, uintptr_t lockedEntity)
{
	int m_lifeState = ReadAbsolute<int>(lockedEntity, Offset::m_lifeState).Value;
	int m_bleedoutState = ReadAbsolute<int>(lockedEntity, Offset::m_bleedoutState).Value;

	if ((m_lifeState != 0 || m_bleedoutState > 0) && currentLock != targetToKill) // if target die, remove target wait for reselect
	{
		currentLock = 0;
		//adaptiveTotalMoveController.ResetControllerParam(); // reset sensitivty that updated in lock
		predict_aim_screen_pos = vec2_t(0, 0);
		lockRadius = 0;
		return;
	}

	uintptr_t latest_primary_weapons = ReadAbsolute<uintptr_t>(localplayer->address, Offset::m_latestPrimaryWeapons).Value;   //m_latestPrimaryWeapons
	//std::cout << latest_primary_weapons << std::endl;
	latest_primary_weapons &= 0xffff;
	uintptr_t weapon_entity = ReadAbsolute<uintptr_t>(base_address + Offset::cl_entitylist, latest_primary_weapons << 5).Value; // 这个latest_primary_weapons上一帧不对
	//std::cout << weapon_entity << std::endl;
	int weaponindex = ReadAbsolute<int>(weapon_entity, Offset::m_weaponNameIndex).Value;
	//std::cout << "weapon index" << weapon_entity << std::endl;
	float projectile_speed = ReadAbsolute<float>(weapon_entity, Offset::m_flProjectileSpeed).Value;   //CWeaponX!m_flProjectileSpeed
	//std::cout << projectile_speed << std::endl;

	if (weaponindex == 2 && specialKey == false) projectile_speed = 28000.0f;

	

	vec3_t locked_target_origin = ReadAbsolute<vec3_t>(lockedEntity, Offset::m_vecAbsOrigin).Value;
	vec3_t head_pos = get_bone_pos_new(lockedEntity, locked_target_origin, 8);
	vec3_t camera_origin = ReadAbsolute<vec3_t>(localplayer->address, Offset::camera_origin).Value;
	//aim angle1
	vec3_t aim_angle = VectorToAngle(camera_origin, head_pos);

	vec3_t view_angle = localplayer->view_angle;
	vec3_t sway_angle = ReadAbsolute<vec3_t>(localplayer->address, Offset::viewAngle - 0x10).Value;   //OFFSET_VIEWANGLES - 0x10 (m_ammoPoolCapacity - 0x14 = OFFSET_VIEWANGLES)
	//sway_angle.z = 0;
	//std::cout << "sway_angle: " << sway_angle.x << "," << sway_angle.y << "," << sway_angle.z << std::endl;
	//aim angle2

	//-----------
	float projectile_scale = ReadAbsolute<float>(weapon_entity, Offset::m_flProjectileScale).Value;   //CWeaponX!m_flProjectileScale
	float projectile_gravity = 750.0f * projectile_scale;
	//projectile_speed = projectile_speed - (projectile_speed * 0.08);
	//projectile_gravity = projectile_gravity + (projectile_gravity * 0.05);
	vec3_t abs_velocity = ReadAbsolute<vec3_t>(lockedEntity, Offset::m_vecAbsVelocity).Value;   //m_vecAbsVelocity
	
	//--------------------
	vec3_t body_pos = get_bone_pos_new(lockedEntity, locked_target_origin, 2);
	//--------------------
	float weapon_FOV = ReadAbsolute<float>(weapon_entity, Offset::m_targetZoomFOV).Value;
	uint16_t zoom = ReadAbsolute<uint16_t>(localplayer->address, Offset::m_bZooming).Value;
	float fovScale = ReadAbsolute<float>(base_address, Offset::cl_fovScale).Value;
	float player_FOV = 110;
	float coefficient = 1.0f;
	if (weapon_FOV > 0 && weapon_FOV < 180 && player_FOV > 0 && player_FOV < 180 && zoom == 1 && fovScale != 0)
	{
		float hfov = calc_apex_hipfire_hfov_16x9(fovScale);
		float ads_hfov = calc_ads_hfov_16x9(weapon_FOV, fovScale);


		coefficient = tan(hfov / 2 * M_PI / 180) / tan(ads_hfov / 2 * M_PI / 180);
		if (coefficient > 100 || coefficient <= 0) coefficient = 1;
		if (weapon_FOV > 110 || weapon_FOV < 1) coefficient = 1;
		//std::cout << std::dec << player_FOV << ", " << weapon_FOV << "," << fovScale << ", " << coefficient << std::endl;
	}
	//--------
	//vec3_t localplayer_velocity = ReadAbsolute<vec3_t>(localplayer->address, Offset::m_vecAbsVelocity).Value;

	uintptr_t test_Value = ReadAbsolute<uintptr_t>(base_address, Offset::localPlayer).Value;

	if (projectile_speed <= 1) return;

	if (camera_origin.IsEmpty() || head_pos.IsEmpty()) return;

	if (test_Value != localplayer->address) return;

	if (latest_primary_weapons == 0) return;

	body_pos.z += 2.0f;
	if (weaponindex == 123)
	{
		head_pos.z -= 4.0f;
	}
	else if (weaponindex == 114 || weaponindex == 95 || weaponindex == 105 || weaponindex == 107)
	{
		head_pos = body_pos;
	}

	vec3_t out_angle;
	if (camera_origin.distance_to(head_pos) * 0.01905f > 30)
	{
		//-------------temp----
		head_pos.z -= 3.0f;

		if (head_pos.z - camera_origin.z > 300)
		{
			head_pos.z += 2.0f;
		}
		else if (head_pos.z - camera_origin.z < -600 && TargetRush(locked_target_origin - camera_origin, abs_velocity) == -1)
		{
			head_pos = body_pos;
		}

		//-------------
		//std::cout << latest_primary_weapons << std::endl;
		//std::cout << "test" << std::endl;
		if (calculate_predict_angle(camera_origin, head_pos, abs_velocity, projectile_speed, projectile_gravity, &out_angle))
		//if (calculate_predict_angleEx(camera_origin, head_pos, localplayer_velocity, abs_velocity, projectile_speed, projectile_gravity, &out_angle))
		{
			aim_angle = out_angle;
		}
		else
		{
			//std::cout << " predict false" << std::endl;          //!!!
		}
	}
	else if(camera_origin.distance_to(head_pos) * 0.01905f < 10)
	{
		head_pos.z -= 3.0f;
		if (calculate_predict_angle_directEx(camera_origin, head_pos, abs_velocity, projectile_speed, projectile_gravity, &out_angle))
		{
			aim_angle = out_angle;
		}
		coefficient *= 1.2f;
	}
	else
	{
		head_pos = body_pos;
		if (calculate_predict_angle_direct(camera_origin, head_pos, abs_velocity, projectile_speed, projectile_gravity, &out_angle))
		{
			aim_angle = out_angle;
		}
		else
		{
			//std::cout << " predict false" << std::endl;          //!!!
		}

		//head_pos.z -= 5.0f;
		//aim_angle = VectorToAngle(camera_origin, head_pos);
	}
	

	//Add
	bool no_recoil = true;
	if (no_recoil)
		aim_angle -= sway_angle - view_angle;

	aim_angle.x = std::clamp(aim_angle.x, -89.0f, 89.0f);
	aim_angle.y = std::clamp(aim_angle.y, -179.0f, 179.0f);
	//vec3_t smooth_angle = view_angle.SmoothDamp(view_angle, aim_angle, velocity, smoothTime, 0.001f);
	//WriteAbsolute(localplayer->address, Offset::viewAngle, smooth_angle);

	if (test_Value == localplayer->address)
	{
		vec3_t aimDir = AngleToVector(aim_angle);
		vec2_t new_predict_pos_2d{};
		if (world_to_screen(localplayer->view_matrix_.matrix, aimDir * 1000 + camera_origin, new_predict_pos_2d, 2560, 1600) &&
			world_to_screen(localplayer->view_matrix_.matrix, head_pos, lock_target_screen_pos, 2560, 1600))
		{
			if (!predict_aim_screen_pos.IsEmpty())
			{
				predict_aim_screen_pos = predict_aim_screen_pos.SmoothDamp(predict_aim_screen_pos, new_predict_pos_2d, velocity_v2, smoothTime_v2, 0.001f);
			}
			else
			{
				predict_aim_screen_pos = new_predict_pos_2d;
			}
		}


		lockRadius = tan(LockMaxAngle * M_PI / 180) * 1280 / tan(player_FOV / 2 * M_PI / 180);

		/*vec2_t lockCircle_pos{};
		vec3_t lockCircleDir = AngleToVector(vec3_t(LockMaxAngle, 0, 0) + view_angle);
		if (world_to_screen(localplayer->view_matrix_.matrix, lockCircleDir * 1000 + camera_origin, lockCircle_pos, 2560, 1600))
		{
			lockRadius = abs(lockCircle_pos.x - 1280);
		}*/
		

		//WriteAbsolute(localplayer->address, Offset::viewAngle, aim_angle);
		//std::cout << view_angle.x << "," << view_angle.y << "-" << aim_angle.x << "," << aim_angle.y << std::endl;

		// Arduino
		adaptiveTotalMoveController.SendArduinoMovmentRaw(aim_angle, view_angle, camera_origin.distance_to(head_pos) * 0.01905f, coefficient, view_angle - sway_angle);
		//arduino_move_info.push_back(vec3_t{ roundToBoundary(base_arduino_x), roundToBoundary(base_arduino_y) ,0 });
		//std::cout << " Write called here" << std::endl;
	}
}

std::mutex objArrayMutex;