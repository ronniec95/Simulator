#pragma once
#include <Windows.h>
#include <functional>
#include <imgui.h>
#include <d3d11.h>
#include <doctest\doctest.h>
#include <atlbase.h>
#include "imgui_impl_dx11.h"

class D3DImgui
{
public:
	D3DImgui(const std::string& name) : name_(name) { init(); }
	~D3DImgui() { cleanup(); }
	void run(const std::function<void(const float)>& f) {
		static ImVec4 clear_col = ImColor(114, 144, 154);
		// Main loop
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (msg.message != WM_QUIT) {
			if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			// Rendering
			::ImGui_ImplDX11_NewFrame();
			f(0.0);
			d3d_device_context_->ClearRenderTargetView(main_render_target_view_, (float*)&clear_col);
			ImGui::Render();
			swap_chain_->Present(0, 0);
			// Wait 32ms, 30fps is fine for a GUI app, not a game
			Sleep(2);
		}
	}
	CComPtr<ID3D11DeviceContext>     device_context() const {return d3d_device_context_;}
	CComPtr<ID3D11Device>            device() const { return d3d_device_; }
	CComPtr<IDXGISwapChain>          swap_chain() const { return swap_chain_;	}

	CComPtr<ID3D11ShaderResourceView> CreateShaderResource(const float x, const float y, const int comp, const void* data);


	void CreateRenderTarget();
	void CleanupRenderTarget();
protected:
	void init();
	void cleanup();
private:
	CComPtr<ID3D11DeviceContext>     d3d_device_context_;
	CComPtr<ID3D11Device>            d3d_device_;
	CComPtr<IDXGISwapChain>          swap_chain_;
	CComPtr<ID3D11RenderTargetView>  main_render_target_view_;

	HRESULT CreateDeviceD3D();

	const std::string&       name_;
	WNDCLASSEX               wc_;
	HWND                     hwnd_;
};

//ID3D11ShaderResourceView* CreateShaderResource(const float x, const float y, const int comp, const void* data);
