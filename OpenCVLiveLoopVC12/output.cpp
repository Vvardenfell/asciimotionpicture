#include "stdafx.h"
#include "output.h"

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> Unicode::utf8wide;
std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> Unicode::utf816;
std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> Unicode::utf832;

void Windows::init_font() {
    if (!AddFontResourceEx(L"unifont-7.0.06.ttf", FR_PRIVATE, 0)) {
        std::cerr << "failed to load true type font from file" << std::endl;
    }

    LOGFONT logical_font;
    memset(&logical_font, 0, sizeof(logical_font));
    logical_font.lfHeight = 19;
    logical_font.lfWeight = FW_NORMAL;
    logical_font.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    const wchar_t* face_name = L"Unifont";
    memcpy(&(logical_font.lfFaceName), face_name, wcslen(face_name) * sizeof(wchar_t) + sizeof(wchar_t));

    Windows::font = CreateFontIndirect(&logical_font);
    if (Windows::font == NULL) {
        std::cerr << "failed to create font from loaded file" << std::endl;
    }
}

void Windows::register_window_class(HINSTANCE hInstance, const std::wstring& application_name_wide, LRESULT (*window_event_handler)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)) {
    WNDCLASS window_class{};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_event_handler;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hInstance;
    window_class.hIcon = NULL; // LoadIconW(hInstance, L"IDI_MAIN_ICON");
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = application_name_wide.c_str();

    RegisterClassW(&window_class);
}

LRESULT CALLBACK window_event_handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {
    case WM_PAINT: {
		std::cout << "painting" << std::endl;
        PAINTSTRUCT ps;
        RECT rc;
        HDC hdcMem;
        HBITMAP hbmMem, hbmOld;

        BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &rc);
        hdcMem = CreateCompatibleDC(ps.hdc);
        hbmMem = CreateCompatibleBitmap(ps.hdc, rc.right-rc.left, rc.bottom-rc.top);
        hbmOld = (HBITMAP) SelectObject(hdcMem, hbmMem);

        static HBRUSH hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdcMem, &rc, hbrBkgnd);
        SelectObject(hdcMem, Windows::font);

        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(0, 0, 0));

       

        std::wstring target = Unicode::widen(Unicode::to8(Windows::frame->get_glyph_frame()));
		Windows::frame->clear();
        DrawTextW(hdcMem, target.c_str(), target.size(), &rc, DT_CENTER);

        BitBlt(ps.hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);


        EndPaint(hwnd, &ps);


        return 0;
    }
    case WM_CLOSE: {
        exit(0);
    }
    case WM_ERASEBKGND: {
        return 1; // tell windows that it's handled, though it isn't. this will be handled later in WM_PAINT to prevent flicker of gui controls
    }
    case WM_SIZE: {
    }
    default: {
            DefWindowProc(hwnd, message, wParam, lParam);
    }
    }

    return 1;
}

void Windows::init(HINSTANCE hInstance, int iCmdShow, const char* application_name) {

    init_font();

    std::wstring application_name_wide = Unicode::widen(application_name);

    register_window_class(hInstance, application_name_wide, window_event_handler);

    Windows::window_handle = CreateWindowW(application_name_wide.c_str(), application_name_wide.c_str()
                                    , WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT
                                    , Windows::WINDOW_WIDTH, Windows::WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
    ShowWindow(window_handle, iCmdShow);
}

HFONT Windows::font;
HWND Windows::window_handle;
FrameBuffer* Windows::frame;


std::u32string FrameBuffer::get_glyph_frame() const {
    std::u32string frame;

    std::size_t size = this->buffer.size();
    for (std::size_t index = 0; index < size; ++index) {
        if (index > 0 && index % this->width == 0) frame += '\n';
        frame += buffer[index];
    }

    return frame;
}