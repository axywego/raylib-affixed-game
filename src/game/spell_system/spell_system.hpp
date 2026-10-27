#pragma once

#include "game/entity/entity.hpp"
#include "game/world/world.hpp"
#include <vector>
#include <memory>
#include <string>
#include <array>

namespace game {
    enum class SpellKind {
        Projectile, Beam, Zone
    };

    struct SpellDef {
        std::string name;
        SpellKind kind;
        float manaCost, cooldown, damage, speed, radius, lifetime;
    };

    constexpr SpellDef kFireball {
        "Fireball", SpellKind::Projectile, 5.f, 6.f, 1.f, 13.f, 0.5f, 3.f
    };

    constexpr SpellDef kFirezone {
        "Firezone", SpellKind::Zone, 10.f, 10.f, 3.f, 0.f, 5.f, 5.f
    };

    class Projectile : public entity::Entity {
    public:
        Projectile(const SpellDef& def, Vector3 spawn, Vector2 dir);
        void update(float dt, const world::World& world) override;
        void draw(float alpha) const override;
        bool alive() const override { return lifetime_ > 0.f; }
    private:
        const SpellDef& def_;
        float lifetime_{};
    };

    class Zone : public entity::Entity {
    public:
        Zone(const SpellDef& def, Vector3 spawn);
        void update(float dt, const world::World& world) override;
        void draw(float alpha) const override;
        bool alive() const override { return lifetime_ > 0.f; }
    private:
        const SpellDef& def_;
        float lifetime_{};
    };

    class SpellSystem {
    public:
        SpellSystem() = default;

        void setSlot(int index, const SpellDef* def);
        bool canCast(int slotIndex) const;
        void cast(int slotIndex, Vector3 spawn, Vector2 dir);
        void castAtPoint(int slotIndex, Vector3 spawn);
        float getCooldownLeft(int slotIndex) const;
        float getCooldownRatio(int slotIndex) const;

        void update(float dt, const world::World& world);
        void draw(float alpha);

    private:
        struct Slot {
            const SpellDef* def {nullptr};
            float cooldownLeft {0.f};
        };

        std::array<Slot, 5> slots_;
        std::vector<std::unique_ptr<entity::Entity>> activeSpells_;
    };
}
