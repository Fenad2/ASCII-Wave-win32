#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr int kFrameMilliseconds = 16;
constexpr std::array<WORD, 12> kWaterColors{
    0,
    FOREGROUND_BLUE,
    FOREGROUND_BLUE,
    FOREGROUND_BLUE | FOREGROUND_GREEN,
    FOREGROUND_BLUE | FOREGROUND_GREEN,
    FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED,
    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

struct Wave {
    float directionX;
    float directionY;
    float frequency;
    float speed;
    float amplitude;
    float phase;
};

class AsciiOcean {
public:
    explicit AsciiOcean(HANDLE output)
        : output_(output) {}

    void run() {
        configureConsole();
        hideCursor();

        auto lastFrame = Clock::now();
        while (running_) {
            const auto frameStart = Clock::now();
            const float deltaSeconds = std::min(
                std::chrono::duration<float>(frameStart - lastFrame).count(), 0.1F);
            lastFrame = frameStart;

            handleKeyboard();
            updateWindowMotion(deltaSeconds);
            render(deltaSeconds);

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - frameStart);
            if (elapsed.count() < kFrameMilliseconds) {
                Sleep(static_cast<DWORD>(kFrameMilliseconds - elapsed.count()));
            }
        }

        restoreConsole();
    }

private:
    using Clock = std::chrono::steady_clock;

    HANDLE output_;
    HANDLE input_ = GetStdHandle(STD_INPUT_HANDLE);
    CONSOLE_CURSOR_INFO originalCursor_{};
    CONSOLE_SCREEN_BUFFER_INFO originalBufferInfo_{};
    std::vector<CHAR_INFO> framebuffer_;
    COORD framebufferSize_{};
    RECT previousWindowRect_{};
    bool havePreviousWindowRect_ = false;
    bool running_ = true;
    float time_ = 0.0F;
    float waveScale_ = 1.0F;
    float animationSpeed_ = 1.0F;
    float windowDriftX_ = 0.0F;
    float windowDriftY_ = 0.0F;

    const std::array<Wave, 6> waves_{
        Wave{0.96F, 0.28F, 0.105F, 0.92F, 0.62F, 0.3F},
        Wave{-0.63F, 0.78F, 0.071F, 1.41F, 0.43F, 1.5F},
        Wave{0.25F, -0.97F, 0.160F, 0.57F, 0.25F, 2.4F},
        Wave{-0.88F, -0.47F, 0.130F, 1.08F, 0.30F, 3.6F},
        Wave{0.71F, -0.70F, 0.205F, 1.79F, 0.17F, 0.7F},
        Wave{-0.10F, 0.99F, 0.048F, 0.68F, 0.32F, 5.1F},
    };

    void configureConsole() {
        GetConsoleCursorInfo(output_, &originalCursor_);
        GetConsoleScreenBufferInfo(output_, &originalBufferInfo_);

        DWORD inputMode = 0;
        if (GetConsoleMode(input_, &inputMode)) {
            SetConsoleMode(input_, inputMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
        }

        SetConsoleTitleW(L"ASCII WAVE");
    }

    void hideCursor() const {
        CONSOLE_CURSOR_INFO cursorInfo = originalCursor_;
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(output_, &cursorInfo);
    }

    void restoreConsole() const {
        SetConsoleCursorInfo(output_, &originalCursor_);
        SetConsoleCursorPosition(output_, {0, originalBufferInfo_.dwCursorPosition.Y});
        std::puts("\nASCII WAVE stopped.");
    }

    void handleKeyboard() {
        INPUT_RECORD records[32];
        DWORD count = 0;
        while (GetNumberOfConsoleInputEvents(input_, &count) && count > 0) {
            DWORD read = 0;
            if (!ReadConsoleInputW(input_, records, std::min<DWORD>(count, 32), &read)) {
                return;
            }

            for (DWORD i = 0; i < read; ++i) {
                const INPUT_RECORD& record = records[i];
                if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown) {
                    continue;
                }

                switch (record.Event.KeyEvent.wVirtualKeyCode) {
                case VK_ESCAPE:
                case 'Q':
                    running_ = false;
                    break;
                case VK_OEM_PLUS:
                case VK_ADD:
                    waveScale_ = std::min(waveScale_ + 0.1F, 2.5F);
                    break;
                case VK_OEM_MINUS:
                case VK_SUBTRACT:
                    waveScale_ = std::max(waveScale_ - 0.1F, 0.2F);
                    break;
                case VK_OEM_4:
                    animationSpeed_ = std::max(animationSpeed_ - 0.1F, 0.1F);
                    break;
                case VK_OEM_6:
                    animationSpeed_ = std::min(animationSpeed_ + 0.1F, 3.0F);
                    break;
                case 'R':
                    windowDriftX_ = 0.0F;
                    windowDriftY_ = 0.0F;
                    break;
                default:
                    break;
                }
            }
        }
    }

    void updateWindowMotion(float deltaSeconds) {
        HWND window = GetConsoleWindow();
        RECT currentRect{};
        if (window == nullptr || !GetWindowRect(window, &currentRect)) {
            return;
        }

        if (havePreviousWindowRect_) {
            const float dx = static_cast<float>(currentRect.left - previousWindowRect_.left);
            const float dy = static_cast<float>(currentRect.top - previousWindowRect_.top);

            windowDriftX_ = std::clamp(windowDriftX_ - dx * 0.0065F, -1.2F, 1.2F);
            windowDriftY_ = std::clamp(windowDriftY_ - dy * 0.0065F, -0.75F, 0.75F);
        }

        previousWindowRect_ = currentRect;
        havePreviousWindowRect_ = true;

        const float decay = std::exp(-3.2F * deltaSeconds);
        windowDriftX_ *= decay;
        windowDriftY_ *= decay;
    }

    [[nodiscard]] float heightAt(float x, float y) const {
        float height = 0.0F;
        for (const Wave& wave : waves_) {
            const float position = (x * wave.directionX + y * wave.directionY) * wave.frequency;
            height += std::sin(position + time_ * wave.speed + wave.phase) * wave.amplitude;
        }
        return height / 2.09F;
    }

    void ensureFramebuffer(COORD size) {
        if (size.X == framebufferSize_.X && size.Y == framebufferSize_.Y) {
            return;
        }

        framebufferSize_ = size;
        framebuffer_.assign(static_cast<std::size_t>(size.X) * static_cast<std::size_t>(size.Y), CHAR_INFO{});
    }

    void render(float deltaSeconds) {
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(output_, &info)) {
            running_ = false;
            return;
        }

        const COORD size{
            static_cast<SHORT>(info.srWindow.Right - info.srWindow.Left + 1),
            static_cast<SHORT>(info.srWindow.Bottom - info.srWindow.Top + 1),
        };
        if (size.X < 2 || size.Y < 2) {
            return;
        }

        ensureFramebuffer(size);
        time_ += deltaSeconds * animationSpeed_;

        for (int row = 0; row < size.Y; ++row) {
            for (int column = 0; column < size.X; ++column) {
                const float x = (static_cast<float>(column) - size.X * 0.5F) * 1.55F
                    + windowDriftX_ * 56.0F;
                const float y = (static_cast<float>(row) - size.Y * 0.5F) * 2.4F
                    + windowDriftY_ * 44.0F;
                const float height = heightAt(x, y) * waveScale_;
                const float normalized = std::clamp((height + 1.0F) * 0.5F, 0.0F, 1.0F);
                const float ripple = std::sin(x * 0.33F - y * 0.19F + time_ * 2.7F) * 0.055F;
                const int glyphIndex = std::clamp(static_cast<int>((normalized + ripple) * 94.0F), 0, 94);
                const int colorIndex = std::clamp(static_cast<int>(normalized * (kWaterColors.size() - 1)), 0,
                                                  static_cast<int>(kWaterColors.size() - 1));

                CHAR_INFO& cell = framebuffer_[static_cast<std::size_t>(row) * size.X + column];
                cell.Char.AsciiChar = static_cast<CHAR>(32 + glyphIndex);
                cell.Attributes = kWaterColors[colorIndex];
            }
        }

        drawHud(size);

        SMALL_RECT destination{
            info.srWindow.Left,
            info.srWindow.Top,
            info.srWindow.Right,
            info.srWindow.Bottom,
        };
        WriteConsoleOutputA(output_, framebuffer_.data(), size, {0, 0}, &destination);
    }

    void drawHud(COORD size) {
        const std::string label = " ASCII WAVE   drag window: current   +/- amplitude   [/] speed   R reset   Q quit ";
        const int maximum = std::min<int>(static_cast<int>(label.size()), size.X);
        for (int column = 0; column < maximum; ++column) {
            CHAR_INFO& cell = framebuffer_[column];
            cell.Char.AsciiChar = label[static_cast<std::size_t>(column)];
            cell.Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
                | BACKGROUND_BLUE;
        }
    }
};

} // namespace

int main() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE || output == nullptr) {
        return 1;
    }

    AsciiOcean ocean(output);
    ocean.run();
    return 0;
}
