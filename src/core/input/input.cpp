#include "input.hpp"
#include <raylib.h>

input::InputMap::InputMap() { resetToDefaults(); }

void input::InputMap::bind(Action a, int key) {
    bindings_[static_cast<int>(a)] = key;
}

int input::InputMap::keyFor(Action a) const {
    return bindings_[static_cast<int>(a)];
}

bool input::InputMap::isDown(Action a) const {
    return IsKeyDown(bindings_[static_cast<int>(a)]);
}

void input::InputMap::resetToDefaults() {
    // walk
    bindings_[static_cast<int>(Action::MoveDown)] = KEY_S;
    bindings_[static_cast<int>(Action::MoveUp)] = KEY_W;
    bindings_[static_cast<int>(Action::MoveRight)] = KEY_D;
    bindings_[static_cast<int>(Action::MoveLeft)] = KEY_A;
    bindings_[static_cast<int>(Action::Jog)] = KEY_LEFT_SHIFT;
    bindings_[static_cast<int>(Action::Sprint)] = KEY_LEFT_ALT;

    // attack
    bindings_[static_cast<int>(Action::Attack)] = MOUSE_LEFT_BUTTON;

    // spells
    bindings_[static_cast<int>(Action::FirstSpell)] = KEY_ONE;
    bindings_[static_cast<int>(Action::SecondSpell)] = KEY_TWO;
}


Vector2 input::ReadMoveInput(const input::InputMap& im) {
    Vector2 dir{0.0f, 0.0f};
    if (im.isDown(input::Action::MoveUp)) dir.y -= 1.0f;
    if (im.isDown(input::Action::MoveDown)) dir.y += 1.0f;
    if (im.isDown(input::Action::MoveLeft)) dir.x -= 1.0f;
    if (im.isDown(input::Action::MoveRight)) dir.x += 1.0f;
    return dir;
}
