#include <windows.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include "overlay.h"
#include <iostream>
#include <string>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRTV = nullptr;

static bool g_showMenu = false;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w, l)) return 0;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (msg == WM_SIZE && g_pd3dDevice)
    {
        if (g_mainRTV) { g_mainRTV->Release(); g_mainRTV = nullptr; }
        g_pSwapChain->ResizeBuffers(0, LOWORD(l), HIWORD(l), DXGI_FORMAT_UNKNOWN, 0);
        ID3D11Texture2D* backBuf;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuf));
        g_pd3dDevice->CreateRenderTargetView(backBuf, nullptr, &g_mainRTV);
        backBuf->Release();
    }
    return DefWindowProc(hwnd, msg, w, l);
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dContext)))
        return false;

    ID3D11Texture2D* backBuf;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuf));
    g_pd3dDevice->CreateRenderTargetView(backBuf, nullptr, &g_mainRTV);
    backBuf->Release();
    return true;
}

void CleanupDeviceD3D()
{
    if (g_mainRTV) g_mainRTV->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dContext) g_pd3dContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

// 专门的颜色lerp函数
ImU32 lerp_color(ImU32 color1, ImU32 color2, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);

    int r1 = (color1 >> IM_COL32_R_SHIFT) & 0xFF;
    int g1 = (color1 >> IM_COL32_G_SHIFT) & 0xFF;
    int b1 = (color1 >> IM_COL32_B_SHIFT) & 0xFF;
    int a1 = (color1 >> IM_COL32_A_SHIFT) & 0xFF;

    int r2 = (color2 >> IM_COL32_R_SHIFT) & 0xFF;
    int g2 = (color2 >> IM_COL32_G_SHIFT) & 0xFF;
    int b2 = (color2 >> IM_COL32_B_SHIFT) & 0xFF;
    int a2 = (color2 >> IM_COL32_A_SHIFT) & 0xFF;

    int r = static_cast<int>(std::lerp(static_cast<float>(r1), static_cast<float>(r2), t));
    int g = static_cast<int>(std::lerp(static_cast<float>(g1), static_cast<float>(g2), t));
    int b = static_cast<int>(std::lerp(static_cast<float>(b1), static_cast<float>(b2), t));
    int a = static_cast<int>(std::lerp(static_cast<float>(a1), static_cast<float>(a2), t));

    return IM_COL32(r, g, b, a);
}

void RenderOverlay()
{
    // 1. 入口不变，控制台保持可见
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // 2. 注册类
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"OverlayClass";
    RegisterClassExW(&wc);

    // 3. 创建透明窗口（保留控制台，去掉 NOACTIVATE）
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,  // 可穿透
        wc.lpszClassName, L"Overlay",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_COLORKEY);   // 背景全透

    // 4. 创建 D3D / ImGui
    if (!CreateDeviceD3D(hwnd))
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);

    // 字体
    ImFont* myFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

    // 5. 消息循环
    MSG msg;
    bool done = false;
    while (!done)
    {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // ImGui 新帧
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        //std::cout << "imgui------------" << entites_screen_pos.size() << std::endl;
        // 画红线
        // 下面是绘制 Missile 的部分
        std::lock_guard<std::mutex> lock(objArrayMutex);  // 锁住 vector 保护
        if (!new_stored_overlay_info.empty())
        {
            for (const auto& newInfo : new_stored_overlay_info)
            {
                int draw_x = newInfo.screenPos.x;
                int draw_y = newInfo.screenPos.y;
                if (!old_stored_overlay_info.empty())
                {
                    for (auto& oldInfo : old_stored_overlay_info)
                    {
                        if (newInfo.entity == oldInfo.entity)
                        {
                            oldInfo.screenPos.x = std::lerp(oldInfo.screenPos.x, newInfo.screenPos.x, 0.2f);
                            oldInfo.screenPos.y = std::lerp(oldInfo.screenPos.y, newInfo.screenPos.y, 0.2f);
                            draw_x = oldInfo.screenPos.x;
                            draw_y = oldInfo.screenPos.y;
                            break;
                        }
                    }
                }
                int dist = static_cast<int>(newInfo.dist);
                float t = 1 - dist / 300;
                t = std::clamp(t, 0.0f, 1.0f);
                float fontSize = std::lerp(18.0f, 30.0f, t);

                ImU32 start_color = IM_COL32(0, 255, 0, 255);   // 红色
                ImU32 end_color = IM_COL32(255, 0, 0, 255);     // 绿色
                ImU32 kill_color = IM_COL32(0, 0, 255, 255);     // 蓝色
                ImU32 gradient_color = lerp_color(start_color, end_color, t);

                if (newInfo.TargetToKill)
                {
                    gradient_color = kill_color;
                }

                //std::cout << "imgui------------" << vec.x << "," << vec.y << std::endl;
                std::string distance_str = std::to_string(dist) + "m";
                // 绘制这两个点之间的直线
                //ImGui::GetBackgroundDrawList()->AddText(ImVec2(vec.x / 1.5f, vec.y / 1.5f), IM_COL32(255, 0, 0, 255), distance_str.c_str(), nullptr);  // 红色，线宽为2
                ImGui::GetBackgroundDrawList()->AddText(myFont, fontSize, ImVec2(draw_x / 1.5f + 15, draw_y / 1.5f - 65), gradient_color, distance_str.c_str());
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(newInfo.screenPos.x / 1.5f, newInfo.screenPos.y / 1.5f), ImVec2(draw_x / 1.5f + 10, draw_y / 1.5f - 40), gradient_color, 1.0f);
            }
        }

        if (!predict_aim_screen_pos.IsEmpty())
        {
            float center_x = predict_aim_screen_pos.x / 1.5;
            float center_y = predict_aim_screen_pos.y / 1.5;
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(center_x + 20, center_y), ImVec2(center_x - 20, center_y), IM_COL32(255, 140, 0, 255), 2.5f);
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(center_x, center_y + 20), ImVec2(center_x, center_y - 20), IM_COL32(255, 140, 0, 255), 2.5f);
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(center_x, center_y), ImVec2(lock_target_screen_pos.x / 1.5f, lock_target_screen_pos.y / 1.5f), IM_COL32(255, 0, 0, 255), 1.0f);
        }

        if (lockRadius != 0)
        {
            ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(1280 / 1.5f, 800 / 1.5f), lockRadius, IM_COL32(255, 140, 0, 255), 30);
        }

        if(specialKeyTimer > 0)
        {
            std::string notice_str = specialKey ? "Special key[=] : On" : "Special key[=] : Off";
            specialKeyTimer -= 0.05f;
            if (specialKeyTimer < 0)
            {
                specialKeyTimer = 0;
            }
            ImGui::GetBackgroundDrawList()->AddText(myFont, 50, ImVec2(50 , 600), IM_COL32(0, 255, 0, 255), notice_str.c_str());
        }
        //if (!listOfMinions.empty())
        //{
        //    for (const auto& minion : listOfMinions)
        //    {
        //        Vector3 v3 = minion;
        //        //Vector2 v2 = WorldToMinimap(minion);
        //        //std::cout << v2.x << "  " << v2.y << std::endl;
        //        // 在 V2 上绘制一个红色的点，半径为 4
        //        //ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(v2.x / 1.5f, v2.y / 1.5f), 6.0f, IM_COL32(255, 0, 0, 255));  // 红色，半径为4
        //    }
        //}

        //ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(2200 / 1.5f, 1300 / 1.5f), 40.0f, IM_COL32(255, 0, 0, 255));

        //ImGui::GetBackgroundDrawList()->AddLine(ImVec2(2000 / 1.5f, 2000/ 1.5f), ImVec2(1500 / 1.5f, 1500 / 1.5f), IM_COL32(255, 0, 0, 255), 2.0f);
        //ImGui::GetBackgroundDrawList()->AddText(ImVec2(2000 / 1.5f, 2000 / 1.5f),IM_COL32(255, 0, 0, 255), "66666");
        // 画菜单
        if (GetAsyncKeyState(VK_INSERT) & 1) g_showMenu = !g_showMenu;
        if (g_showMenu)
        {
            ImGui::Begin("Menu");
            ImGui::Text("Hello Console + Overlay");
            ImGui::End();
        }

        ImGui::Render();
        const float clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
        g_pd3dContext->OMSetRenderTargets(1, &g_mainRTV, nullptr);
        g_pd3dContext->ClearRenderTargetView(g_mainRTV, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    CleanupDeviceD3D();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return;
}