#pragma once

#include "raylib.h"
#include <array>

namespace input {

    enum class Action {
        MoveUp,
        MoveDown,
        MoveRight,
        MoveLeft,
        Jog,
        Sprint,
        Attack,
        FirstSpell,
        SecondSpell,
        Count
    };

    class InputMap {
    public:
        InputMap();
        void bind(Action a, int key);
        int keyFor(Action a) const;
        bool isDown(Action a) const;
        void resetToDefaults();
    private:
        std::array<int, static_cast<std::size_t>(Action::Count)> bindings_{};
    };

    Vector2 ReadMoveInput(const InputMap& im);

}
