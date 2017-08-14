#include <d3d11.h>
#include <imgui.h>
#define DIRECTINPUT_VERSION 0x0800
#include "../Utilities.h"
#include "D3DImgui.h"
#include "imgui_impl_dx11.h"
#include <comdef.h>
#include <d2d1_2.h>
#include <dinput.h>
#include <spdlog\spdlog.h>
#include <tchar.h>
// Data
static IDXGIDevice *  g_dxgiDevice = NULL;
static ID2D1Factory2 *g_d2dFactory = NULL;
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3dcompiler.lib")

void D3DImgui::CreateRenderTarget() {
    MethodLogger         mlog("::CreateRenderTarget");
    DXGI_SWAP_CHAIN_DESC sd;
    swap_chain_->GetDesc(&sd);
    // Create the render target
    CComPtr<ID3D11Texture2D>      pBackBuffer;
    D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
    ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
    render_target_view_desc.Format        = sd.BufferDesc.Format;
    render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBackBuffer);
    d3d_device_->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &main_render_target_view_);
    d3d_device_context_->OMSetRenderTargets(1, &main_render_target_view_, NULL);
}

void D3DImgui::CleanupRenderTarget() { main_render_target_view_.Release(); }

HRESULT D3DImgui::CreateDeviceD3D() {
    // Setup swap chain
    MethodLogger         mlog("::CreateDeviceD3D");
    DXGI_SWAP_CHAIN_DESC sd;
    {
        SPDLOG_TRACE(spdlog::get("logger"), "Creating swap chain");
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount                        = 2;
        sd.BufferDesc.Width                   = 0;
        sd.BufferDesc.Height                  = 0;
        sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator   = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow                       = hwnd_;
        sd.SampleDesc.Count                   = 1;
        sd.SampleDesc.Quality                 = 0;
        sd.Windowed                           = TRUE;
        sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
    }

    UINT createDeviceFlags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL       featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = {
        D3D_FEATURE_LEVEL_11_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1,
                                      D3D11_SDK_VERSION, &sd, &swap_chain_, &d3d_device_, &featureLevel,
                                      &d3d_device_context_) != S_OK) {
        return E_FAIL;
    }
    SPDLOG_TRACE(spdlog::get("logger"), "D3D11CreateDeviceAndSwapChain created");
    CreateRenderTarget();
    return S_OK;
}

static void check_com_error(HRESULT hr) {
    if (hr != S_OK) {
        _com_error err(hr);
        spdlog::get("logger")->error("{}", ws2s(err.ErrorMessage()));
    }
}

CComPtr<ID3D11ShaderResourceView> D3DImgui::CreateShaderResource(const float x, const float y, const int comp,
                                                                 const void *data) {
    MethodLogger mlog("::CreateShaderResource");
    mlog.logger()->debug("Creating texture description x:{} y:{} comp:{}", x, y, comp);
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.ArraySize          = 1;
    texDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags     = 0;
    texDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.Height             = x;
    texDesc.Width              = y;
    texDesc.MipLevels          = 1;
    texDesc.MiscFlags          = 0;
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage              = D3D11_USAGE_DEFAULT;
    CComPtr<ID3D11Texture2D> texture;
    D3D11_SUBRESOURCE_DATA   subresources;
    subresources.pSysMem     = data;
    subresources.SysMemPitch = x * comp;
    check_com_error(d3d_device_->CreateTexture2D(&texDesc, &subresources, &texture));
    D3D11_SHADER_RESOURCE_VIEW_DESC svDesc;
    svDesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
    svDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    svDesc.Texture2D     = D3D11_TEX2D_SRV{0, (UINT)-1};
    CComPtr<ID3D11ShaderResourceView> shaderResourceView;
    check_com_error(d3d_device_->CreateShaderResourceView(texture, &svDesc, &shaderResourceView));
    return shaderResourceView;
    // float dpiX, dpiY;
    // g_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
    // IDXGISurface *backBuffer = NULL;
    // HRESULT hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    ////	ID3D11Texture2D* pOffscreenTexture = NULL;
    ////	HRESULT hr = g_pd3dDevice->CreateTexture2D(&texDesc, NULL, &pOffscreenTexture);
    ////	CHECK(hr == S_OK);
    //	//IDXGISurface *pdxgiSurface = NULL;
    //	//hr = pOffscreenTexture->QueryInterface(&pdxgiSurface);
    //	//CHECK(hr == S_OK);
    // ID2D1RenderTarget* d2dRenderTarget = NULL;
    // D2D1_RENDER_TARGET_PROPERTIES props =
    //	D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN,
    // D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);  hr = g_d2dFactory->CreateDxgiSurfaceRenderTarget(backBuffer,
    // &props,
    // &d2dRenderTarget);

    // check_com_error(hr);
    // CHECK(hr == S_OK);
    // return d2dRenderTarget;
}
extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam)) { return true; }
    D3DImgui *pThis = reinterpret_cast<D3DImgui *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (msg) {
    case WM_NCCREATE: {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis               = static_cast<D3DImgui *>(lpcs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } break;
    case WM_SIZE:
        if (pThis->device() != NULL && wParam != SIZE_MINIMIZED) {
            spdlog::get("logger")->debug("WM_SIZE");
            ImGui_ImplDX11_InvalidateDeviceObjects();
            pThis->CleanupRenderTarget();
            pThis->swap_chain()->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            pThis->CreateRenderTarget();
            ImGui_ImplDX11_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
        {
            spdlog::get("logger")->debug("WM_SYSCOMMAND");
            return 0;
        }
        break;
    case WM_DESTROY:
        spdlog::get("logger")->debug("WM_DESTROY");
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void D3DImgui::init() {
    MethodLogger mlog("::D3DImgui_init");
    wc_ = {sizeof(WNDCLASSEX),
           CS_CLASSDC,
           WndProc,
           0L,
           0L,
           GetModuleHandle(NULL),
           NULL,
           LoadCursor(NULL, IDC_ARROW),
           NULL,
           NULL,
           _T("ImGui Example"),
           NULL};
    mlog.logger()->debug("Registering window class");
    RegisterClassEx(&wc_);
    mlog.logger()->debug("Creating window");
    hwnd_ = CreateWindow(_T("ImGui Example"), s2ws(name_).data(), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL,
                         wc_.hInstance, this);
    // Initialize Direct3D
    if (CreateDeviceD3D() < 0) {
        mlog.logger()->debug("Unregistering class");
        UnregisterClass(_T("ImGui Example"), wc_.hInstance);
        return;
    }
    // Show the window
    mlog.logger()->debug("Showing window");
    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    mlog.logger()->debug("Update window");
    UpdateWindow(hwnd_);
    // Setup ImGui binding
    mlog.logger()->debug("Imgui Init window");
    ImGui_ImplDX11_Init(hwnd_, d3d_device_, d3d_device_context_);
}

void D3DImgui::cleanup() {
    MethodLogger mlog("::D3DImgui_cleanup");
    ImGui_ImplDX11_Shutdown();
    UnregisterClass(_T("ImGui Example"), wc_.hInstance);
}