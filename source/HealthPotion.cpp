#include "HealthPotion.hpp"
#include "Collision2D.hpp"
#include "Player.hpp"
#include "TileMap.hpp"
#include <algorithm>
#include <cmath>

void HealthPotion::update(float dt, const TileMap& tileMap, Player& player) {
    if (!active || dt <= 0.0f) {
        return;
    }

    if (pickupDelay > 0.0f) {
        pickupDelay = std::max(0.0f, pickupDelay - dt);
    }

    if (!settled) {
        velocity.y = std::min(MAX_FALL_SPEED, velocity.y + GRAVITY * dt);

        constexpr float SKIN = 0.01f;

        // Horizontal motion + wall bounce.
        position.x += velocity.x * dt;
        {
            int minCol = 0, minRow = 0, maxCol = 0, maxRow = 0;
            tileMap.toIndices({ position.x - halfExtents.x + SKIN, position.y - halfExtents.y + SKIN }, minCol, minRow);
            tileMap.toIndices({ position.x + halfExtents.x - SKIN, position.y + halfExtents.y - SKIN }, maxCol, maxRow);

            auto solidRange = [&](int col) {
                for (int r = minRow; r <= maxRow; ++r) {
                    if (col < 0 || col >= tileMap.width() || tileMap.isSolid(col, r)) {
                        return true;
                    }
                }
                return false;
            };

            if (velocity.x > 0.0f && solidRange(maxCol)) {
                position.x = static_cast<float>(maxCol) * TILE_SIZE - halfExtents.x;
                velocity.x = -velocity.x * 0.5f;
            } else if (velocity.x < 0.0f && solidRange(minCol)) {
                position.x = static_cast<float>(minCol + 1) * TILE_SIZE + halfExtents.x;
                velocity.x = -velocity.x * 0.5f;
            }
        }

        // Vertical motion + ground bounce.
        position.y += velocity.y * dt;
        {
            int minCol = 0, minRow = 0, maxCol = 0, maxRow = 0;
            tileMap.toIndices({ position.x - halfExtents.x + SKIN, position.y - halfExtents.y + SKIN }, minCol, minRow);
            tileMap.toIndices({ position.x + halfExtents.x - SKIN, position.y + halfExtents.y - SKIN }, maxCol, maxRow);

            auto solidRow = [&](int row) {
                for (int c = minCol; c <= maxCol; ++c) {
                    if (tileMap.isSolid(c, row)) {
                        return true;
                    }
                }
                return false;
            };

            if (velocity.y > 0.0f && solidRow(maxRow)) {
                position.y = static_cast<float>(maxRow) * TILE_SIZE - halfExtents.y;
                if (velocity.y > MIN_BOUNCE_SPEED) {
                    velocity.y = -velocity.y * BOUNCE_DAMPING;
                    velocity.x *= 0.6f;
                } else {
                    velocity = { 0.0f, 0.0f };
                    settled = true;
                }
            } else if (velocity.y < 0.0f && solidRow(minRow)) {
                position.y = static_cast<float>(minRow + 1) * TILE_SIZE + halfExtents.y;
                velocity.y = 0.0f;
            }
        }

        if (position.y > static_cast<float>(tileMap.height()) * TILE_SIZE + TILE_SIZE) {
            active = false;
            return;
        }
    }

    const AABB potionBox{
        { position.x - halfExtents.x, position.y - halfExtents.y },
        { position.x + halfExtents.x, position.y + halfExtents.y }
    };
    if (pickupDelay <= 0.0f && player.active && potionBox.intersects(player.bounds())) {
        player.hp = std::min(player.maxHp, player.hp + HEAL_AMOUNT);
        active = false;
    }
}
