#include "player.hpp"

#include "game/collision/collision.hpp"
#include "raymath.h"

#include <raylib.h>

#include <algorithm>

game::Player::Player() {
    model_ = LoadModel("assets/UAL1_Standard.glb");
    anims_ = LoadModelAnimations("assets/UAL1_Standard.glb", &animCount_);
    play(findAnim("Idle_Loop"));

    yawOffset = 90.f;
    collision_ = collision::AABB(Vector3Add(transform_.position, colliderOffset_), colliderSize_);
}

game::Player::~Player() {
    UnloadModelAnimations(anims_, animCount_);
    UnloadModel(model_);
}

int game::Player::findAnim(const char* name) const {
    for (int i = 0; i < animCount_; ++i)
        if (TextIsEqual(anims_[i].name, name)) return i;
    return -1;
}

void game::Player::play(int a) {
    if (a < 0 || a == currAnim_) return;

    prevAnim_ = currAnim_;
    prevFrame_ = currFrame_;

    currAnim_ = a;
    currFrame_ = 0.f;
    blend_ = 0.f;

    // родная скорость клипа (ед/с) с учётом масштаба модели
    const float s = transform_.scale.x;
    clipSpeed_ =
        TextIsEqual(anims_[a].name, "Walk_Loop")    ? 1.46f * s :
        TextIsEqual(anims_[a].name, "Jog_Fwd_Loop") ? 8.04f * s :
        TextIsEqual(anims_[a].name, "Sprint_Loop")  ? 12.37f * s : 0.f;
}

void game::Player::setMoveDir(Vector2 d) {
    moveDir_ = d;
}

void game::Player::setMoveMode(MoveMode m) {
    mode_ = m;
    switch (m) {
        case MoveMode::Jog:    speed_ = kJogSpeed;    break;
        case MoveMode::Sprint: speed_ = kSprintSpeed; break;
        default:               speed_ = kWalkSpeed;   break;
    }
}

void game::Player::updateFacing(Vector2 cursor, bool hasCursor) {
    // движение: смотрит в сторону движения; стоя: следит за курсором
    if (isMoving()) {
        facing_ = velocityDir();
    } else if (hasCursor) {
        const Vector2 player2d {transform_.position.x, transform_.position.z};
        const Vector2 toCursor = Vector2Subtract(cursor, player2d);
        if (Vector2LengthSqr(toCursor) > 0.0001f) {
            facing_ = Vector2Normalize(toCursor);
        }
    }
}

void game::Player::update(float dt, const world::World& world) {
    prevTransform_ = transform_;

    const bool hasInput = Vector2LengthSqr(moveDir_) > 0.0f;
    moveDir_ = Vector2Normalize(moveDir_);

    if (hasInput) {
        const Vector2 desired = moveDir_ * speed_;
        const Vector2 difference = desired - velocity_;
        velocity_ = velocity_ + difference * kAcceleration;
    } else {
        velocity_ = velocity_ - velocity_ * kFriction;
    }

    if (Vector2LengthSqr(velocity_) < kStopEpsilon * kStopEpsilon) {
        velocity_ = {0.0f, 0.0f};
    }
    // else {
    //     transform_.position.x = transform_.position.x + velocity_.x * dt;
    //     transform_.position.z = transform_.position.z + velocity_.y * dt;
    // }
    //

    // world boxes collider check
    // collision_.update(Vector3Add(transform_.position, {0.f, 1.f, 0.f}), colliderSize_);

    Vector3 pos = transform_.position;

    // X
    Vector3 tryX = pos;
    tryX.x += velocity_.x * dt;
    collision_.update(tryX + colliderOffset_, colliderSize_);
    if (!world.overlaps(collision_)) pos.x = tryX.x;
    else velocity_.x = 0.f;

    // Z
    Vector3 tryZ = pos;
    tryZ.z += velocity_.y * dt;
    collision_.update(tryZ + colliderOffset_, colliderSize_);
    if (!world.overlaps(collision_)) pos.z = tryZ.z;
    else velocity_.y = 0.f;

    transform_.position = pos;
    collision_.update(pos + colliderOffset_, colliderSize_);

    const float spd = Vector2Length(velocity_);
    int target = findAnim("Idle_Loop");
    if      (spd > 7.0f) target = findAnim("Sprint_Loop");
    else if (spd > 3.5f) target = findAnim("Jog_Fwd_Loop");
    else if (spd > 0.3f) target = findAnim("Walk_Loop");
    play(target);

}

void game::Player::animate(float dt) {
    if (!anims_ || animCount_ <= 0 || currAnim_ < 0 || currAnim_ >= animCount_) return;
    constexpr float kFps = 30.f;

    const int   currKf   = anims_[currAnim_].keyframeCount;
    if (currKf <= 0) return;
    const float currLast = static_cast<float>(currKf - 1);

    // ноги проходят ровно столько пути, сколько едет тело:
    // множитель = реальная скорость / родная скорость клипа
    const float spd = Vector2Length(velocity_);
    const float ratio = clipSpeed_ > 0.f ? spd / clipSpeed_ : 0.f;

    // не более 1.5 ключа за кадр - иначе клип "плывёт" между позами
    float step = kFps * ratio * animTrim_ * dt;
    step = std::min(step, 1.5f);

    currFrame_ += step;
    while (currFrame_ >= currLast) currFrame_ -= currLast;

    if (blend_ < 1.f) {
        if (prevAnim_ < 0 || prevAnim_ >= animCount_) {
            blend_ = 1.f;
        } else {
            blend_ = std::min(1.f, blend_ + dt / kBlendTime);

            const int   prevKf   = anims_[prevAnim_].keyframeCount;
            if (prevKf > 0) {
                const float prevLast = static_cast<float>(prevKf - 1);

                prevFrame_ += step;
                while (prevFrame_ >= prevLast) prevFrame_ -= prevLast;

                UpdateModelAnimationEx(model_, anims_[prevAnim_], prevFrame_,
                                               anims_[currAnim_], currFrame_, blend_);
                return;
            }
        }
    }

    UpdateModelAnimation(model_, anims_[currAnim_], currFrame_);
}
