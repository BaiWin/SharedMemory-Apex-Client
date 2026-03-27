#pragma once

namespace Offset {
	// local_player
	constexpr uintptr_t localPlayer = 0x2a7be28;  // 0x2a7be28  C
	constexpr uintptr_t cl_entitylist = 0x65fe6d8; // 0x6282c28   C
	//constexpr uintptr_t m_iname = 0x479;  // 0x0580
	//constexpr uintptr_t m_iHealth = 0x324;  
	constexpr uintptr_t m_iTeamNum = 0x0334;  // C
	constexpr uintptr_t m_lifeState = 0x690;   // >0 = dead   C
	constexpr uintptr_t m_bleedoutState = 0x2920;  // 0x2830   >0 = knocked down C
	constexpr uintptr_t viewAngle = 0x25f0;       // 0x25f0 - 0x14 // m_ammoPoolCapacity - 0x14 C
	constexpr uintptr_t m_vecAbsVelocity = 0x0170;  //
	constexpr uintptr_t m_vecAbsOrigin = 0x017c;          //
	constexpr uintptr_t m_latestPrimaryWeapons = 0x19b4; //0x19b4
	constexpr uintptr_t ViewRender = 0x40d8a38; // 0x3d3cd78   
	constexpr uintptr_t ViewMatrix = 0x11a350;  // 
	constexpr uintptr_t camera_origin = 0x1fb4; //
	constexpr uintptr_t m_flProjectileSpeed = 0x2818; //0x2810
	constexpr uintptr_t m_flProjectileScale = 0x2820;       // 0x2818
	constexpr uintptr_t m_nForceBone = 0xda8 + 0x48; //0x0da8 + 0x48
	constexpr uintptr_t m_targetZoomFOV = 0x1650 + 0xc4; // FOV with Scope C
	constexpr uintptr_t m_bZooming = 0x1ca1; // playing enable zoom
	constexpr uintptr_t m_weaponNameIndex = 0x1818;
	constexpr uintptr_t cl_fovScale = 0x2008900 + 0x58; // 0x1c81f10 + 0x58  // Player Setting FOV // base address + cl_fovScale + 0x58   C


	constexpr uintptr_t m_curZoomFOV = 0x1650 + 0xc0; // Wepaon Origin FOV // weapon entity + playerdata(0x1650) + m_curZoomFOV
	
	
}

//#pragma once
// 
//// Core
//constexpr uint64_t OFF_LEVEL = 0x1bd2b64;                         //[Miscellaneous]->LevelName
//constexpr uint64_t OFF_LOCAL_PLAYER = 0x26df2d8;                  //[Miscellaneous]->LocalPlayer
//constexpr uint64_t OFF_ENTITY_LIST = 0x6282c28;                   //[Miscellaneous]->cl_entitylist
// 
//constexpr uint64_t OFF_NAME_LIST = 0x8cbd680;                     //[Miscellaneous]->NameList
//constexpr uint64_t OFF_NAME_INDEX = 0x0580;                         //nameIndex
// 
//// HUD
//constexpr uint64_t OFF_VIEWRENDER = 0x3d3cd78;                    //[Miscellaneous]->ViewRender
//constexpr uint64_t OFF_VIEWMATRIX = 0x11a350;                     //[Miscellaneous]->ViewMatrix 
// 
//// Player
//constexpr uint64_t OFF_HEALTH = 0x324;                           //[RecvTable.DT_Player]->m_iHealth
//constexpr uint64_t OFF_MAXHEALTH = 0x468;                        //[RecvTable.DT_Player]->m_iMaxHealth
//constexpr uint64_t OFF_SHIELD = 0x1a0;                           //[RecvTable.DT_TitanSoul]->m_shieldHealth [RecvTable.DT_BaseCombatCharacter]
//constexpr uint64_t OFF_MAXSHIELD = 0x1a4;                        //[RecvTable.DT_TitanSoul]->m_shieldHealthMax [RecvTable.DT_BaseCombatCharacter]
// 
//constexpr uint64_t OFF_INATTACK = 0x03d3ce90;                     //[Buttons]->in_attack
// 
//constexpr uint64_t OFF_CAMERAORIGIN = 0x1fb4;                     //[Miscellaneous]->CPlayer!camera_origin
//constexpr uint64_t OFF_STUDIOHDR = 0xff0;                        //[Miscellaneous]->CBaseAnimating!m_pStudioHdr
//constexpr uint64_t OFF_BONES = 0x0da8 + 0x48;                     //m_nForceBone [RecvTable.DT_BaseAnimating]
// 
//constexpr uint64_t OFF_LOCAL_ORIGIN = 0x017c;                     //[DataMap.C_BaseEntity]->m_vecAbsOrigin
//constexpr uint64_t OFF_ABSVELOCITY = 0x0170;                      //[DataMap.C_BaseEntity]->m_vecAbsVelocity 
// 
//constexpr uint64_t OFF_ZOOMING = 0x1ca1;                          //[RecvTable.DT_Player]->m_bZooming 
//constexpr uint64_t OFF_TEAM_NUMBER = 0x0334;                      //[RecvTable.DT_BaseEntity]->m_iTeamNum
//constexpr uint64_t OFF_NAME = 0x0479;                             //[RecvTable.DT_BaseEntity]->m_iName
//constexpr uint64_t OFF_LIFE_STATE = 0x690;                       //[RecvTable.DT_Player]->m_lifeState
//constexpr uint64_t OFF_BLEEDOUT_STATE = 0x2830;                   //[RecvTable.DT_Player]->m_bleedoutState 
//constexpr uint64_t OFF_LAST_VISIBLE_TIME = 0x1a52 + 0x3;          //[RecvTable.DT_BaseCombatCharacter]->m_hudInfo_visibilityTestAlwaysPasses + 0x3 
//constexpr uint64_t OFF_LAST_AIMEDAT_TIME = 0x1a52 + 0x3 + 0x8;    //[RecvTable.DT_BaseCombatCharacter]->m_hudInfo_visibilityTestAlwaysPasses + 0x3 + 0x8 
//constexpr uint64_t OFF_VIEW_ANGLES = 0x2604 - 0x14;               //[DataMap.C_Player]-> m_ammoPoolCapacity - 0x14 
//constexpr uint64_t OFF_PUNCH_ANGLES = 0x2508;                     //[DataMap.C_Player]->m_currentFrameLocalPlayer.m_vecPunchWeapon_Angle
//constexpr uint64_t OFF_YAW = 0x230c - 0x8;                        //[DataMap.C_Player]->m_currentFramePlayer.m_ammoPoolCount - 0x8 
// 
//// Weapon 
//constexpr uint64_t OFF_WEAPON_HANDLE = 0x19b4;                    //[RecvTable.DT_Player]->m_latestPrimaryWeapons 
//constexpr uint64_t OFF_WEAPON_INDEX = 0x1818;                     //[RecvTable.DT_WeaponX]->m_weaponNameIndex 
//constexpr uint64_t OFF_PROJECTILESCALE = 0x0d80 + 0xd68;		  //CWeaponX!m_flProjectileScale or [WeaponSettings]->projectile_gravity_scale + [WeaponSettingsMeta]->base
//constexpr uint64_t OFF_PROJECTILESPEED = 0x0d78 + 0xd60;	      //CWeaponX!m_flProjectileSpeed or [WeaponSettings]->projectile_launch_speed + [WeaponSettingsMeta]->base
//constexpr uint64_t OFF_OFFHAND_WEAPON = 0x19c4;                   //[DT_BaseCombatCharacter]m_latestNonOffhandWeapons
//constexpr uint64_t OFF_CURRENTZOOMFOV = 0x1650 + 0x00bc;          //m_playerData + m_curZoomFOV
//constexpr uint64_t OFF_TARGETZOOMFOV = 0x1650 + 0x00c0;           //m_playerData + m_targetZoomFOV
//constexpr uint64_t OFF_WEAPON_AMMO = 0x1600;                      //RecvTable.DT_WeaponX_LocalWeaponData -> m_ammoInClip missing
//constexpr uint64_t OFF_RELOADING = 0x161a;						  //[RecvTable.DT_WeaponX_LocalWeaponData]-> m_bInReload
//constexpr uint64_t OFF_SIGNIFIER_NAME = 0x470;					  // [RecvTable.DT_BaseEntity].m_iSignifierName 
//constexpr uint64_t OFF_MODEL_NAME = 0x0030;							// [DataMap.C_BaseEntity].m_ModelName 
//constexpr uint64_t OFFSET_WEAPON_BITFIELD = 0x1798;					// [DataMap.CWeaponX]->m_modBitfieldInternal (Verified)
// 
//// Glow
//constexpr uint64_t OFF_GLOW_HIGHLIGHTS = 0x692f070;               //HighlightSettings
//constexpr uint64_t OFF_GLOW_ENABLE = 0x30c;                       //Script_Highlight_GetCurrentContext //not found
//constexpr uint64_t OFF_GLOW_THROUGH_WALL = 0x26C;                 //Script_Highlight_SetVisibilityType //not found
//constexpr uint64_t OFF_GLOW_FIX = 0x278;                          //not found
//constexpr uint64_t OFF_GLOW_HIGHLIGHT_ID = 0x298;                 //[DT_HighlightSettings].m_highlightServerActiveStates    //not found
//constexpr uint64_t OFF_GLOW_HIGHLIGHT_TYPE_SIZE = 0x34;           //not found
// 
//// Misc
//constexpr long OFF_TIME_BASE = 0x2158;                        //m_currentFramePlayer.timeBase
//constexpr long OFFSET_TRAVERSAL_START_TIME = 0x2c44;          //[RecvTable.DT_LocalPlayerExclusive]->m_traversalStartTime 
//constexpr long OFFSET_TRAVERSAL_PROGRESS = 0x2c3c;            //[RecvTable.DT_LocalPlayerExclusive]->m_traversalProgress

//local HELD_WEAPON_NAMES = {
//[114] = "Peacekeeper",
//[122] = "TripleTake",
//[102] = "Kraber",
//[192] = "Knife",
//[93] = "Havoc",
//[90] = "Devotion",
//[103] = "L-STAR",
//[125] = "Volt SMG",
//[129] = "Nemesis",
//[97] = "Flatline",
//[100] = "Hemlok",
//[112] = "Prowler",
//[126] = "30-30",
//[6] = "Rampage",
//[128] = "C.A.R",
//[119] = "P2020 Akimbo",
//[88] = "RE-45",
//[85] = "Alternator",
//[116] = "R-99",
//[0] = "R301",
//[120] = "Spitfire",
//[98] = "G7 Scout",
//[107] = "Mozambique",
//[95] = "EVA-8",
//[105] = "Mastiff",
//[123] = "Wingman",
//[92] = "Longbow",
//[89] = "Charge Rifle",
//[1] = "Sentinel",
//[2] = "BOCEK",
//[132] = "Fists",
//}