#pragma once
#include <imgui.h>
#include <d3d11.h>
#include <memory>
#include <functional>
#include "TimeSeries.h"
#include <atlbase.h>
#include "D3DImgui.h"

class MainWindow
{
public:
	MainWindow(D3DImgui& imgui) :imgui_(imgui) { init(); }
	void run(const float ms);
private:
	void init();
	CComPtr<ID3D11ShaderResourceView> data_;
	bool import_ts_window = false;
	int tb_button_pressed = -1;
	float timestamp = 0.0f;
	AARC::TimeSeriesMgr mgr_;
	D3DImgui& imgui_;
};
