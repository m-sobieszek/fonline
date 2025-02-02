//      __________        ___               ______            _
//     / ____/ __ \____  / (_)___  ___     / ____/___  ____ _(_)___  ___
//    / /_  / / / / __ \/ / / __ \/ _ \   / __/ / __ \/ __ `/ / __ \/ _ `
//   / __/ / /_/ / / / / / / / / /  __/  / /___/ / / / /_/ / / / / /  __/
//  /_/    \____/_/ /_/_/_/_/ /_/\___/  /_____/_/ /_/\__, /_/_/ /_/\___/
//                                                  /____/
// FOnline Engine
// https://fonline.ru
// https://github.com/cvet/fonline
//
// MIT License
//
// Copyright (c) 2006 - 2022, Anton Tsvetinskiy aka cvet <cvet@tut.by>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include "Common.h"

#include "Rendering.h"
#include "Settings.h"

DECLARE_EXCEPTION(AppInitException);

///@ ExportEnum
enum class KeyCode : uchar
{
    None = 0x00,
    Escape = 0x01,
    C1 = 0x02,
    C2 = 0x03,
    C3 = 0x04,
    C4 = 0x05,
    C5 = 0x06,
    C6 = 0x07,
    C7 = 0x08,
    C8 = 0x09,
    C9 = 0x0A,
    C0 = 0x0B,
    Minus = 0x0C,
    Equals = 0x0D,
    Back = 0x0E,
    Tab = 0x0F,
    Q = 0x10,
    W = 0x11,
    E = 0x12,
    R = 0x13,
    T = 0x14,
    Y = 0x15,
    U = 0x16,
    I = 0x17,
    O = 0x18,
    P = 0x19,
    Lbracket = 0x1A,
    Rbracket = 0x1B,
    Return = 0x1C,
    Lcontrol = 0x1D,
    A = 0x1E,
    S = 0x1F,
    D = 0x20,
    F = 0x21,
    G = 0x22,
    H = 0x23,
    J = 0x24,
    K = 0x25,
    L = 0x26,
    Semicolon = 0x27,
    Apostrophe = 0x28,
    Grave = 0x29,
    Lshift = 0x2A,
    Backslash = 0x2B,
    Z = 0x2C,
    X = 0x2D,
    C = 0x2E,
    V = 0x2F,
    B = 0x30,
    N = 0x31,
    M = 0x32,
    Comma = 0x33,
    Period = 0x34,
    Slash = 0x35,
    Rshift = 0x36,
    Multiply = 0x37,
    Lmenu = 0x38,
    Space = 0x39,
    Capital = 0x3A,
    F1 = 0x3B,
    F2 = 0x3C,
    F3 = 0x3D,
    F4 = 0x3E,
    F5 = 0x3F,
    F6 = 0x40,
    F7 = 0x41,
    F8 = 0x42,
    F9 = 0x43,
    F10 = 0x44,
    Numlock = 0x45,
    Scroll = 0x46,
    Numpad7 = 0x47,
    Numpad8 = 0x48,
    Numpad9 = 0x49,
    Subtract = 0x4A,
    Numpad4 = 0x4B,
    Numpad5 = 0x4C,
    Numpad6 = 0x4D,
    Add = 0x4E,
    Numpad1 = 0x4F,
    Numpad2 = 0x50,
    Numpad3 = 0x51,
    Numpad0 = 0x52,
    Decimal = 0x53,
    F11 = 0x57,
    F12 = 0x58,
    Numpadenter = 0x9C,
    Rcontrol = 0x9D,
    Divide = 0xB5,
    Sysrq = 0xB7,
    Rmenu = 0xB8,
    Pause = 0xC5,
    Home = 0xC7,
    Up = 0xC8,
    Prior = 0xC9,
    Left = 0xCB,
    Right = 0xCD,
    End = 0xCF,
    Down = 0xD0,
    Next = 0xD1,
    Insert = 0xD2,
    Delete = 0xD3,
    Lwin = 0xDB,
    Rwin = 0xDC,
    Text = 0xFE,
    ClipboardPaste = 0xFF,
};

///@ ExportEnum
enum class MouseButton
{
    Left = 0,
    Right = 1,
    Middle = 2,
    WheelUp = 3,
    WheelDown = 4,
    Ext0 = 5,
    Ext1 = 6,
    Ext2 = 7,
    Ext3 = 8,
    Ext4 = 9,
};

struct InputEvent
{
    enum class EventType
    {
        NoneEvent,
        MouseMoveEvent,
        MouseDownEvent,
        MouseUpEvent,
        MouseWheelEvent,
        KeyDownEvent,
        KeyUpEvent,
    } Type {};

    struct MouseMoveEvent
    {
        int MouseX {};
        int MouseY {};
        int DeltaX {};
        int DeltaY {};
    } MouseMove {};

    struct MouseDownEvent
    {
        MouseButton Button {};
    } MouseDown {};

    struct MouseUpEvent
    {
        MouseButton Button {};
    } MouseUp {};

    struct MouseWheelEvent
    {
        int Delta {};
    } MouseWheel {};

    struct KeyDownEvent
    {
        KeyCode Code {};
        string Text {};
    } KeyDown {};

    struct KeyUpEvent
    {
        KeyCode Code {};
    } KeyUp {};

    InputEvent() = default;
    explicit InputEvent(MouseMoveEvent ev) : Type {EventType::MouseMoveEvent}, MouseMove {ev} { }
    explicit InputEvent(MouseDownEvent ev) : Type {EventType::MouseDownEvent}, MouseDown {ev} { }
    explicit InputEvent(MouseUpEvent ev) : Type {EventType::MouseUpEvent}, MouseUp {ev} { }
    explicit InputEvent(MouseWheelEvent ev) : Type {EventType::MouseWheelEvent}, MouseWheel {ev} { }
    explicit InputEvent(KeyDownEvent ev) : Type {EventType::KeyDownEvent}, KeyDown {std::move(ev)} { }
    explicit InputEvent(KeyUpEvent ev) : Type {EventType::KeyUpEvent}, KeyUp {ev} { }
};

class Application final
{
    friend void InitApp(int argc, char** argv, string_view name_appendix);

    Application(int argc, char** argv, string_view name_appendix);

public:
    struct AppWindow
    {
        [[nodiscard]] auto GetSize() const -> tuple<int, int>;
        [[nodiscard]] auto GetPosition() const -> tuple<int, int>;
        [[nodiscard]] auto GetMousePosition() const -> tuple<int, int>;
        [[nodiscard]] auto IsFocused() const -> bool;
        [[nodiscard]] auto IsFullscreen() const -> bool;

        void SetSize(int w, int h);
        void SetPosition(int x, int y);
        void SetMousePosition(int x, int y);
        void Minimize();
        auto ToggleFullscreen(bool enable) -> bool;
        void Blink();
        void AlwaysOnTop(bool enable);

    private:
        int _nonConstHelper {};
    };

    struct AppRender
    {
        static constexpr uint MAX_ATLAS_SIZE = 4096;
        static constexpr uint MIN_ATLAS_SIZE = 2048;
        static const uint& MAX_ATLAS_WIDTH;
        static const uint& MAX_ATLAS_HEIGHT;
        static const uint& MAX_BONES;

        [[nodiscard]] auto GetRenderTarget() -> RenderTexture*;
        [[nodiscard]] auto CreateTexture(uint width, uint height, bool linear_filtered, bool with_depth) -> RenderTexture*;
        [[nodiscard]] auto CreateDrawBuffer(bool is_static) -> RenderDrawBuffer*;
        [[nodiscard]] auto CreateEffect(EffectUsage usage, string_view name, string_view defines, const RenderEffectLoader& loader) -> RenderEffect*;

        void SetRenderTarget(RenderTexture* tex);
        void ClearRenderTarget(optional<uint> color, bool depth = false, bool stencil = false);
        void EnableScissor(int x, int y, uint w, uint h);
        void DisableScissor();
    };

    struct AppInput
    {
        static constexpr uint DROP_FILE_STRIP_LENGHT = 2048;

        [[nodiscard]] auto GetClipboardText() -> string;

        [[nodiscard]] auto PollEvent(InputEvent& event) -> bool;

        void PushEvent(const InputEvent& event);
        void SetClipboardText(string_view text);
    };

    struct AppAudio
    {
        static const int AUDIO_FORMAT_U8;
        static const int AUDIO_FORMAT_S16;

        using AudioStreamCallback = std::function<void(uchar*)>;

        [[nodiscard]] auto IsEnabled() -> bool;
        [[nodiscard]] auto GetStreamSize() -> uint;
        [[nodiscard]] auto GetSilence() -> uchar;

        [[nodiscard]] auto ConvertAudio(int format, int channels, int rate, vector<uchar>& buf) -> bool;

        void SetSource(AudioStreamCallback stream_callback);
        void MixAudio(uchar* output, uchar* buf, int volume);
        void LockDevice();
        void UnlockDevice();

    private:
        struct AudioConverter;
        vector<AudioConverter*> _converters {};
    };

    Application(const Application&) = delete;
    Application(Application&&) noexcept = delete;
    auto operator=(const Application&) = delete;
    auto operator=(Application&&) noexcept = delete;
    ~Application() = default;

    [[nodiscard]] auto GetName() const -> string_view { return _name; }

#if FO_IOS
    void SetMainLoopCallback(void (*callback)(void*));
#endif
    void BeginFrame();
    void EndFrame();

    GlobalSettings Settings;

    EventObserver<> OnFrameBegin {};
    EventObserver<> OnFrameEnd {};
    EventObserver<> OnPause {};
    EventObserver<> OnResume {};
    EventObserver<> OnLowMemory {};
    EventObserver<> OnQuit {};

    AppWindow Window;
    AppRender Render;
    AppInput Input;
    AppAudio Audio;

private:
    string _name {};
    uint64 _time {};
    uint64 _timeFrequency {};
    bool _mouseCanUseGlobalState {};
    int _pendingMouseLeaveFrame {};
    int _mouseButtonsDown {};
    RenderDrawBuffer* _imguiDrawBuf {};
    RenderEffect* _imguiEffect {};
    EventDispatcher<> _onFrameBeginDispatcher {OnFrameBegin};
    EventDispatcher<> _onFrameEndDispatcher {OnFrameEnd};
    EventDispatcher<> _onPauseDispatcher {OnPause};
    EventDispatcher<> _onResumeDispatcher {OnResume};
    EventDispatcher<> _onLowMemoryDispatcher {OnLowMemory};
    EventDispatcher<> _onQuitDispatcher {OnQuit};
};

class MessageBox final
{
public:
    MessageBox() = delete;

    static void ShowErrorMessage(string_view title, string_view message, string_view traceback);
};
