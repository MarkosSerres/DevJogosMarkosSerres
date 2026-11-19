#ifndef GAME_HEALTH_POTION_HPP
#define GAME_HEALTH_POTION_HPP

#include "Global.hpp"
#include "Vector2D.hpp"

class TileMap;
struct Player;

/**
 * @file HealthPotion.hpp
 * @brief Collectible health potion dropped by defeated enemies with bounce physics.
 */
struct HealthPotion {
    static constexpr float HEAL_AMOUNT         = 20.0f;
    static constexpr int   DROP_CHANCE_MODULO  = 2; ///< 1 in 2 (50%) chance on enemy defeat.
    static constexpr float INITIAL_UP_VELOCITY = -310.0f;
    static constexpr float BOUNCE_DAMPING      = 0.45f;
    static constexpr float MIN_BOUNCE_SPEED    = 50.0f;
    static constexpr float PICKUP_DELAY        = 0.45f;

    Vector2D position;
    Vector2D velocity;
    Vector2D halfExtents{ 10.0f, 10.0f };
    float    pickupDelay{PICKUP_DELAY};
    bool     active{true};
    bool     settled{false};

    void update(float dt, const TileMap& tileMap, Player& player);
};

#endif // GAME_HEALTH_POTION_HPP
