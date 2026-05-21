#pragma once

#include "gui_theme.hpp"
#include <windows.h>
#include <unordered_map>
#include <functional>

namespace dppbot {
namespace animation {

using namespace theme;

// Состояние анимации
struct AnimationState {
    float startValue;
    float endValue;
    ULONGLONG startTime;
    float duration;
    std::function<float(float)> easingFunc;
    bool active;

    AnimationState()
        : startValue(0.0f), endValue(0.0f), startTime(0),
          duration(0.0f), active(false) {}
};

// Контроллер анимаций
class AnimationController {
public:
    AnimationController() = default;

    // Запустить анимацию
    void Start(
        const std::string& id,
        float startValue,
        float endValue,
        float duration = theme::anim::DurationNormal,
        std::function<float(float)> easingFunc = theme::anim::EaseOutQuad
    ) {
        AnimationState state;
        state.startValue = startValue;
        state.endValue = endValue;
        state.startTime = GetTickCount64();
        state.duration = duration;
        state.easingFunc = easingFunc;
        state.active = true;

        animations_[id] = state;
    }

    // Получить текущее значение анимации
    float GetValue(const std::string& id) {
        auto it = animations_.find(id);
        if (it == animations_.end() || !it->second.active) {
            return 0.0f;
        }

        AnimationState& state = it->second;
        ULONGLONG currentTime = GetTickCount64();
        float elapsed = static_cast<float>(currentTime - state.startTime);

        if (elapsed >= state.duration) {
            state.active = false;
            return state.endValue;
        }

        float t = elapsed / state.duration;
        float easedT = state.easingFunc(t);
        return state.startValue + (state.endValue - state.startValue) * easedT;
    }

    // Проверить, активна ли анимация
    bool IsActive(const std::string& id) const {
        auto it = animations_.find(id);
        return it != animations_.end() && it->second.active;
    }

    // Остановить анимацию
    void Stop(const std::string& id) {
        auto it = animations_.find(id);
        if (it != animations_.end()) {
            it->second.active = false;
        }
    }

    // Есть ли активные анимации
    bool HasActiveAnimations() const {
        for (const auto& pair : animations_) {
            if (pair.second.active) {
                return true;
            }
        }
        return false;
    }

    // Очистить все анимации
    void Clear() {
        animations_.clear();
    }

private:
    std::unordered_map<std::string, AnimationState> animations_;
};

// Менеджер переходов для UI элементов
class TransitionManager {
public:
    TransitionManager() = default;

    // Hover эффект
    void StartHover(const std::string& id) {
        controller_.Start(
            id + "_hover",
            0.0f, 1.0f,
            theme::anim::DurationFast,
            theme::anim::EaseOutQuad
        );
    }

    void EndHover(const std::string& id) {
        controller_.Start(
            id + "_hover",
            controller_.GetValue(id + "_hover"), 0.0f,
            theme::anim::DurationFast,
            theme::anim::EaseOutQuad
        );
    }

    float GetHoverValue(const std::string& id) {
        return controller_.GetValue(id + "_hover");
    }

    // Press эффект
    void StartPress(const std::string& id) {
        controller_.Start(
            id + "_press",
            1.0f, 0.98f,
            theme::anim::DurationFast,
            theme::anim::EaseInOutQuad
        );
    }

    void EndPress(const std::string& id) {
        controller_.Start(
            id + "_press",
            controller_.GetValue(id + "_press"), 1.0f,
            theme::anim::DurationFast,
            theme::anim::EaseOutQuad
        );
    }

    float GetPressValue(const std::string& id) {
        return controller_.GetValue(id + "_press");
    }

    // Focus эффект
    void StartFocus(const std::string& id) {
        controller_.Start(
            id + "_focus",
            0.0f, 1.0f,
            theme::anim::DurationNormal,
            theme::anim::EaseOutQuad
        );
    }

    void EndFocus(const std::string& id) {
        controller_.Start(
            id + "_focus",
            controller_.GetValue(id + "_focus"), 0.0f,
            theme::anim::DurationNormal,
            theme::anim::EaseOutQuad
        );
    }

    float GetFocusValue(const std::string& id) {
        return controller_.GetValue(id + "_focus");
    }

    // Fade эффект
    void StartFadeIn(const std::string& id, float duration = theme::anim::DurationNormal) {
        controller_.Start(
            id + "_fade",
            0.0f, 1.0f,
            duration,
            theme::anim::EaseOutQuad
        );
    }

    void StartFadeOut(const std::string& id, float duration = theme::anim::DurationNormal) {
        controller_.Start(
            id + "_fade",
            1.0f, 0.0f,
            duration,
            theme::anim::EaseOutQuad
        );
    }

    float GetFadeValue(const std::string& id) {
        return controller_.GetValue(id + "_fade");
    }

    // Slide эффект
    void StartSlideIn(const std::string& id, float distance = 50.0f) {
        controller_.Start(
            id + "_slide",
            distance, 0.0f,
            theme::anim::DurationSlow,
            theme::anim::EaseOutQuad
        );
    }

    void StartSlideOut(const std::string& id, float distance = 50.0f) {
        controller_.Start(
            id + "_slide",
            0.0f, distance,
            theme::anim::DurationSlow,
            theme::anim::EaseInQuad
        );
    }

    float GetSlideValue(const std::string& id) {
        return controller_.GetValue(id + "_slide");
    }

    // Проверка активности
    bool HasActiveAnimations() const {
        return controller_.HasActiveAnimations();
    }

    // Очистка
    void Clear() {
        controller_.Clear();
    }

private:
    AnimationController controller_;
};

}  // namespace animation
}  // namespace dppbot
