#ifndef GAME_ENEMY_HPP
#define GAME_ENEMY_HPP

#include "Entity.hpp"
#include "Global.hpp"

class TileMap;
struct Player;

/**
 * @file Enemy.hpp
 * @brief Hopping slime enemy that patrols platforms without jumping into pits,
 *        knocks back the player on hit, and can collide harmlessly with other slimes.
 */
struct Enemy : Entity {
    static constexpr float DEFAULT_SPEED  = 135.0f;
    static constexpr float HOP_VELOCITY   = -260.0f;
    static constexpr float HOP_INTERVAL   = 1.6f;
    static constexpr float CONTACT_DAMAGE = 20.0f;
    static constexpr int   STOMP_SCORE    = 100;
    static constexpr int   FRAME_COUNT    = 6;
    static constexpr float FRAME_DURATION = 0.10f;

    float speed{DEFAULT_SPEED};
    float direction{-1.0f}; ///< -1.0f = left, +1.0f = right.
    bool  onGround{false};
    float hopTimer{0.35f};
    int   animFrame{0};
    float animTimer{0.0f};

    Enemy();

    /**
     * @brief Advances slime hop physics, pit-safe turning, animation and player collision.
     */
    void update(float dt, const TileMap& tileMap, Player& player);

private:
    [[nodiscard]] bool hasSolidInCells(const TileMap& tileMap, int minCol, int minRow, int maxCol, int maxRow) const;
    [[nodiscard]] bool hasSafeLandingAhead(const TileMap& tileMap, float dir) const;
};

#endif // GAME_ENEMY_HPP
