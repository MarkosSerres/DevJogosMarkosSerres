#include "Mushroom.hpp"
#include "Player.hpp"
#include "TileMap.hpp"
#include <algorithm>
#include <cmath>

void Projectile::update(float dt, const TileMap& tileMap, Player& player) {
    if (!active || dt <= 0.0f) {
        return;
    }

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    int col = 0;
    int row = 0;
    tileMap.toIndices(position, col, row);
    if (col < 0 || col >= tileMap.width() || row < 0 || row >= tileMap.height() ||
        tileMap.isSolid(col, row)) {
        active = false;
        return;
    }

    const AABB projBox{
        { position.x - radius, position.y - radius },
        { position.x + radius, position.y + radius }
    };
    if (player.active && projBox.intersects(player.bounds())) {
        if (player.invulnTimer <= 0.0f) {
            const float kbDir = (velocity.x >= 0.0f) ? 1.0f : -1.0f;
            player.takeDamage(DAMAGE);
            player.applyKnockback(kbDir, 220.0f, -200.0f);
        }
        active = false;
    }
}

Mushroom::Mushroom() {
    collision.halfExtents = { TILE_SIZE * 0.36f, TILE_SIZE * 0.44f };
}

void Mushroom::update(float dt, Player& player, std::vector<Projectile>& projectiles) {
    if (!active || !player.active || dt <= 0.0f) {
        return;
    }

    const float dx = player.body.position.x - body.position.x;
    const float dy = player.body.position.y - body.position.y;
    const float distSq = dx * dx + dy * dy;

    const bool playerClose = distSq <= (HIDE_RADIUS * HIDE_RADIUS);
    const bool playerInAttackRange = !playerClose && (distSq <= ATTACK_RADIUS * ATTACK_RADIUS);

    if (state != MushroomState::Hidden && state != MushroomState::Hiding) {
        facingRight = (dx >= 0.0f);
    }

    // Stomp check when mushroom is above ground.
    if (state == MushroomState::Emerging || state == MushroomState::Attacking) {
        const AABB mBox = bounds();
        const AABB pBox = player.bounds();
        if (mBox.intersects(pBox)) {
            const bool stomped = (player.body.velocity.y > 0.0f) &&
                                 (pBox.max.y - player.body.velocity.y * dt <= body.position.y);
            if (stomped) {
                active = false;
                player.body.velocity.y = Player::STOMP_BOUNCE_VELOCITY;
                player.canAirDash = true;
                player.airDashesRemaining = player.maxAirDashes;
                player.jumpsRemaining = std::max(player.jumpsRemaining, 1);
                player.score += STOMP_SCORE;
                return;
            }
        }
    }

    switch (state) {
        case MushroomState::Hidden:
            animFrame = HIDE_START_FRAME + HIDE_FRAME_COUNT - 1;
            if (playerInAttackRange) {
                state = MushroomState::Emerging;
                animFrame = EMERGE_START_FRAME;
                animTimer = 0.0f;
                facingRight = (dx >= 0.0f);
            }
            break;

        case MushroomState::Emerging:
            animTimer += dt;
            while (animTimer >= FRAME_DURATION) {
                animTimer -= FRAME_DURATION;
                if (animFrame < EMERGE_START_FRAME + EMERGE_FRAME_COUNT - 1) {
                    ++animFrame;
                } else {
                    if (playerClose) {
                        state = MushroomState::Hiding;
                        animFrame = HIDE_START_FRAME;
                    } else {
                        state = MushroomState::Attacking;
                        animFrame = ATTACK_START_FRAME;
                        shotFiredThisCycle = false;
                    }
                    break;
                }
            }
            break;

        case MushroomState::Attacking:
            if (playerClose || !playerInAttackRange) {
                state = MushroomState::Hiding;
                animFrame = HIDE_START_FRAME;
                animTimer = 0.0f;
                break;
            }

            animTimer += dt;
            while (animTimer >= FRAME_DURATION) {
                animTimer -= FRAME_DURATION;
                if (animFrame < ATTACK_START_FRAME + ATTACK_FRAME_COUNT - 1) {
                    ++animFrame;
                } else {
                    animFrame = ATTACK_START_FRAME;
                    shotFiredThisCycle = false;
                }

                // Fire projectile at middle height on the 6th attack frame (index 13).
                if (animFrame == ATTACK_FIRE_FRAME && !shotFiredThisCycle) {
                    shotFiredThisCycle = true;
                    const float len = std::sqrt(distSq);
                    Vector2D dir{ facingRight ? 1.0f : -1.0f, 0.0f };
                    if (len > EPSILON) {
                        dir = { dx / len, dy / len };
                    }
                    Projectile proj;
                    proj.position = {
                        body.position.x + (facingRight ? 10.0f : -10.0f),
                        body.position.y
                    };
                    proj.velocity = { dir.x * PROJECTILE_SPEED, dir.y * PROJECTILE_SPEED };
                    projectiles.push_back(proj);
                }
            }
            break;

        case MushroomState::Hiding:
            animTimer += dt;
            while (animTimer >= FRAME_DURATION) {
                animTimer -= FRAME_DURATION;
                if (animFrame < HIDE_START_FRAME + HIDE_FRAME_COUNT - 1) {
                    ++animFrame;
                } else {
                    state = MushroomState::Hidden;
                    break;
                }
            }
            break;
    }
}
