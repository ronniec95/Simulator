#include "MainWindow.h"
#include <memory>
#include <stb.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>
#include "D3DImgui.h"
#include <imgui_user.h>
#include "Utilities.h"
#include "TimeSeries.h"
//#include <wincodec.h>
//#pragma comment(lib, "windowscodecs.lib")
//#include <comdef.h>
//void check_com_error(HRESULT hr) {
//	if (hr != S_OK) {
//		_com_error err(hr);
//	}
//}

void MainWindow::run(const float ms) {
	ImGui::Toolbar toolbar("myFirstToolbar##foo");
	toolbar.setProperties(false, true, false, ImVec2(0.0, 0.1), ImVec2(0.6, 1.0));
	if (toolbar.getNumButtons() == 0) {
		ImVec2 uv0, uv1;
		char buf[24];
		for (int i = 0; i < 10; i++) {
			uv0 = ImVec2(0, 0);
			uv1 = ImVec2(0.5, 1);
			stbsp_snprintf(buf, 24, "Toolbar button %d", i + 1);
			toolbar.addButton(ImGui::Toolbutton(buf, (void*)data_, uv0, uv1, ImVec2(64, 64)));
		}
		toolbar.addSeparator(16);
		toolbar.addButton(ImGui::Toolbutton("Tooltip button 11", (void*)data_, uv0, uv1, ImVec2(64, 64), true, false, ImVec4(1.0, 1.0, 1.0, 1.0)));
		toolbar.addButton(ImGui::Toolbutton("Tooltip button 12", (void*)data_, uv0, uv1, ImVec2(64, 64), true, false, ImVec4(1.0, 1.0, 1.0, 1.0)));
	}
	const int pressed = toolbar.render();
	switch (pressed) {
		case 0:
			import_ts_window = true;
			break;
		default:
			break;
	}
	mgr_.run(import_ts_window);
}

void MainWindow::init() {
	MethodLogger mlog("MainWindow::init");
	static auto settings_icon = "res/icons8-Settings-64.png";
	int x, y, comp;
	auto img = stbi_load(settings_icon, &x, &y, &comp, STBI_rgb_alpha);
	mlog.logger()->warn_if(img == nullptr, "Error loading image {}", settings_icon);
	data_ = imgui_.CreateShaderResource(x, y, comp, img);
}
