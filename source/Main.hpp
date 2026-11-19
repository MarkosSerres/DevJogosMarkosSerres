#ifndef GAME_MAIN_HPP
#define GAME_MAIN_HPP

#include <array>
#include <optional>
#include <vector>
#include <SFML/Audio.hpp>
#include "Enemy.hpp"
#include "Global.hpp"
#include "HealthPotion.hpp"
#include "Menu.hpp"
#include "Mushroom.hpp"
#include "Player.hpp"
#include "Renderer.hpp"
#include "TileMap.hpp"
#include "Trap.hpp"

/**
 * @file Main.hpp
 * @brief Application entry point and top-level game loop orchestrator.
 *
 * Main owns the core world state (tile map, player, patrolling enemies, menu
 * and renderer) and drives the lifecycle of a platformer session.
 */
enum class AppState {
    Menu,
    Playing,
    Paused
};

enum class FadePhase {
    None,
    FadeOut,
    FadeIn
};

enum class TransitionTarget {
    None,
    StartNewGame,
    NextLevel,
    ReturnToMenu
};

enum class SfxId {
    Collect = 0,
    Victory,
    Damage,
    Defeat,
    Trap,
    MushroomAttack,
    SlimeJump,
    SlimeDeath,
    Count
};

class Main {
public:
    explicit Main(sf::RenderWindow& win);

    // --- Lifecycle ---------------------------------------------------------

    [[nodiscard]] bool init();
    int run();
    void update(float dt);
    void render();
    void shutdown() noexcept;

    // --- Queries -----------------------------------------------------------

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] const TileMap& tileMap() const noexcept;
    [[nodiscard]] const Player& player() const noexcept;

private:
    void startNewGame();
    void startNextLevel();
    void startTransition(TransitionTarget target);
    void playSfx(SfxId id);
    void loadRecord();
    void saveRecord() const;
    void returnToMainMenu();
    void restartMusic();
    void applyAudioVolume();
    void spawnEnemies(const std::vector<Vector2D>& spawns);
    void spawnTraps();
    [[nodiscard]] float checkHorizontalInput() const;
    [[nodiscard]] bool  checkJumpInput() const;

    TileMap                 m_tileMap;
    Player                  m_player;
    std::vector<Enemy>      m_enemies;
    std::vector<Mushroom>     m_mushrooms;
    std::vector<Projectile>   m_projectiles;
    std::vector<HealthPotion> m_potions;
    std::vector<Trap>         m_traps;
    EntityId           m_nextEnemyId{INITIAL_ENTITY_ID};
    bool               m_running{false};
    Renderer           m_renderer;
    Menu               m_menu;
    std::array<sf::SoundBuffer, static_cast<std::size_t>(SfxId::Count)>           m_sfxBuffers;
    std::array<std::optional<sf::Sound>, static_cast<std::size_t>(SfxId::Count)> m_sfxSounds;
    sf::Music          m_music;
    bool               m_musicLoaded{false};
    int                m_bestCoins{0};
    int                m_round{1};
    FadePhase          m_fadePhase{FadePhase::None};
    TransitionTarget   m_fadeTarget{TransitionTarget::None};
    float              m_fadeAlpha{0.0f};
    AppState           m_state{AppState::Menu};
};

#endif // GAME_MAIN_HPP
