#include <windows.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include "overlay.h"

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

        // 画红线
        // 下面是绘制 Missile 的部分
        //std::lock_guard<std::mutex> lock(objArrayMutex);  // 锁住 vector 保护
        if (!listOfMissiles.empty())
        {
            for (const auto& missile : listOfMissiles)
            {
                Vector6 v6 = missile;  // 获取 Vector4，包含两个点的坐标
                Vector3 temp1{ v6.x,v6.y,v6.z };
                Vector3 temp2{ v6.x1,v6.y1,v6.z1 };
                Vector2 p1 = WorldToScreen(temp1);
                Vector2 p2 = WorldToScreen(temp2);
                ImVec2 point1 = ImVec2(p1.x / 1.5f, p1.y / 1.5f);  //  / DPI 1.5f
                ImVec2 point2 = ImVec2(p2.x / 1.5f, p2.y / 1.5f);  // (w, z) 为第二个点

                // 绘制这两个点之间的直线
                ImGui::GetBackgroundDrawList()->AddLine(point1, point2, IM_COL32(255, 0, 0, 255), 6.0f);  // 红色，线宽为2
            }
        }

        if (!listOfMinions.empty())
        {
            for (const auto& minion : listOfMinions)
            {
                Vector3 v3 = minion;
                Vector2 v2 = WorldToMinimap(minion);
                //std::cout << v2.x << "  " << v2.y << std::endl;
                // 在 V2 上绘制一个红色的点，半径为 4
                ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(v2.x / 1.5f, v2.y / 1.5f), 6.0f, IM_COL32(255, 0, 0, 255));  // 红色，半径为4
            }
        }

        //ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(2200 / 1.5f, 1300 / 1.5f), 40.0f, IM_COL32(255, 0, 0, 255));

        //ImGui::GetBackgroundDrawList()->AddLine(ImVec2(2000 / 1.5f, 2000/ 1.5f), ImVec2(1500 / 1.5f, 1500 / 1.5f), IM_COL32(255, 0, 0, 255), 2.0f);

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