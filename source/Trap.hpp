#ifndef GAME_TRAP_HPP
#define GAME_TRAP_HPP

#include "Collision2D.hpp"
#include "Global.hpp"
#include "Vector2D.hpp"

struct Player;

/**
 * @file Trap.hpp
 * @brief Hidden carnivorous plant trap placed on top of grass tiles.
 */
enum class TrapState {
    Idle,
    Attacking,
    Cooldown
};

struct Trap {
    static constexpr int   ATK_FRAME_COUNT = 9;
    static constexpr float FRAME_DURATION  = 0.07f;
    static constexpr float COOLDOWN_TIME   = 1.2f;
    static constexpr float DAMAGE          = 20.0f;

    Vector2D  position; ///< Center of the 32x32 tile just above the grass block.
    TrapState state{TrapState::Idle};
    int       animFrame{0};
    float     animTimer{0.0f};
    float     cooldownTimer{0.0f};

    [[nodiscard]] AABB bounds() const noexcept {
        const Vector2D half{ TILE_SIZE * 0.35f, TILE_SIZE * 0.40f };
        return {
            { position.x - half.x, position.y + TILE_SIZE * 0.5f - half.y * 2.0f },
            { position.x + half.x, position.y + TILE_SIZE * 0.5f }
        };
    }

    void update(float dt, Player& player);
};

#endif // GAME_TRAP_HPP
