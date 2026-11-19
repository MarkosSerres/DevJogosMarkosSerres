#ifndef GAME_RENDERER_HPP
#define GAME_RENDERER_HPP

#include <array>
#include <vector>
#include <SFML/Graphics.hpp>

#include "Enemy.hpp"
#include "HealthPotion.hpp"
#include "Menu.hpp"
#include "Mushroom.hpp"
#include "Player.hpp"
#include "TileMap.hpp"
#include "Trap.hpp"

/**
 * @file Renderer.hpp
 * @brief SFML rendering layer for menus, side-scrolling platformer world and HUD.
 */
class Renderer {
public:
    explicit Renderer(sf::RenderWindow& win);

    bool loadResources();
    void setupMapView(float zoomFactor = 1.0f);
    void updateCamera(const Player& player, const TileMap& tileMap);

    sf::RenderWindow& getWindow();
    [[nodiscard]] const sf::View& getUiView() const noexcept;
    [[nodiscard]] bool windowHasFocus() const;

    void beginFrame(sf::Color clearColor = sf::Color(92, 148, 252));
    void updateBackground(float dt) noexcept;
    void drawBackground();
    void drawMenuBackground();
    void drawMap(const TileMap& tileMap);
    void drawEnemies(const std::vector<Enemy>& enemies);
    void drawMushrooms(const std::vector<Mushroom>& mushrooms);
    void drawProjectiles(const std::vector<Projectile>& projectiles);
    void drawPotions(const std::vector<HealthPotion>& potions);
    void drawTraps(const std::vector<Trap>& traps);
    void drawPlayer(const Player& player);
    void drawHud(const Player& player, int bestCoins, int round);
    void drawFade(float alpha);

    void drawMenu(const Menu& menu);
    sf::FloatRect getMenuButtonBounds(const Menu& menu, std::size_t index) const;
    void endFrame();

private:
    [[nodiscard]] const sf::Texture& menuButtonTexture(const Menu& menu, std::size_t index) const;
    [[nodiscard]] sf::Vector2f menuButtonPosition(const Menu& menu, std::size_t index) const;
    [[nodiscard]] sf::FloatRect volumeControlBounds() const;
    void drawVolumeControl(const Menu& menu);
    void drawSpriteButton(const sf::Texture& tex, sf::Vector2f pos, const sf::IntRect* rect = nullptr, float scale = 1.0f);
    void drawParallaxLayer(const sf::Texture& tex, float parallaxFactor, float driftSpeedPx);
    void drawAnimatedPoint(sf::Vector2f centerPos, float scale);

    sf::RenderWindow& m_window;
    sf::View m_mapView;
    sf::View m_uiView;

    struct AnimPair {
        std::vector<sf::Texture> east;
        std::vector<sf::Texture> west;
    };

    AnimPair m_walkAnim;
    AnimPair m_runAnim;
    AnimPair m_jumpAnim;

    sf::Texture m_groundTex;
    sf::Texture m_coinTex;
    sf::Texture m_slimeRightTex;
    sf::Texture m_slimeLeftTex;
    sf::Texture m_mushroomTex;
    sf::Texture m_potionTex;
    sf::Texture m_trapBlockTex;
    sf::Texture m_trapAtkTex;
    sf::Texture m_bgLayer1Tex;
    sf::Texture m_bgLayer2Tex;
    sf::Texture m_bgLayer3Tex;
    std::array<sf::Texture, 5> m_menuBgLayers;
    float       m_bgTime{0.0f};
    std::array<sf::Texture, static_cast<std::size_t>(MainOption::Count)> m_mainBtnTex;
    std::array<sf::IntRect, static_cast<std::size_t>(MainOption::Count)> m_mainBtnRect;
    sf::Texture m_backBtnTex;
    sf::Font m_font;
};

#endif // GAME_RENDERER_HPP
