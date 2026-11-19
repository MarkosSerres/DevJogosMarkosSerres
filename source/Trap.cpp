#include "Trap.hpp"
#include "Player.hpp"
#include <algorithm>

void Trap::update(float dt, Player& player) {
    if (dt <= 0.0f || !player.active) {
        return;
    }

    switch (state) {
        case TrapState::Idle:
            if (bounds().intersects(player.bounds())) {
                state = TrapState::Attacking;
                animFrame = 0;
                animTimer = 0.0f;

                if (player.invulnTimer <= 0.0f) {
                    const float kbDir = (player.body.position.x >= position.x) ? 1.0f : -1.0f;
                    player.takeDamage(DAMAGE);
                    player.applyKnockback(kbDir, 200.0f, -250.0f);
                }
            }
            break;

        case TrapState::Attacking:
            animTimer += dt;
            while (animTimer >= FRAME_DURATION) {
                animTimer -= FRAME_DURATION;
                if (animFrame < ATK_FRAME_COUNT - 1) {
                    ++animFrame;
                } else {
                    state = TrapState::Cooldown;
                    cooldownTimer = COOLDOWN_TIME;
                    animFrame = 0;
                    break;
                }
            }
            break;

        case TrapState::Cooldown:
            cooldownTimer -= dt;
            if (cooldownTimer <= 0.0f) {
                state = TrapState::Idle;
                cooldownTimer = 0.0f;
            }
            break;
    }
}
