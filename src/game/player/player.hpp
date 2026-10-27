#pragma once

#include "raylib.h"

#include "../entity/entity.hpp"

namespace game {

    constexpr float kAcceleration = 0.12f;
    constexpr float kFriction = 0.15f;
    constexpr float kStopEpsilon  = 0.05f;

    constexpr float kWalkSpeed   = 2.5f;
    constexpr float kJogSpeed    = 6.0f;
    constexpr float kSprintSpeed = 15.0f; // 9.0

    class Player : public entity::Entity {
    public:
        enum class MoveMode { Walk, Jog, Sprint };

        Player();
        ~Player();
        void update(float dt, const world::World& world) override;
        void animate(float dt) override;
        void setMoveDir(Vector2 d);
        void setMoveMode(MoveMode m);
        MoveMode moveMode() const { return mode_; }
        void updateFacing(Vector2 cursor, bool hasCursor);
        void adjustAnimTrim(float delta) { animTrim_ += delta; }
        float animTrim() const { return animTrim_; }
        float speed() const { return speed_; }
        bool isMoving() const { return Vector2LengthSqr(velocity_) > 0.01f; }
        Vector2 velocityDir() const { return Vector2Normalize(velocity_); }

        // --- debug ---
        float debugSpeed() const { return Vector2Length(velocity_); }
        float debugRatio() const { return clipSpeed_ > 0.f ? Vector2Length(velocity_) / clipSpeed_ : 0.f; }
        float debugClipSpeed() const { return clipSpeed_; }
        float debugAnimTrim() const { return animTrim_; }
        float debugFrame() const { return currFrame_; }
        const char* debugClipName() const { return (currAnim_ >= 0 && currAnim_ < animCount_) ? anims_[currAnim_].name : "<none>"; }
        const char* debugModeName() const {
            switch (mode_) {
                case MoveMode::Jog:    return "jog";
                case MoveMode::Sprint: return "sprint";
                default:               return "walk";
            }
        }
        int debugKeyframes() const { return (currAnim_ >= 0 && currAnim_ < animCount_) ? anims_[currAnim_].keyframeCount : 0; }
        float debugBlend() const { return blend_; }
    private:
        void play(int animIndex);
        int findAnim(const char* name) const;

        ModelAnimation* anims_ {};
        int animCount_{};
        int currAnim_{}, prevAnim_{};
        float currFrame_{}, prevFrame_{};
        float blend_{1.f};

        float clipSpeed_ {1.f};      // родная скорость текущего клипа (ед/с, с масштабом)
        float animTrim_ {3.f};       // ручная подстройка скорости анимации

        static constexpr float kBlendTime {0.2f};

        Vector2 moveDir_ {};
        Vector2 velocity_ {};
        MoveMode mode_ {MoveMode::Walk};
        float speed_ { kWalkSpeed };

        const Vector3 colliderOffset_ {0.f, 1.f, 0.f};
    };
}
