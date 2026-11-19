#include "Enemy.hpp"
#include "Player.hpp"
#include "TileMap.hpp"
#include <algorithm>
#include <cmath>

Enemy::Enemy() {
    collision.halfExtents = { TILE_SIZE * 0.36f, TILE_SIZE * 0.34f };
}

bool Enemy::hasSolidInCells(const TileMap& tileMap, int minCol, int minRow, int maxCol, int maxRow) const {
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

bool Enemy::hasSafeLandingAhead(const TileMap& tileMap, float dir) const {
    const float feetY = body.position.y + collision.halfExtents.y;
    const float probe1X = body.position.x + dir * (collision.halfExtents.x + 6.0f);
    const float probe2X = body.position.x + dir * (collision.halfExtents.x + TILE_SIZE * 1.1f);

    int curCol = 0, curRow = 0;
    tileMap.toIndices({ body.position.x, feetY + 6.0f }, curCol, curRow);

    auto hasColumnFloor = [&](float worldX) -> bool {
        int col = 0, dummyRow = 0;
        tileMap.toIndices({ worldX, feetY + 6.0f }, col, dummyRow);
        if (col <= 0 || col >= tileMap.width()) {
            return false;
        }
        // Check current floor row or 1 tile below so it never jumps into a pit.
        for (int r = curRow; r <= std::min(tileMap.height() - 1, curRow + 1); ++r) {
            if (tileMap.isSolid(col, r)) {
                return true;
            }
        }
        return false;
    };

    return hasColumnFloor(probe1X) && hasColumnFloor(probe2X);
}

void Enemy::update(float dt, const TileMap& tileMap, Player& player) {
    if (!active || !player.active || dt <= 0.0f) {
        return;
    }

    if (onGround) {
        body.velocity.x = 0.0f;
        animFrame = 0;
        animTimer = 0.0f;

        hopTimer -= dt;
        if (hopTimer <= 0.0f) {
            hopTimer = HOP_INTERVAL;
            if (!hasSafeLandingAhead(tileMap, direction)) {
                direction = -direction;
            }
            if (hasSafeLandingAhead(tileMap, direction)) {
                body.velocity.y = HOP_VELOCITY;
                body.velocity.x = direction * speed;
                onGround = false;
            }
        }
    } else {
        body.velocity.x = direction * speed;
    }

    body.velocity.y = std::min(MAX_FALL_SPEED, body.velocity.y + GRAVITY * dt);

    constexpr float SKIN = 0.01f;

    // 1. Horizontal patrol and wall bounce.
    body.position.x += body.velocity.x * dt;
    {
        const AABB box = bounds();
        int minCol = 0, minRow = 0, maxCol = 0, maxRow = 0;
        tileMap.toIndices({ box.min.x + SKIN, box.min.y + SKIN }, minCol, minRow);
        tileMap.toIndices({ box.max.x - SKIN, box.max.y - SKIN }, maxCol, maxRow);

        if (direction > 0.0f && hasSolidInCells(tileMap, maxCol, minRow, maxCol, maxRow)) {
            body.position.x = static_cast<float>(maxCol) * TILE_SIZE - collision.halfExtents.x;
            direction = -1.0f;
        } else if (direction < 0.0f && hasSolidInCells(tileMap, minCol, minRow, minCol, maxRow)) {
            body.position.x = static_cast<float>(minCol + 1) * TILE_SIZE + collision.halfExtents.x;
            direction = 1.0f;
        }
    }

    // 2. Vertical gravity and ground/ceiling resolution.
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
            body.velocity.x = 0.0f;
            onGround = true;
            animFrame = 0;
            animTimer = 0.0f;
        } else if (body.velocity.y < 0.0f && hasSolidInCells(tileMap, minCol, minRow, maxCol, minRow)) {
            body.position.y = static_cast<float>(minRow + 1) * TILE_SIZE + collision.halfExtents.y;
            body.velocity.y = 0.0f;
        }
    }

    // Advance hopping animation only while airborne.
    if (!onGround) {
        animTimer += dt;
        while (animTimer >= FRAME_DURATION) {
            animTimer -= FRAME_DURATION;
            animFrame = (animFrame + 1) % FRAME_COUNT;
        }
    }

    // 3. Player collision: stomp from above or deal damage + knockback.
    const AABB enemyBox = bounds();
    const AABB playerBox = player.bounds();
    if (enemyBox.intersects(playerBox)) {
        const bool stomped = (player.body.velocity.y > 0.0f) &&
                             (playerBox.max.y - player.body.velocity.y * dt <= body.position.y);
        if (stomped) {
            active = false;
            player.body.velocity.y = Player::STOMP_BOUNCE_VELOCITY;
            player.canAirDash = true;
            player.airDashesRemaining = player.maxAirDashes;
            player.jumpsRemaining = std::max(player.jumpsRemaining, 1);
            player.score += STOMP_SCORE;
        } else if (player.invulnTimer <= 0.0f) {
        } else if (player.invulnTimer <= 0.0f && !player.isIntangible()) {
            const float kbDir = (player.body.position.x >= body.position.x) ? 1.0f : -1.0f;
            player.takeDamage(CONTACT_DAMAGE);
            player.applyKnockback(kbDir);
        }
    }
}
