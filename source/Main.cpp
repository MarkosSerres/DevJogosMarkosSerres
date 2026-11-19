#include "Main.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>

Main::Main(sf::RenderWindow& win)
    : m_renderer(win) {
    m_player.id = PLAYER_ID;
}

bool Main::init() {
    if (!m_renderer.loadResources()) {
        return false;
    }

    const std::array<const char*, static_cast<std::size_t>(SfxId::Count)> sfxPaths = {
        COLLECT_SFX,
        VICTORY_SFX,
        DAMAGE_SFX,
        DEFEAT_SFX,
        TRAP_SFX,
        MUSHROOM_ATTACK_SFX,
        SLIME_JUMP_SFX,
        SLIME_DEATH_SFX
    };
    for (std::size_t i = 0; i < sfxPaths.size(); ++i) {
        if (m_sfxBuffers[i].loadFromFile(sfxPaths[i])) {
            m_sfxSounds[i].emplace(m_sfxBuffers[i]);
        } else {
            std::cerr << "Warning: could not load sound " << sfxPaths[i] << "\n";
        }
    }

    m_musicLoaded = m_music.openFromFile("musica.ogg") || m_music.openFromFile(GAME_MUSIC);
    if (m_musicLoaded) {
        m_music.setLooping(true);
        applyAudioVolume();
        restartMusic();
    } else {
        std::cerr << "Warning: could not load background music (unsupported format or missing file).\n";
    }

    m_renderer.setupMapView(1.5f);
    loadRecord();
    m_state = AppState::Menu;
    m_running = true;
    return true;
}

void Main::playSfx(SfxId id) {
    const auto idx = static_cast<std::size_t>(id);
    if (m_sfxSounds[idx].has_value()) {
        m_sfxSounds[idx]->setVolume(static_cast<float>(m_menu.volume()));
        m_sfxSounds[idx]->play();
    }
}

void Main::loadRecord() {
    std::ifstream in("record.txt");
    if (in) {
        int val = 0;
        if (in >> val && val >= 0) {
            m_bestCoins = val;
        }
    }
}

void Main::saveRecord() const {
    std::ofstream out("record.txt");
    if (out) {
        out << m_bestCoins << "\n";
    }
}

void Main::applyAudioVolume() {
    const float vol = static_cast<float>(m_menu.volume());
    if (m_musicLoaded) {
        m_music.setVolume(vol);
    }
    for (auto& snd : m_sfxSounds) {
        if (snd.has_value()) {
            snd->setVolume(vol);
        }
    }
}

void Main::startTransition(TransitionTarget target) {
    if (m_fadePhase != FadePhase::None) {
        return;
    }
    m_fadePhase = FadePhase::FadeOut;
    m_fadeTarget = target;
    m_fadeAlpha = 0.0f;
}

void Main::restartMusic() {
    if (!m_musicLoaded) {
        return;
    }
    m_music.stop();
    m_music.play();
}

void Main::returnToMainMenu() {
    m_state = AppState::Menu;
    m_menu.openMain();
    m_renderer.getWindow().setView(m_renderer.getWindow().getDefaultView());
    restartMusic();
}

void Main::startNewGame() {
    m_player.hp = m_player.maxHp;
    m_player.coins = 0;
    m_player.score = 0;
    m_player.resetRunStats();
    m_round = 1;
    startNextLevel();
    restartMusic();
}

void Main::startNextLevel() {
    std::random_device rd;
    const std::vector<Vector2D> enemySpawns = m_tileMap.loadPlatformLevel(rd());

    m_player.body.position = m_tileMap.toWorld(3, m_tileMap.height() - 4);
    m_player.body.velocity = { 0.0f, 0.0f };
    m_player.active = true;
    m_player.stamina = m_player.maxStamina;
    m_player.invulnTimer = 0.0f;
    m_player.onGround = false;
    m_player.reachedGoal = false;

    spawnEnemies(enemySpawns);
    spawnTraps();

    m_renderer.setupMapView(1.5f);
    m_renderer.updateCamera(m_player, m_tileMap);
    m_state = AppState::Playing;
    m_running = true;
}

void Main::spawnEnemies(const std::vector<Vector2D>& spawns) {
    m_enemies.clear();
    m_mushrooms.clear();
    m_projectiles.clear();
    m_potions.clear();
    m_nextEnemyId = INITIAL_ENTITY_ID;

    if (spawns.empty()) {
        return;
    }

    const int totalEnemies = static_cast<int>(spawns.size()) + std::max(0, m_round - 1) * 2;
    for (int idx = 0; idx < totalEnemies; ++idx) {
        const std::size_t spawnIdx = static_cast<std::size_t>(idx) % spawns.size();
        const int wave = idx / static_cast<int>(spawns.size());
        const float offsetX = (wave == 0) ? 0.0f : ((wave % 2 == 1) ? 26.0f : -26.0f);
        const Vector2D pos{ spawns[spawnIdx].x + offsetX, spawns[spawnIdx].y };

        if (idx % 3 == 1) {
            Mushroom mushroom;
            mushroom.id = m_nextEnemyId++;
            mushroom.body.position = {
                pos.x,
                pos.y + (TILE_SIZE * 0.5f - mushroom.collision.halfExtents.y)
            };
            m_mushrooms.push_back(mushroom);
        } else {
            Enemy enemy;
            enemy.id = m_nextEnemyId++;
            enemy.body.position = pos;
            enemy.direction = (idx % 2 == 0) ? -1.0f : 1.0f;
            m_enemies.push_back(enemy);
        }
    }
}

void Main::spawnTraps() {
    m_traps.clear();
    const int minCol = 9;
    const int maxCol = m_tileMap.width() - 15;
    if (maxCol <= minCol) {
        return;
    }

    std::vector<Vector2D> candidates;
    for (int col = minCol; col < maxCol; ++col) {
        for (int row = 1; row < m_tileMap.height() - 1; ++row) {
            if (m_tileMap.at(col, row) == EMPTY_TILE && m_tileMap.isWall(col, row + 1)) {
                candidates.push_back(m_tileMap.toWorld(col, row));
                break;
            }
        }
    }

    constexpr float MIN_TRAP_SPACING_SQ = (TILE_SIZE * 6.0f) * (TILE_SIZE * 6.0f);
    for (const Vector2D& pos : candidates) {
        if (std::rand() % 6 != 0) {
            continue;
        }
        bool tooClose = false;
        for (const Trap& existing : m_traps) {
            const float dx = existing.position.x - pos.x;
            const float dy = existing.position.y - pos.y;
            if (dx * dx + dy * dy < MIN_TRAP_SPACING_SQ) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            Trap trap;
            trap.position = pos;
            m_traps.push_back(trap);
        }
    }
}

int Main::run() {
    if (!init()) {
        std::cerr << "Failed to initialize game.\n";
        return 1;
    }

    sf::Clock clock;

    while (m_renderer.getWindow().isOpen()) {
        while (const std::optional event = m_renderer.getWindow().pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                m_renderer.getWindow().close();
            } else if (m_state == AppState::Menu || m_state == AppState::Paused) {
                if (m_state == AppState::Paused) {
                    if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                        if (key->code == sf::Keyboard::Key::Escape) {
                            if (m_menu.currentScreen() == MenuScreen::PauseOptions) {
                                m_menu.openPause();
                            } else {
                                m_state = AppState::Playing;
                            }
                            continue;
                        }
                    }
                }
                if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouse->button == sf::Mouse::Button::Left) {
                        const MenuAction action = m_menu.handleMenuClick(mouse->position, m_renderer);
                        applyAudioVolume();
                        if (action == MenuAction::StartGame) {
                            startTransition(TransitionTarget::StartNewGame);
                        } else if (action == MenuAction::ResumeGame) {
                            m_state = AppState::Playing;
                        } else if (action == MenuAction::ReturnToMenu) {
                            startTransition(TransitionTarget::ReturnToMenu);
                        } else if (action == MenuAction::ExitGame) {
                            m_renderer.getWindow().close();
                        }
                    }
                }
            } else if (m_state == AppState::Playing) {
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        m_state = AppState::Paused;
                        m_menu.openPause();
                    }
                }
            }
        }

        const float dt = std::min(clock.restart().asSeconds(), FIXED_DT);
        if (dt > 0.0f) {
            update(dt);
        }

        render();
    }

    shutdown();
    return 0;
}

float Main::checkHorizontalInput() const {
    if (!m_renderer.windowHasFocus()) {
        return 0.0f;
    }

    float moveX = 0.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        moveX -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        moveX += 1.0f;
    }
    return moveX;
}

bool Main::checkJumpInput() const {
    if (!m_renderer.windowHasFocus()) {
        return false;
    }
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
}

void Main::update(float dt) {
    if (!m_running) {
        return;
    }

    if (m_fadePhase != FadePhase::None) {
        constexpr float FADE_SPEED = 2.8f;
        if (m_fadePhase == FadePhase::FadeOut) {
            m_fadeAlpha = std::min(1.0f, m_fadeAlpha + FADE_SPEED * dt);
            if (m_fadeAlpha >= 1.0f) {
                switch (m_fadeTarget) {
                    case TransitionTarget::StartNewGame:
                        startNewGame();
                        break;
                    case TransitionTarget::NextLevel:
                        startNextLevel();
                        break;
                    case TransitionTarget::ReturnToMenu:
                        returnToMainMenu();
                        break;
                    case TransitionTarget::None:
                        break;
                }
                m_fadeTarget = TransitionTarget::None;
                m_fadePhase = FadePhase::FadeIn;
            }
        } else {
            m_fadeAlpha = std::max(0.0f, m_fadeAlpha - FADE_SPEED * dt);
            if (m_fadeAlpha <= 0.0f) {
                m_fadePhase = FadePhase::None;
            }
        }
        return;
    }

    if (m_state == AppState::Menu) {
        m_renderer.updateBackground(dt);
        return;
    }
    if (m_state != AppState::Playing) {
        return;
    }

    const bool sprinting = m_renderer.windowHasFocus() &&
                           (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift));

    const int prevCoins = m_player.coins;
    const float prevHp = m_player.hp;
    m_player.update(checkHorizontalInput(), checkJumpInput(), sprinting, dt, m_tileMap);
    if (m_player.coins > prevCoins) {
        playSfx(SfxId::Collect);
        if (m_player.coins > m_bestCoins) {
            m_bestCoins = m_player.coins;
            saveRecord();
        }
    }

    auto tryDropPotion = [this](const Vector2D& pos) {
        if (std::rand() % HealthPotion::DROP_CHANCE_MODULO == 0) {
            HealthPotion potion;
            potion.position = pos;
            potion.velocity = {
                (std::rand() % 2 == 0) ? 55.0f : -55.0f,
                HealthPotion::INITIAL_UP_VELOCITY
            };
            m_potions.push_back(potion);
        }
    };

    for (Enemy& enemy : m_enemies) {
        const bool wasActive = enemy.active;
        const bool wasOnGround = enemy.onGround;
        enemy.update(dt, m_tileMap, m_player);
        if (wasActive && !enemy.active) {
            playSfx(SfxId::SlimeDeath);
            tryDropPotion(enemy.body.position);
        } else if (enemy.active && wasOnGround && !enemy.onGround && enemy.body.velocity.y < 0.0f) {
            playSfx(SfxId::SlimeJump);
        }
    }

    // Slimes can bump into each other and bounce apart without dealing damage.
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        if (!m_enemies[i].active) {
            continue;
        }
        for (std::size_t j = i + 1; j < m_enemies.size(); ++j) {
            if (!m_enemies[j].active) {
                continue;
            }
            const AABB boxA = m_enemies[i].bounds();
            const AABB boxB = m_enemies[j].bounds();
            if (boxA.intersects(boxB)) {
                const float overlapX = std::min(boxA.max.x, boxB.max.x) - std::max(boxA.min.x, boxB.min.x);
                const float push = overlapX * 0.5f + 0.5f;
                if (m_enemies[i].body.position.x <= m_enemies[j].body.position.x) {
                    m_enemies[i].body.position.x -= push;
                    m_enemies[j].body.position.x += push;
                    m_enemies[i].direction = -1.0f;
                    m_enemies[j].direction = 1.0f;
                } else {
                    m_enemies[i].body.position.x += push;
                    m_enemies[j].body.position.x -= push;
                    m_enemies[i].direction = 1.0f;
                    m_enemies[j].direction = -1.0f;
                }
            }
        }
    }

    for (Mushroom& mushroom : m_mushrooms) {
        const bool wasActive = mushroom.active;
        const std::size_t prevProjCount = m_projectiles.size();
        mushroom.update(dt, m_player, m_projectiles);
        if (m_projectiles.size() > prevProjCount) {
            playSfx(SfxId::MushroomAttack);
        }
        if (wasActive && !mushroom.active) {
            tryDropPotion(mushroom.body.position);
        }
    }

    for (HealthPotion& potion : m_potions) {
        potion.update(dt, m_tileMap, m_player);
    }

    for (Trap& trap : m_traps) {
        const TrapState prevState = trap.state;
        trap.update(dt, m_player);
        if (prevState == TrapState::Idle && trap.state == TrapState::Attacking) {
            playSfx(SfxId::Trap);
        }
    }
    m_potions.erase(
        std::remove_if(m_potions.begin(), m_potions.end(),
                       [](const HealthPotion& p) { return !p.active; }),
        m_potions.end()
    );

    for (Projectile& proj : m_projectiles) {
        proj.update(dt, m_tileMap, m_player);
    }
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
                       [](const Projectile& p) { return !p.active; }),
        m_projectiles.end()
    );

    if (m_player.hp < prevHp && m_player.hp > 0.0f) {
        playSfx(SfxId::Damage);
    }

    if (m_player.hp <= 0.0f) {
        playSfx(SfxId::Defeat);
        startTransition(TransitionTarget::ReturnToMenu);
        return;
        if (m_player.extraLives > 0) {
            --m_player.extraLives;
            m_player.hp = m_player.maxHp;
            m_player.invulnTimer = 2.0f;
            m_player.glowTimer = Player::GLOW_DURATION;
            if (m_player.body.position.y > static_cast<float>(m_tileMap.height()) * TILE_SIZE) {
                m_player.body.position = m_tileMap.toWorld(3, m_tileMap.height() - 4);
                m_player.body.velocity = { 0.0f, 0.0f };
            }
        } else {
            playSfx(SfxId::Defeat);
            startTransition(TransitionTarget::ReturnToMenu);
            return;
        }
    }

    if (m_player.reachedGoal) {
        playSfx(SfxId::Victory);
        ++m_round;
        startTransition(TransitionTarget::NextLevel);
        return;
    }

    if (m_player.active) {
        m_renderer.updateCamera(m_player, m_tileMap);
    }
    m_renderer.updateBackground(dt);
}

void Main::render() {
    if (!m_running) {
        return;
    }

    if (m_state == AppState::Menu) {
        m_renderer.beginFrame(sf::Color(35, 40, 55));
        m_renderer.drawMenuBackground();
        m_renderer.drawMenu(m_menu);
    } else {
        m_renderer.beginFrame(sf::Color(92, 148, 252));
        m_renderer.drawBackground();
        m_renderer.drawMap(m_tileMap);
        m_renderer.drawTraps(m_traps);
        m_renderer.drawPotions(m_potions);
        m_renderer.drawMushrooms(m_mushrooms);
        m_renderer.drawEnemies(m_enemies);
        m_renderer.drawProjectiles(m_projectiles);
        m_renderer.drawPlayer(m_player);
        m_renderer.drawHud(m_player, m_bestCoins, m_round);

        if (m_state == AppState::Paused) {
            m_renderer.drawMenu(m_menu);
        }
    }

    m_renderer.drawFade(m_fadeAlpha);
    m_renderer.endFrame();
}

void Main::shutdown() noexcept {
    m_running = false;
}

bool Main::isRunning() const noexcept {
    return m_running;
}

const TileMap& Main::tileMap() const noexcept {
    return m_tileMap;
}

const Player& Main::player() const noexcept {
    return m_player;
}

int main() {
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Plataforma 2D", sf::State::Fullscreen);
    Main app(window);
    return app.run();
}
