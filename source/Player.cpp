#include "Player.hpp"
#include "TileMap.hpp"
#include <algorithm>
#include <cmath>

Player::Player() {
    id = PLAYER_ID;
    collision.halfExtents = { TILE_SIZE * 0.34f, TILE_SIZE * 0.46f };
}

bool Player::hasSolidInCells(const TileMap& tileMap, int minCol, int minRow, int maxCol, int maxRow) const {
    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            if (col < 0 || col >= tileMap.width()) {
                return true;
            }
            if (tileMap.isSolid(col, row)) {
                return true;
            }
        }
    }
    return false;
}

bool Player::isIntangible() const noexcept {
    return dashIntangible && (dashTimer > 0.0f);
}

void Player::resetRunStats() noexcept {
    maxHp = INITIAL_HP;
    hp = maxHp;
    maxStamina = INITIAL_STAMINA;
    stamina = maxStamina;
    coins = 0;
    score = 0;
    powerUpCount = 0;
    maxJumps = 1;
    jumpsRemaining = 1;
    maxAirDashes = 1;
    airDashesRemaining = 1;
    extraLives = 0;
    dashIntangible = false;
    slowFall = false;
    invulnTimer = 0.0f;
    knockbackTimer = 0.0f;
    dashTimer = 0.0f;
    glowTimer = 0.0f;
    prevJumpHeld = false;
    prevSprintHeld = false;
}

void Player::takeDamage(float amount) noexcept {
    if (invulnTimer > 0.0f || amount <= 0.0f) {
    if (invulnTimer > 0.0f || isIntangible() || amount <= 0.0f) {
        return;
    }
    hp = std::max(0.0f, hp - amount);
    invulnTimer = INVULN_DURATION;
}

void Player::applyKnockback(float dirX, float horizontalSpeed, float verticalSpeed) noexcept {
    if (isIntangible()) {
        return;
    }
    const float sign = (dirX >= 0.0f) ? 1.0f : -1.0f;
    body.velocity.x = sign * horizontalSpeed;
    body.velocity.y = verticalSpeed;
    knockbackTimer = 0.22f;
    dashTimer = 0.0f;
    onGround = false;
}

void Player::checkCollectiblesAndGoal(TileMap& tileMap) {
    const AABB box = bounds();
    constexpr float SKIN = 0.05f;
    int minCol = 0, minRow = 0, maxCol = 0, maxRow = 0;
    tileMap.toIndices({ box.min.x + SKIN, box.min.y + SKIN }, minCol, minRow);
    tileMap.toIndices({ box.max.x - SKIN, box.max.y - SKIN }, maxCol, maxRow);

    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            if (col < 0 || col >= tileMap.width() || row < 0 || row >= tileMap.height()) {
                continue;
            }
            const int cell = tileMap.at(col, row);
            if (cell == COIN_TILE) {
                tileMap.setCell(col, row, EMPTY_TILE);
                ++coins;
                score += 10;

                const int targetPowerUps = coins / STARS_PER_POWERUP;
                while (powerUpCount < targetPowerUps && powerUpCount < 8) {
                    switch (powerUpCount) {
                        case 0: // Pulo duplo
                            maxJumps = 2;
                            jumpsRemaining = std::max(jumpsRemaining, 1);
                            break;
                        case 1: // +20 vida
                        case 4: // +20 vida
                            maxHp += 20.0f;
                            hp = std::min(maxHp, hp + 20.0f);
                            break;
                        case 2: // +20 estamina
                            maxStamina += 20.0f;
                            stamina = std::min(maxStamina, stamina + 20.0f);
                            break;
                        case 3: // Intangibilidade durante o dash
                            dashIntangible = true;
                            break;
                        case 5: // Dash duplo
                            maxAirDashes = 2;
                            airDashesRemaining = std::max(airDashesRemaining, 1);
                            break;
                        case 6: // Queda leve
                            slowFall = true;
                            break;
                        case 7: // Vida extra
                            ++extraLives;
                            break;
                    }
                    ++powerUpCount;
                    glowTimer = GLOW_DURATION;
                }
            } else if (cell == GOAL_TILE) {
                reachedGoal = true;
            }
        }
    }
}

void Player::update(float moveX, bool jumpPressed, bool sprinting, float dt, TileMap& tileMap) {
    if (!active || dt <= 0.0f) {
        return;
    }

    if (invulnTimer > 0.0f) {
        invulnTimer = std::max(0.0f, invulnTimer - dt);
    }
    if (knockbackTimer > 0.0f) {
        knockbackTimer = std::max(0.0f, knockbackTimer - dt);
    }
    if (glowTimer > 0.0f) {
        glowTimer = std::max(0.0f, glowTimer - dt);
    }

    const bool isMoving = std::abs(moveX) > EPSILON;
    if (isMoving && dashTimer <= 0.0f && knockbackTimer <= 0.0f) {
        facingRight = (moveX > 0.0f);
    }

    const bool jumpJustPressed = jumpPressed && !prevJumpHeld;
    prevJumpHeld = jumpPressed;

    const bool sprintJustPressed = sprinting && !prevSprintHeld;
    prevSprintHeld = sprinting;

    if (knockbackTimer > 0.0f) {
        body.acceleration = { 0.0f, GRAVITY };
        body.velocity.y = std::min(MAX_FALL_SPEED, body.velocity.y + body.acceleration.y * dt);
    } else if (!onGround && sprintJustPressed && canAirDash && dashTimer <= 0.0f && stamina > 0.0f) {
        canAirDash = false;
    } else if (!onGround && sprintJustPressed && airDashesRemaining > 0 && dashTimer <= 0.0f && stamina > 0.0f) {
        --airDashesRemaining;
        dashTimer = AIR_DASH_DURATION;
        dashDirection = facingRight ? 1.0f : -1.0f;
        stamina = std::max(0.0f, stamina - AIR_DASH_STAMINA_COST);
    }

    bool isSprinting = false;
    if (knockbackTimer > 0.0f) {
        // Let knockback velocity play out without player override.
    } else if (dashTimer > 0.0f) {
        dashTimer = std::max(0.0f, dashTimer - dt);
        body.velocity.x = dashDirection * AIR_DASH_SPEED;
        body.velocity.y = 0.0f;
    } else {
        isSprinting = sprinting && onGround && isMoving && (stamina > 0.0f);
        if (isSprinting) {
            stamina = std::max(0.0f, stamina - STAMINA_DRAIN_RATE * dt);
        } else if (!sprinting || !isMoving) {
            stamina = std::min(maxStamina, stamina + STAMINA_REGEN_RATE * dt);
        }

        const float currentSpeed = isSprinting ? (speed * SPRINT_MULTIPLIER) : speed;
        body.velocity.x = moveX * currentSpeed;

        if (jumpPressed && onGround) {
        if (jumpJustPressed && jumpsRemaining > 0) {
            body.velocity.y = JUMP_VELOCITY;
            onGround = false;
            --jumpsRemaining;
        }

        body.acceleration = { 0.0f, GRAVITY };
        body.velocity.y = std::min(MAX_FALL_SPEED, body.velocity.y + body.acceleration.y * dt);
        const bool gliding = slowFall && !onGround && jumpPressed && (body.velocity.y > 0.0f);
        const float grav = gliding ? (GRAVITY * 0.45f) : GRAVITY;
        const float maxFall = gliding ? (MAX_FALL_SPEED * 0.35f) : MAX_FALL_SPEED;
        body.acceleration = { 0.0f, grav };
        body.velocity.y = std::min(maxFall, body.velocity.y + body.acceleration.y * dt);
    }

    constexpr float SKIN = 0.01f;

    // 1. Horizontal movement and wall resolution.
    if (std::abs(body.velocity.x) > EPSILON) {
        body.position.x += body.velocity.x * dt;
        const AABB box = bounds();
        int minCol = 0, minRow = 0, maxCol = 0, maxRow = 0;
        tileMap.toIndices({ box.min.x + SKIN, box.min.y + SKIN }, minCol, minRow);
        tileMap.toIndices({ box.max.x - SKIN, box.max.y - SKIN }, maxCol, maxRow);

        if (body.velocity.x > 0.0f && hasSolidInCells(tileMap, maxCol, minRow, maxCol, maxRow)) {
            body.position.x = static_cast<float>(maxCol) * TILE_SIZE - collision.halfExtents.x;
            body.velocity.x = 0.0f;
        } else if (body.velocity.x < 0.0f && hasSolidInCells(tileMap, minCol, minRow, minCol, maxRow)) {
            body.position.x = static_cast<float>(minCol + 1) * TILE_SIZE + collision.halfExtents.x;
            body.velocity.x = 0.0f;
        }
    }

    // 2. Vertical movement and ground/ceiling resolution.
    onGround = false;
    body.position.y += body.velocity.y * dt;
    {
        const AABB box = bounds();
        int minCol = 0, minRow = 0, maxCol = 0, maxRow = 0;
        tileMap.toIndices({ box.min.x + SKIN, box.min.y + SKIN }, minCol, minRow);
        tileMap.toIndices({ box.max.x - SKIN, box.max.y - SKIN }, maxCol, maxRow);

        if (body.velocity.y > 0.0f && hasSolidInCells(tileMap, minCol, maxRow, maxCol, maxRow)) {
            body.position.y = static_cast<float>(maxRow) * TILE_SIZE - collision.halfExtents.y;
            body.velocity.y = 0.0f;
            onGround = true;
            canAirDash = true;
            jumpsRemaining = maxJumps;
            airDashesRemaining = maxAirDashes;
        } else if (body.velocity.y < 0.0f && hasSolidInCells(tileMap, minCol, minRow, maxCol, minRow)) {
            body.position.y = static_cast<float>(minRow + 1) * TILE_SIZE + collision.halfExtents.y;
            body.velocity.y = 0.0f;
        }
    }

    // Falling below the bottom of the level kills the player.
    if (body.position.y > static_cast<float>(tileMap.height()) * TILE_SIZE + TILE_SIZE) {
        hp = 0.0f;
        return;
    }

    checkCollectiblesAndGoal(tileMap);

    // 3. Update animation state machine.
    PlayerAnimState nextState = PlayerAnimState::Idle;
    if (!onGround) {
        nextState = PlayerAnimState::Jumping;
    } else if (isSprinting) {
        nextState = PlayerAnimState::Running;
    } else if (isMoving) {
        nextState = PlayerAnimState::Walking;
    }

    if (nextState != animState) {
        animState = nextState;
        animFrame = 0;
        animTimer = 0.0f;
    } else if (animState != PlayerAnimState::Idle) {
        float frameDuration = WALK_FRAME_DURATION;
        int   frameCount    = WALK_FRAME_COUNT;
        bool  loop          = true;

        if (animState == PlayerAnimState::Running) {
            frameDuration = RUN_FRAME_DURATION;
            frameCount    = RUN_FRAME_COUNT;
        } else if (animState == PlayerAnimState::Jumping) {
            frameDuration = JUMP_FRAME_DURATION;
            frameCount    = JUMP_FRAME_COUNT;
            loop          = false;
        }

        animTimer += dt;
        while (animTimer >= frameDuration) {
            animTimer -= frameDuration;
            if (loop) {
                animFrame = (animFrame + 1) % frameCount;
            } else {
                animFrame = std::min(animFrame + 1, frameCount - 1);
            }
        }
    }
}
