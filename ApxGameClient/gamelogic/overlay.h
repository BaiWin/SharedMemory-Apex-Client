#pragma once
#include <windows.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "game.h"
#include "Vector.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void RenderOverlay();