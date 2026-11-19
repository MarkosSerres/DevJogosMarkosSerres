#ifndef GAME_PLAYER_HPP
#define GAME_PLAYER_HPP

#include "Entity.hpp"
#include "Global.hpp"

class TileMap;

/**
 * @file Player.hpp
 * @brief Playable 2D platformer character with gravity, jumping, sprinting and tile collisions.
 */
enum class PlayerAnimState {
    Idle,
    Walking,
    Running,
    Jumping
};

struct Player : Entity {
    static constexpr float DEFAULT_SPEED         = 180.0f;
    static constexpr float SPRINT_MULTIPLIER     = 1.45f;
    static constexpr float JUMP_VELOCITY         = -540.0f;
    static constexpr float STOMP_BOUNCE_VELOCITY = -340.0f;
    static constexpr float INITIAL_HP            = 100.0f;
    static constexpr float INITIAL_STAMINA       = 100.0f;
    static constexpr float STAMINA_DRAIN_RATE    = 35.0f;
    static constexpr float STAMINA_REGEN_RATE    = 25.0f;
    static constexpr float AIR_DASH_SPEED        = 520.0f;
    static constexpr float AIR_DASH_DURATION     = 0.16f;
    static constexpr float AIR_DASH_STAMINA_COST = 25.0f;
    static constexpr float INVULN_DURATION       = 1.0f;
    static constexpr float GLOW_DURATION         = 1.5f;
    static constexpr int   STARS_PER_POWERUP     = 30;
    static constexpr int   WALK_FRAME_COUNT      = 8;
    static constexpr int   RUN_FRAME_COUNT       = 4;
    static constexpr int   JUMP_FRAME_COUNT      = 9;
    static constexpr float WALK_FRAME_DURATION   = 0.09f;
    static constexpr float RUN_FRAME_DURATION    = 0.08f;
    static constexpr float JUMP_FRAME_DURATION   = 0.07f;

    float speed{DEFAULT_SPEED};
    float hp{INITIAL_HP};
    float maxHp{INITIAL_HP};
    float stamina{INITIAL_STAMINA};
    float maxStamina{INITIAL_STAMINA};
    float invulnTimer{0.0f};

    int             coins{0};
    int             score{0};
    int             powerUpCount{0};
    int             maxJumps{1};
    int             jumpsRemaining{1};
    int             maxAirDashes{1};
    int             airDashesRemaining{1};
    int             extraLives{0};
    bool            dashIntangible{false};
    bool            slowFall{false};
    bool            onGround{false};
    bool            facingRight{true};
    bool            reachedGoal{false};
    bool            canAirDash{true};
    bool            prevJumpHeld{false};
    bool            prevSprintHeld{false};
    float           dashTimer{0.0f};
    float           dashDirection{1.0f};
    float           knockbackTimer{0.0f};
    float           glowTimer{0.0f};
    PlayerAnimState animState{PlayerAnimState::Idle};
    int             animFrame{0};
    float           animTimer{0.0f};

    Player();

    /**
     * @brief Applies horizontal input, jump, gravity and resolves solid tile collisions.
     * @param moveX     Horizontal input in [-1, 1].
     * @param jumpHeld  True when jump was pressed and the player can jump.
     * @param sprinting True when the sprint key is held.
     * @param dt        Elapsed timestep in seconds.
     * @param tileMap   Level tile map (also mutates collected COIN_TILE cells).
     */
    void update(float moveX, bool jumpPressed, bool sprinting, float dt, TileMap& tileMap);

    /// Applies damage if not currently invulnerable.
    /// Applies damage if not currently invulnerable or dash-intangible.
    void takeDamage(float amount) noexcept;

    /// Pushes the player away horizontally and slightly upward.
    void applyKnockback(float dirX, float horizontalSpeed = 280.0f, float verticalSpeed = -260.0f) noexcept;

    /// Resets all run-scoped power-ups and stats for a fresh run.
    void resetRunStats() noexcept;

    /// True while dash intangibility is active.
    [[nodiscard]] bool isIntangible() const noexcept;

private:
    [[nodiscard]] bool hasSolidInCells(const TileMap& tileMap, int minCol, int minRow, int maxCol, int maxRow) const;
    void checkCollectiblesAndGoal(TileMap& tileMap);
};

#endif // GAME_PLAYER_HPP
