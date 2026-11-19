#ifndef GAME_MUSHROOM_HPP
#define GAME_MUSHROOM_HPP

#include <vector>
#include "Entity.hpp"
#include "Global.hpp"

class TileMap;
struct Player;

/**
 * @file Mushroom.hpp
 * @brief Stationary mushroom enemy that hides when the player approaches and
 *        emerges to spit projectiles when the player is at medium/long range.
 */

struct Projectile {
    static constexpr float DEFAULT_RADIUS = 5.0f;
    static constexpr float DAMAGE         = 20.0f;

    Vector2D position;
    Vector2D velocity;
    float    radius{DEFAULT_RADIUS};
    bool     active{true};

    void update(float dt, const TileMap& tileMap, Player& player);
};

enum class MushroomState {
    Hidden,
    Emerging,
    Attacking,
    Hiding
};

struct Mushroom : Entity {
    static constexpr int   EMERGE_START_FRAME = 0;
    static constexpr int   EMERGE_FRAME_COUNT = 4;  ///< Frames 0..3
    static constexpr int   HIDE_START_FRAME   = 4;
    static constexpr int   HIDE_FRAME_COUNT   = 4;  ///< Frames 4..7
    static constexpr int   ATTACK_START_FRAME = 8;
    static constexpr int   ATTACK_FRAME_COUNT = 8;  ///< Frames 8..15
    static constexpr int   ATTACK_FIRE_FRAME  = 13; ///< 6th attack frame (8 + 5)
    static constexpr float FRAME_DURATION     = 0.10f;
    static constexpr float HIDE_RADIUS        = TILE_SIZE * 4.0f;
    static constexpr float ATTACK_RADIUS      = TILE_SIZE * 14.0f;
    static constexpr float PROJECTILE_SPEED   = 230.0f;
    static constexpr int   STOMP_SCORE        = 100;

    MushroomState state{MushroomState::Hidden};
    int           animFrame{HIDE_START_FRAME + HIDE_FRAME_COUNT - 1};
    float         animTimer{0.0f};
    bool          facingRight{false};
    bool          shotFiredThisCycle{false};

    Mushroom();

    void update(float dt, Player& player, std::vector<Projectile>& projectiles);
};

#endif // GAME_MUSHROOM_HPP
