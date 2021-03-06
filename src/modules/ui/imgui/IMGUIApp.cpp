/**
 * @file
 */

#include "IMGUIApp.h"

#include "core/io/Filesystem.h"
#include "core/TimeProvider.h"
#include "core/command/Command.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "math/Rect.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "video/ScopedViewPort.h"
#include "video/TextureConfig.h"
#include "IMGUI.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <thread>

namespace ui {
namespace imgui {

IMGUIApp::IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {
}

IMGUIApp::~IMGUIApp() {
}

bool IMGUIApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return true;
	}
	if (y > 0) {
		_mouseWheel = 1;
	} else if (y < 0) {
		_mouseWheel = -1;
	}
	return Super::onMouseWheel(x, y);
}

void IMGUIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (_console.isActive()) {
		return;
	}
	Super::onMouseButtonRelease(x, y, button);
}

void IMGUIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (_console.onMouseButtonPress(x, y, button)) {
		return;
	}
	if (button == SDL_BUTTON_LEFT) {
		_mousePressed[0] = true;
	} else if (button == SDL_BUTTON_RIGHT) {
		_mousePressed[1] = true;
	} else if (button == SDL_BUTTON_MIDDLE) {
		_mousePressed[2] = true;
	}
	Super::onMouseButtonPress(x, y, button, clicks);
}

bool IMGUIApp::onTextInput(const core::String& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharactersUTF8(text.c_str());
	return true;
}

bool IMGUIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier)) {
		return true;
	}
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < lengthof(io.KeysDown));
	io.KeysDown[key] = true;
	const int16_t modifiers = SDL_GetModState();
	io.KeyShift = (modifiers & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifiers & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifiers & KMOD_ALT) != 0;
	io.KeySuper = (modifiers & KMOD_GUI) != 0;
	return true;
}

bool IMGUIApp::onKeyRelease(int32_t key, int16_t modifier) {
	if (_console.isActive()) {
		return true;
	}
	if (Super::onKeyRelease(key, modifier)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < lengthof(io.KeysDown));
	io.KeysDown[key] = false;
	io.KeyShift = (modifier & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifier & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifier & KMOD_ALT) != 0;
	io.KeySuper = (modifier & KMOD_GUI) != 0;
	return true;
}

void IMGUIApp::onWindowResize(int windowWidth, int windowHeight) {
	Super::onWindowResize(windowWidth, windowHeight);
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)_frameBufferDimension.x, (float)_frameBufferDimension.y);
	io.DisplayFramebufferScale = ImVec2(_dpiHorizontalFactor, _dpiVerticalFactor);

	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_camera.update(0L);
	video::ScopedShader scoped(_shader);
	_shader.setViewprojection(_camera.projectionMatrix());
	_shader.setModel(glm::mat4(1.0f));
}

core::AppState IMGUIApp::onConstruct() {
	const core::AppState state = Super::onConstruct();
	_console.construct();
	core::Command::registerCommand("cl_rendertracing", [&] (const core::CmdArgs& args) { _renderTracing ^= true; }).setHelp("Toggle the trace data rendering");
	return state;
}

static const char* _getClipboardText(void*) {
	const char* text = SDL_GetClipboardText();
	if (!text) {
		return nullptr;
	}
	const int len = SDL_strlen(text);
	if (len == 0) {
		SDL_free((void*) text);
		return "";
	}
	static ImVector<char> clipboardBuffer;
	// Optional branch to keep clipboardBuffer.capacity() low:
	if (len <= clipboardBuffer.capacity() && clipboardBuffer.capacity() > 512) {
		ImVector<char> emptyBuffer;
		clipboardBuffer.swap(emptyBuffer);
	}
	clipboardBuffer.resize(len + 1);
	SDL_strlcpy(&clipboardBuffer[0], text, clipboardBuffer.size());
	SDL_free((void*) text);
	return (const char*) &clipboardBuffer[0];
}

static void _setClipboardText(void*, const char* text) {
	SDL_SetClipboardText(text);
}

core::AppState IMGUIApp::onInit() {
	const core::AppState state = Super::onInit();
	video::checkError();
	if (state != core::AppState::Running) {
		return state;
	}

	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");

	if (!_shader.setup()) {
		Log::error("Could not load the ui shader");
		return core::AppState::InitFailure;
	}

	_bufferIndex = _vbo.create();
	if (_bufferIndex < 0) {
		Log::error("Failed to create ui vertex buffer");
		return core::AppState::InitFailure;
	}
	_vbo.setMode(_bufferIndex, video::BufferMode::Stream);
	_indexBufferIndex = _vbo.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_indexBufferIndex < 0) {
		Log::error("Failed to create ui index buffer");
		return core::AppState::InitFailure;
	}

	_camera = video::uiCamera(glm::ivec2(0), frameBufferDimension(), windowDimension());

	_vbo.addAttribute(_shader.getColorAttribute(_bufferIndex, &ImDrawVert::r, true));
	_vbo.addAttribute(_shader.getTexcoordAttribute(_bufferIndex, &ImDrawVert::u));
	_vbo.addAttribute(_shader.getPosAttribute(_bufferIndex, &ImDrawVert::x));

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	const core::String iniFile = _appname + "-imgui.ini";
	_writePathIni = _filesystem->writePath(iniFile.c_str());
	io.IniFilename = _writePathIni.c_str();
	const core::String logFile = _appname + "-imgui.log";
	_writePathLog = _filesystem->writePath(logFile.c_str());
	io.LogFilename = _writePathLog.c_str();
	io.DisplaySize = ImVec2((float)_frameBufferDimension.x, (float)_frameBufferDimension.y);
	io.DisplayFramebufferScale = ImVec2(_dpiHorizontalFactor, _dpiVerticalFactor);
	ImFontConfig fontCfg;
	fontCfg.SizePixels = 13.0f * _dpiFactor;
	io.Fonts->AddFontDefault(&fontCfg);

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	_texture = video::genTexture();
	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA);
	video::bindTexture(video::TextureUnit::Upload, cfg.type(), _texture);
	video::setupTexture(cfg);
	video::uploadTexture(cfg.type(), cfg.format(), width, height, pixels, 0);
	io.Fonts->TexID = (void *) (intptr_t) _texture;

	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;
	io.RenderDrawListsFn = nullptr;
	io.SetClipboardTextFn = _setClipboardText;
	io.GetClipboardTextFn = _getClipboardText;
	io.ClipboardUserData = nullptr;

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(_window, &wmInfo);
	io.ImeWindowHandle = wmInfo.info.win.window;
#endif
	SDL_StartTextInput();

	_console.init();

	Log::info("Set up imgui");

	return state;
}

core::AppState IMGUIApp::onRunning() {
	core_trace_scoped(IMGUIAppOnRunning);
	core::AppState state = Super::onRunning();

	if (state != core::AppState::Running) {
		return state;
	}

	_console.update(_deltaFrameMillis);

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(_mousePos.x, _mousePos.y);

	core_assert(_bufferIndex > -1);
	core_assert(_indexBufferIndex > -1);

	{
		core_trace_scoped(IMGUIAppBeforeUI);
		beforeUI();
	}

	const bool renderUI = _renderUI->boolVal();

	io.DeltaTime = _deltaFrameSeconds;
	const uint32_t mouseMask = SDL_GetMouseState(nullptr, nullptr);
	// If a mouse press event came, always pass it as "mouse held this frame",
	// so we don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[0] = _mousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	io.MouseDown[1] = _mousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	_mousePressed[0] = _mousePressed[1] = _mousePressed[2] = false;

	io.MouseWheel = (float)_mouseWheel;
	_mouseWheel = 0;

	ImGui::NewFrame();

	showCursor(io.MouseDrawCursor ? false : true);

	video::ScopedShader scopedShader(_shader);
	_shader.setViewprojection(_camera.projectionMatrix());
	_shader.setModel(glm::mat4(1.0f));
	_shader.setTexture(video::TextureUnit::Zero);

	video::ScopedViewPort scopedViewPort(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);

	video::enable(video::State::Blend);
	video::disable(video::State::DepthTest);
	video::enable(video::State::Scissor);
	video::disable(video::State::CullFace);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	video::blendEquation(video::BlendEquation::Add);

	if (renderUI) {
		core_trace_scoped(IMGUIAppOnRenderUI);
		onRenderUI();
	}

	if (ImGui::IsAnyWindowHovered()) {
		core::setBindingContext(core::BindingContext::UserInterface);
	} else {
		core::setBindingContext(core::BindingContext::World);
	}

	const int index = _currentFrameCounter % _maxMeasureSize;
	_frameMillis[index] = _deltaFrameMillis;

	{
		core_trace_scoped(IMGUIAppOnRenderTracing);
		renderTracing();
	}

	const math::Rect<int> rect(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	_console.render(rect, _deltaFrameMillis);
	ImGui::EndFrame();
	ImGui::Render();

	ImDrawData* drawData = ImGui::GetDrawData();
	drawData->ScaleClipRects(io.DisplayFramebufferScale);

	for (int n = 0; n < drawData->CmdListsCount; ++n) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		const ImDrawIdx* idxBufferOffset = nullptr;

		core_assert_always(_vbo.update(_bufferIndex, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert)));
		core_assert_always(_vbo.update(_indexBufferIndex, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx)));
		video::ScopedBuffer scopedBuf(_vbo);

		for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
			if (cmd->UserCallback) {
				cmd->UserCallback(cmdList, cmd);
			} else {
				video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, (video::Id)(intptr_t)cmd->TextureId);
				const ImVec4& cr = cmd->ClipRect;
				video::scissor(cr.x, cr.y, cr.z - cr.x, cr.w - cr.y);
				video::drawElements<ImDrawIdx>(video::Primitive::Triangles, cmd->ElemCount, (void*)idxBufferOffset);
			}
			idxBufferOffset += cmd->ElemCount;
		}
	}

	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	return core::AppState::Running;
}

void IMGUIApp::onAfterFrame() {
	Super::onAfterFrame();
	if (_traceMeasuresPause) {
		return;
	}
	++_currentFrameCounter;
	const int index = _currentFrameCounter % _maxMeasureSize;
	std::lock_guard lock(_traceMutex);
	for (auto& entry : _traceMeasuresLastFrame[index]) {
		delete entry.second;
	}
	_traceMeasuresLastFrame[index] = _traceMeasures;
	_traceMeasures.clear();
}

IMGUIApp::TraceData::TraceData(uint64_t _value, const char *_name) :
		value(_value), name(_name) {
	children.reserve(512);
}

IMGUIApp::TraceData::~TraceData() {
	const int size = children.size();
	for (int i = 0; i < size; ++i) {
		delete children[i];
	}
	children.clear();
}

IMGUIApp::TraceRoot::TraceRoot(uint64_t nanos, const char *name) :
		data(new TraceData(nanos, name)) {
	queue.push(data);
}

IMGUIApp::TraceRoot::TraceRoot(TraceRoot&& other) :
		data(nullptr) {
	std::swap(data, other.data);
	std::swap(queue, other.queue);
}

IMGUIApp::TraceRoot::~TraceRoot() {
	delete data;
	data = nullptr;
}

void IMGUIApp::TraceRoot::begin(uint64_t nanos, const char *name) {
	core_assert(!queue.empty());
	TraceData* top = queue.top();
	std::vector<TraceData*> &children = top->children;
	TraceData* t = new TraceData(nanos, name);
	children.emplace_back(t);
	queue.push(t);
}

void IMGUIApp::TraceRoot::end(uint64_t nanos) {
	core_assert(!queue.empty());
	TraceData* top = queue.top();
	top->delta = nanos - top->value;
	queue.pop();
}

void IMGUIApp::traceBeginFrame(const char *threadName) {
	//Super::traceBeginFrame(threadName);
	if (_traceMeasuresPause) {
		return;
	}
	const std::thread::id id = std::this_thread::get_id();
	const uint64_t nanos = core::TimeProvider::systemNanos();
	TraceRoot* traceRoot = new TraceRoot(nanos, threadName);
	std::lock_guard<std::mutex> lock(_traceMutex);
	auto i = _traceMeasures.insert(std::make_pair(id, traceRoot));
	if (!i.second) {
		delete i.first->second;
	}
}

void IMGUIApp::traceBegin(const char *threadName, const char* name) {
	//Super::traceBegin(threadName, name);
	if (_traceMeasuresPause) {
		return;
	}
	const std::thread::id id = std::this_thread::get_id();
	const uint64_t nanos = core::TimeProvider::systemNanos();
	std::lock_guard<std::mutex> lock(_traceMutex);
	auto measureIter = _traceMeasures.find(id);
	// might happen if this was activated after the traceBeginFrame was already fired.
	if (measureIter == _traceMeasures.end()) {
		return;
	}
	TraceRoot* frame = measureIter->second;
	frame->begin(nanos, name);
}

void IMGUIApp::traceEnd(const char *threadName) {
	//Super::traceEnd(threadName);
	if (_traceMeasuresPause) {
		return;
	}
	const std::thread::id id = std::this_thread::get_id();
	const uint64_t nanos = core::TimeProvider::systemNanos();
	std::lock_guard<std::mutex> lock(_traceMutex);
	auto measureIter = _traceMeasures.find(id);
	// might happen if this was activated after the traceBeginFrame was already fired.
	if (measureIter == _traceMeasures.end()) {
		return;
	}
	TraceRoot* frame = measureIter->second;
	frame->end(nanos);
}

void IMGUIApp::traceEndFrame(const char *threadName) {
	//Super::traceEndFrame(threadName);
	if (_traceMeasuresPause) {
		return;
	}
	const std::thread::id id = std::this_thread::get_id();
	std::lock_guard<std::mutex> lock(_traceMutex);
	auto measureIter = _traceMeasures.find(id);
	// might happen if this was activated after the traceBeginFrame was already fired.
	if (measureIter == _traceMeasures.end()) {
		return;
	}
	TraceRoot* frame = measureIter->second;
	const uint64_t nanos = core::TimeProvider::systemNanos();
	frame->end(nanos);
}

void IMGUIApp::addSubTrees(const TraceData* data, bool expandAll, int &depth) const {
	const ImGuiTreeNodeFlags flags = (expandAll || depth <= 5) ? ImGuiTreeNodeFlags_DefaultOpen : 0;
	const double millis = (double)data->delta / (SDL_GetPerformanceFrequency() / 1000.0);
	if (ImGui::TreeNodeEx(data->name, flags, "%s (%f)", data->name, millis)) {
		const ImU32 colBase = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImVec2 posEnd = {pos.x + core_max(1.0f, (float)millis * 10.0f), pos.y + 8.0f};
		const ImRect size(pos, posEnd);
		ImGui::ItemSize(size, 0.0f);
		ImGui::ItemAdd(size, 0, nullptr);
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DrawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), colBase);
		++depth;
		for (const TraceData* cdata : data->children) {
			addSubTrees(cdata, expandAll, depth);
		}
		ImGui::TreePop();
	}
}

void IMGUIApp::renderTracing() {
	if (!_renderTracing) {
		return;
	}
	if (!ImGui::Begin("Profiler", &_renderTracing, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar)) {
		return;
	}
	core::Array<float, _maxMeasureSize> frameMillis;
	float max = 0.0f;
	float min = 0.0f;
	float avg = 0.0f;
	for (int i = 0; i < _maxMeasureSize; ++i) {
		const int index = (_currentFrameCounter + i) % _maxMeasureSize;
		frameMillis[i] = _frameMillis[index];
		min = core_min(min, frameMillis[i]);
		max = core_max(max, frameMillis[i]);
		avg += frameMillis[i];
	}
	core::Array<float, _maxMeasureSize> frameMillisQuantile = frameMillis;
	std::sort(frameMillisQuantile.begin(), frameMillisQuantile.end());
	const int quantileIndex = int(0.95f * (float(_maxMeasureSize + 1)));
	const float twenthyQuantile = frameMillisQuantile[quantileIndex];
	avg /= (float)_maxMeasureSize;
	const core::String& maxStr = core::string::format("min: %.3fms, max: %.3fms, avg: %.3fms, quantile: %.3f", min, max, avg, twenthyQuantile);
	ImGui::PlotHistogram("Millis", &frameMillis[0], frameMillis.size(), 0, maxStr.c_str(), min, max, ImVec2(700, 100));
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Profiler", ImGuiTreeNodeFlags_DefaultOpen)) {
		static bool expandAll = false;
		ImGui::Checkbox("Expand all", &expandAll);
		ImGui::Checkbox("Pause recording", &_traceMeasuresPause);
		if (_traceMeasuresPause) {
			ImGui::SameLine();
			ImGui::InputInt("Frame index", &_currentFrameCounter, 1, 10);
			_currentFrameCounter = core_max(0, _currentFrameCounter);
		}
		const int index = _currentFrameCounter % _maxMeasureSize;
		for (auto i = _traceMeasuresLastFrame[index].begin(); i != _traceMeasuresLastFrame[index].end(); ++i) {
			if (ImGui::TreeNodeEx("Thread", ImGuiTreeNodeFlags_DefaultOpen)) {
				const TraceRoot* frameData = i->second;
				int depth = 0;
				addSubTrees(frameData->data, expandAll, depth);
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
}

core::AppState IMGUIApp::onCleanup() {
	if (ImGui::GetCurrentContext() != nullptr) {
		ImGui::DestroyContext();
	}
	_console.shutdown();
	_shader.shutdown();
	_vbo.shutdown();
	_indexBufferIndex = -1;
	_bufferIndex = -1;
	return Super::onCleanup();
}

}
}
