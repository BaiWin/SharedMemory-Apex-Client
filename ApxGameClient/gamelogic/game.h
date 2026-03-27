#pragma once
#include "ClientIncludes.h"
#include "Offset.h"
#include "overlay.h"
#include <vector>
#include <mutex>
#include "utils.h"
#include <vectors.h>


struct view_matrix_t
{
	float matrix[16];
};

struct LocalPlayer
{
	uintptr_t address;
	int m_iTeamNum;
	int m_lifeState; // >0 = dead
	int m_bleedoutState; // >0 = knocked
	vec3_t local_origin;
	vec3_t view_angle;
	view_matrix_t view_matrix_;

	//vec3_t camera_origin;
	//uintptr_t latest_primary_weapons;
};

struct LockPlayer
{
	uintptr_t address;

	float angleToLocalPlayer;
	float dist;
	//vec3_t head_pos;
	//vec3_t m_vecAbsVelocity;
};

typedef struct Bone
{
	uint8_t pad1[0xCC];
	float x;
	uint8_t pad2[0xC];
	float y;
	uint8_t pad3[0xC];
	float z;
}Bone;

struct DistanceAnglePair
{
	float minDistance;
	float maxDistance;
	float angle;
};

struct EntityOverlayInfo
{
	uintptr_t entity;
	vec2_t screenPos;
	float dist;
	bool TargetToKill;
};

void GameMain(ULONG pid);

bool SearchForEntities(LocalPlayer* localPlayer);

void PerformLock(LocalPlayer* localplayer, uintptr_t lockedEntity);

bool SearchForEntities(LocalPlayer* localPlayer);

bool ReadLocalPlayer(LocalPlayer* localPlayer);

uintptr_t LockTarget(std::vector<LockPlayer> datas);

extern std::vector<LockPlayer> stored_entities_read_data;

extern std::vector<EntityOverlayInfo> old_stored_overlay_info;

extern std::vector<EntityOverlayInfo> new_stored_overlay_info;

extern vec2_t predict_aim_screen_pos;

extern vec2_t lock_target_screen_pos;

extern float lockRadius;

extern bool specialKey;

extern float specialKeyTimer;

extern std::mutex objArrayMutex;