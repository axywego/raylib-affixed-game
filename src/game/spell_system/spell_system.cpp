#include "spell_system.hpp"
#include <raylib.h>
#include <raymath.h>
#include <algorithm>

game::Projectile::Projectile(const SpellDef& def, Vector3 spawn, Vector2 dir)
 : def_(def), lifetime_(def.lifetime) {
     transform_.position = spawn;
     prevTransform_.position = spawn;
     facing_ = Vector2Normalize(dir);
}

void game::Projectile::update(float dt, const world::World& world) {
    prevTransform_ = transform_;
    lifetime_ -= dt;

    transform_.position.x = transform_.position.x + facing_.x * dt * def_.speed;
    transform_.position.z = transform_.position.z + facing_.y * dt * def_.speed;

    if(world.blockedBetween(prevTransform_.position, transform_.position))
        lifetime_ = 0.f;
}

void game::Projectile::draw(float alpha) const {
    if(alive())
        DrawSphere(renderPosition(alpha), def_.radius, RED);
}


game::Zone::Zone(const SpellDef& def, Vector3 spawn)
 : def_(def), lifetime_(def.lifetime) {
     transform_.position = spawn;
     prevTransform_.position = spawn;
}

void game::Zone::update(float dt, const world::World& world) {
    prevTransform_ = transform_;
    lifetime_ -= dt;
}

void game::Zone::draw(float alpha) const {
    if(alive())
        DrawSphere(renderPosition(alpha), def_.radius, Fade(ORANGE, 0.4f));
}



void game::SpellSystem::setSlot(int index, const SpellDef *def){
    if (index < 0 || index >= static_cast<int>(slots_.max_size())) return;
    slots_[index].def = def;
    slots_[index].cooldownLeft = 0.f;
}

bool game::SpellSystem::canCast(int slotIndex) const {
     if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.max_size())) return false;
     return slots_[slotIndex].cooldownLeft <= 0.f;
}

void game::SpellSystem::cast(int slotIndex, Vector3 spawn, Vector2 dir) {
    if(!canCast(slotIndex)) return;

    auto& spell = slots_[slotIndex];
    spell.cooldownLeft = spell.def->cooldown;

    activeSpells_.push_back(std::make_unique<Projectile>(*spell.def, spawn, dir));
}

void game::SpellSystem::castAtPoint(int slotIndex, Vector3 spawn) {
    if(!canCast(slotIndex)) return;

    auto& spell = slots_[slotIndex];
    spell.cooldownLeft = spell.def->cooldown;

    activeSpells_.push_back(std::make_unique<Zone>(*spell.def, spawn));
}

float game::SpellSystem::getCooldownLeft(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.max_size())) return 0.f;
    return slots_[slotIndex].cooldownLeft;
}

float game::SpellSystem::getCooldownRatio(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.max_size())) return 0.f;
    const auto& spell = slots_[slotIndex];
    if (spell.def->cooldown) return 0.f;
    return spell.cooldownLeft / spell.def->cooldown;
}

void game::SpellSystem::update(float dt, const world::World& world) {
    for (auto& spell : activeSpells_) spell->update(dt, world);

    std::erase_if(activeSpells_, [](const auto& s) { return !s->alive(); });

    for (auto& slot : slots_) {
        if (slot.cooldownLeft > 0.f) {
            slot.cooldownLeft -= dt;
            if (slot.cooldownLeft < 0.f) slot.cooldownLeft = 0.f;
        }
    }
}

void game::SpellSystem::draw(float alpha) {
    for (auto& spell : activeSpells_) {
        spell->draw(alpha);
    }
}
