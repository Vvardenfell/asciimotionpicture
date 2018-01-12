#include <windows.h>

#include <codecvt>
#include <locale>
#include <string>
#include <limits>
#include <vector>
#include <cmath>
#include <algorithm>

#include <iostream>

class Unicode {
private:

    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8wide;
    static std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> utf816;
    static std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> utf832;

public:

    static std::wstring widen(const std::string& in) {
        return Unicode::utf8wide.from_bytes(in);
    }

    static std::string narrow(const std::wstring& in) {
        return Unicode::utf8wide.to_bytes(in);
    }

    static std::u16string to16(const std::string& in) {
		auto p = Unicode::utf816.from_bytes(in);
		return std::u16string(reinterpret_cast<const char16_t*> (p.data()));
    }

    static std::u32string to32(const std::string& in) {
		auto p = Unicode::utf832.from_bytes(in);
		return std::u32string(reinterpret_cast<const char32_t*>(p.data()));
    }

    static inline std::string to8(const std::wstring& in) {
        return narrow(in);
    }

    static std::string to8(const std::u16string& in) {
		auto p = reinterpret_cast<const int16_t *>(in.data());
        return Unicode::utf816.to_bytes(p, p+in.size());
    }

    static std::string to8(const std::u32string& in) {
		auto p = reinterpret_cast<const int32_t *>(in.data());
        return Unicode::utf832.to_bytes(p, p+in.size());
    }
};

class FrameBuffer;

class Windows {
private:

    const static int WINDOW_WIDTH = CW_USEDEFAULT, WINDOW_HEIGHT = CW_USEDEFAULT; // orig: 806, 485

    static HWND window_handle;

    static void init_font();

    static void register_window_class(HINSTANCE hInstance, const std::wstring& application_name_wide, LRESULT (*window_event_handler)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));

public:

    static HFONT font;

    static FrameBuffer* frame;

    static void init(HINSTANCE hInstance, int iCmdShow, const char* application_name);

    static void redraw() {
        InvalidateRect(Windows::window_handle, NULL, TRUE);
    }
};


class FrameBuffer {
private:

    std::size_t width, height;
    std::vector<char32_t> buffer;

public:

    FrameBuffer(std::size_t width, std::size_t height) : width(width), height(height), buffer(width * height, char32_t{}) {};

    void clear() {
        this->buffer = std::vector<char32_t>(this->width * this->height, char32_t{});
    }

    std::u32string get_glyph_frame() const;

    void render(char32_t glyph, std::size_t x, std::size_t y) {
        this->buffer[(this->width * y) + x] = glyph;
    }
};