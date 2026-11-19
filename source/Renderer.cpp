#include "Renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
constexpr float UI_WIDTH            = 960.0f;
constexpr float UI_HEIGHT           = 540.0f;
constexpr float MENU_START_Y        = 175.0f;
constexpr float MENU_TITLE_Y        = 70.0f;
constexpr float MENU_BUTTON_SPACING = 76.0f;
constexpr float MENU_BTN_SCALE      = 3.0f;
constexpr float CREDIT_LINE_SPACING = 42.0f;
constexpr float CREDIT_BUTTON_GAP   = 32.0f;
constexpr float OPTIONS_LABEL_Y     = 165.0f;
constexpr float OPTIONS_VALUE_Y     = 210.0f;
constexpr float OPTIONS_BAR_Y       = 265.0f;
constexpr float OPTIONS_BAR_WIDTH   = 340.0f;
constexpr float OPTIONS_BAR_HEIGHT  = 26.0f;
constexpr float OPTIONS_BACK_Y      = 410.0f;
constexpr float PAUSE_BTN_W         = 280.0f;
constexpr float PAUSE_BTN_H         = 54.0f;
constexpr int   GROUND_TILE_STRIDE  = static_cast<int>(TILE_SIZE) + 1;

int selectGroundTile(const TileMap& tileMap, int col, int row) {
    const bool top    = (row == 0) || !tileMap.isSolid(col, row - 1);
    const bool bottom = (row + 1 < tileMap.height()) && !tileMap.isSolid(col, row + 1);
    const bool left   = (col > 0) && !tileMap.isSolid(col - 1, row);
    const bool right  = (col + 1 < tileMap.width()) && !tileMap.isSolid(col + 1, row);

    if (top && left && right) {
        return 5; // All sides (or top + both sides)
    }
    if (top && right) {
        return 3; // Top + right
    }
    if (top && left) {
        return 4; // Top + left
    }
    if (top) {
        const unsigned h = static_cast<unsigned>(col) * 73856093u ^
                           static_cast<unsigned>(row) * 19349663u;
        return (h % 5u == 0u) ? 1 : 0; // Top grass + variation
    }
    if (bottom && right) {
        return 8; // Bottom + right
    }
    if (bottom && left) {
        return 9; // Bottom + left
    }
    if (right) {
        return 6; // Right only
    }
    if (left) {
        return 7; // Left only
    }
    return 2; // Pure dirt
}
}

Renderer::Renderer(sf::RenderWindow& win)
    : m_window(win)
    , m_uiView({ UI_WIDTH * 0.5f, UI_HEIGHT * 0.5f }, { UI_WIDTH, UI_HEIGHT }) {
    m_window.setFramerateLimit(FRAME_RATE);
}

bool Renderer::loadResources() {
    using Option = MainOption;

    auto loadDirection = [](const std::string& dir, int count, std::vector<sf::Texture>& out) -> bool {
        out.clear();
        out.resize(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            char nameBuf[32];
            std::snprintf(nameBuf, sizeof(nameBuf), "/frame_%03d.png", i);
            if (!out[static_cast<std::size_t>(i)].loadFromFile(dir + nameBuf)) {
                return false;
            }
        }
        return true;
    };

    auto loadAnimPair = [&](const std::string& animRoot, int count, AnimPair& pair) -> bool {
        return loadDirection(animRoot + "/east", count, pair.east) &&
               loadDirection(animRoot + "/west", count, pair.west);
    };

    auto loadCroppedButton = [](const char* path, sf::Texture& outTex, sf::IntRect& outRect) -> bool {
        sf::Image img;
        if (!img.loadFromFile(path) || !outTex.loadFromImage(img)) {
            return false;
        }
        const sf::Vector2u sz = img.getSize();
        unsigned minX = sz.x, minY = sz.y, maxX = 0, maxY = 0;
        bool any = false;
        for (unsigned y = 0; y < sz.y; ++y) {
            for (unsigned x = 0; x < sz.x; ++x) {
                if (img.getPixel({ x, y }).a > 0) {
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);
                    maxX = std::max(maxX, x);
                    maxY = std::max(maxY, y);
                    any = true;
                }
            }
        }
        if (any) {
            outRect = sf::IntRect(
                { static_cast<int>(minX), static_cast<int>(minY) },
                { static_cast<int>(maxX - minX + 1), static_cast<int>(maxY - minY + 1) }
            );
        } else {
            outRect = sf::IntRect({ 0, 0 }, { static_cast<int>(sz.x), static_cast<int>(sz.y) });
        }
        return true;
    };

    return loadCroppedButton(PLAY_BTN_TEX,
                             m_mainBtnTex[static_cast<std::size_t>(Option::Play)],
                             m_mainBtnRect[static_cast<std::size_t>(Option::Play)]) &&
           loadCroppedButton(OPTIONS_BTN_TEX,
                             m_mainBtnTex[static_cast<std::size_t>(Option::Options)],
                             m_mainBtnRect[static_cast<std::size_t>(Option::Options)]) &&
           loadCroppedButton(CREDITS_BTN_TEX,
                             m_mainBtnTex[static_cast<std::size_t>(Option::Credits)],
                             m_mainBtnRect[static_cast<std::size_t>(Option::Credits)]) &&
           loadCroppedButton(EXIT_BTN_TEX,
                             m_mainBtnTex[static_cast<std::size_t>(Option::Exit)],
                             m_mainBtnRect[static_cast<std::size_t>(Option::Exit)]) &&
           m_backBtnTex.loadFromFile(BACK_BTN_TEX) &&
           m_groundTex.loadFromFile(FLOOR_TEX) &&
           m_coinTex.loadFromFile(COIN_TEX) &&
           m_slimeRightTex.loadFromFile(SLIME_RIGHT_TEX) &&
           m_slimeLeftTex.loadFromFile(SLIME_LEFT_TEX) &&
           m_mushroomTex.loadFromFile(MUSHROOM_TEX) &&
           m_potionTex.loadFromFile(POTION_TEX) &&
           m_trapBlockTex.loadFromFile(TRAP_BLOCK_TEX) &&
           m_trapAtkTex.loadFromFile(TRAP_ATK_TEX) &&
           m_bgLayer1Tex.loadFromFile(BG_LAYER1_TEX) &&
           m_bgLayer2Tex.loadFromFile(BG_LAYER2_TEX) &&
           m_bgLayer3Tex.loadFromFile(BG_LAYER3_TEX) &&
           m_menuBgLayers[0].loadFromFile("paralax menu/camada1.png") &&
           m_menuBgLayers[1].loadFromFile("paralax menu/camada2.png") &&
           m_menuBgLayers[2].loadFromFile("paralax menu/camada3.png") &&
           m_menuBgLayers[3].loadFromFile("paralax menu/camada4.png") &&
           m_menuBgLayers[4].loadFromFile("paralax menu/camada5.png") &&
           m_font.openFromFile(MENU_FONT) &&
           loadAnimPair("Create_a_charming_and_me-Idle/Idle/animations/Walking",
                        Player::WALK_FRAME_COUNT, m_walkAnim) &&
           loadAnimPair("Create_a_charming_and_me-Idle_copy/Idle_copy/animations/Running",
                        Player::RUN_FRAME_COUNT, m_runAnim) &&
           loadAnimPair("Create_a_charming_and_me-Create_a_pixel_art_i/Create_a_pixel_art_i/animations/Jumping",
                        Player::JUMP_FRAME_COUNT, m_jumpAnim);
}

const sf::Texture& Renderer::menuButtonTexture(const Menu& menu, std::size_t index) const {
    if (menu.currentScreen() == MenuScreen::Main) {
        return m_mainBtnTex[index];
    }
    return m_backBtnTex;
}

sf::Vector2f Renderer::menuButtonPosition(const Menu& menu, std::size_t index) const {
    const float centerX = UI_WIDTH * 0.5f;

    if (menu.currentScreen() == MenuScreen::Options ||
        menu.currentScreen() == MenuScreen::PauseOptions) {
        return { centerX, OPTIONS_BACK_Y };
    }

    float startY = MENU_START_Y;
    if (menu.currentScreen() == MenuScreen::Credits) {
        startY += static_cast<float>(menu.creditsLines().size()) * CREDIT_LINE_SPACING;
        startY += CREDIT_BUTTON_GAP;
    }

    return {
        centerX,
        startY + static_cast<float>(index) * MENU_BUTTON_SPACING
    };
}

sf::FloatRect Renderer::volumeControlBounds() const {
    const float centerX = UI_WIDTH * 0.5f;
    return {
        { centerX - OPTIONS_BAR_WIDTH * 0.5f, OPTIONS_BAR_Y - OPTIONS_BAR_HEIGHT * 0.5f },
        { OPTIONS_BAR_WIDTH, OPTIONS_BAR_HEIGHT }
    };
}

sf::FloatRect Renderer::getMenuButtonBounds(const Menu& menu, std::size_t index) const {
    if ((menu.currentScreen() == MenuScreen::Options ||
         menu.currentScreen() == MenuScreen::PauseOptions) &&
        index == static_cast<std::size_t>(OptionsItem::MasterVolume)) {
        return volumeControlBounds();
    }

    if (menu.currentScreen() == MenuScreen::Pause) {
        const sf::Vector2f pos = menuButtonPosition(menu, index);
        return {
            { pos.x - PAUSE_BTN_W * 0.5f, pos.y - PAUSE_BTN_H * 0.5f },
            { PAUSE_BTN_W, PAUSE_BTN_H }
        };
    }

    const sf::Vector2f position = menuButtonPosition(menu, index);
    sf::Vector2f size;
    if (menu.currentScreen() == MenuScreen::Main) {
        const sf::IntRect& r = m_mainBtnRect[index];
        size = {
            static_cast<float>(r.size.x) * MENU_BTN_SCALE,
            static_cast<float>(r.size.y) * MENU_BTN_SCALE
        };
    } else {
        const sf::Texture& texture = menuButtonTexture(menu, index);
        const sf::FloatRect localBounds = sf::Sprite(texture).getLocalBounds();
        size = localBounds.size;
    }

    return {
        { position.x - size.x * 0.5f, position.y - size.y * 0.5f },
        size
    };
}

void Renderer::drawSpriteButton(const sf::Texture& texture, sf::Vector2f position, const sf::IntRect* rect, float scale) {
    sf::Sprite sprite(texture);
    if (rect != nullptr) {
        sprite.setTextureRect(*rect);
    }
    const sf::FloatRect bounds = sprite.getLocalBounds();
    sprite.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });
    sprite.setScale({ scale, scale });
    sprite.setPosition(position);
    m_window.draw(sprite);
}

void Renderer::drawVolumeControl(const Menu& menu) {
    const sf::FloatRect bounds = volumeControlBounds();

    sf::RectangleShape background(bounds.size);
    background.setPosition(bounds.position);
    background.setFillColor(sf::Color(70, 70, 70));
    m_window.draw(background);

    const float volumeRatio = static_cast<float>(menu.volume()) /
                              static_cast<float>(Menu::MAX_VOLUME);
    sf::RectangleShape fill({ bounds.size.x * volumeRatio, bounds.size.y });
    fill.setPosition(bounds.position);
    fill.setFillColor(sf::Color(235, 200, 70));
    m_window.draw(fill);
}

void Renderer::setupMapView(float zoomFactor) {
    m_mapView.setSize({
        UI_WIDTH / zoomFactor,
        UI_HEIGHT / zoomFactor
    });
    m_mapView.setCenter({
        m_mapView.getSize().x * 0.5f,
        (MAP_HEIGHT * TILE_SIZE) * 0.5f
    });
    m_window.setView(m_mapView);
}

void Renderer::updateCamera(const Player& player, const TileMap& tileMap) {
    const sf::Vector2f viewSize = m_mapView.getSize();
    const float mapW = static_cast<float>(tileMap.width()) * TILE_SIZE;
    const float mapH = static_cast<float>(tileMap.height()) * TILE_SIZE;

    const float halfW = viewSize.x * 0.5f;
    const float halfH = viewSize.y * 0.5f;

    const float camX = (mapW > viewSize.x)
        ? std::clamp(player.body.position.x, halfW, mapW - halfW)
        : (mapW * 0.5f);

    const float camY = (mapH > viewSize.y)
        ? std::clamp(player.body.position.y, halfH, mapH - halfH)
        : (mapH * 0.5f);

    m_mapView.setCenter({ camX, camY });
    m_window.setView(m_mapView);
}

void Renderer::drawMenu(const Menu& menu) {
    m_window.setView(m_uiView);

    if (menu.currentScreen() == MenuScreen::Pause ||
        menu.currentScreen() == MenuScreen::PauseOptions) {
        sf::RectangleShape overlay({ UI_WIDTH, UI_HEIGHT });
        overlay.setFillColor(sf::Color(0, 0, 0, 175));
        m_window.draw(overlay);
    }

    const float centerX = UI_WIDTH * 0.5f;

    if (menu.currentScreen() == MenuScreen::Pause ||
        menu.currentScreen() == MenuScreen::PauseOptions) {
        sf::Text title(m_font, "Pause", 48);
        title.setFillColor(sf::Color::White);
        const sf::FloatRect titleBounds = title.getLocalBounds();
        title.setOrigin({
            titleBounds.position.x + titleBounds.size.x * 0.5f,
            titleBounds.position.y + titleBounds.size.y * 0.5f
        });
        title.setPosition({ centerX, MENU_TITLE_Y });
        m_window.draw(title);
    }

    if (menu.currentScreen() == MenuScreen::Pause) {
        const char* labels[] = { "Voltar ao Jogo", "Configuracoes", "Voltar ao Menu" };
        for (std::size_t i = 0; i < menu.itemCount(); ++i) {
            const sf::Vector2f pos = menuButtonPosition(menu, i);
            sf::RectangleShape box({ PAUSE_BTN_W, PAUSE_BTN_H });
            box.setOrigin({ PAUSE_BTN_W * 0.5f, PAUSE_BTN_H * 0.5f });
            box.setPosition(pos);
            box.setFillColor(sf::Color(35, 35, 42, 235));
            box.setOutlineThickness(2.0f);
            box.setOutlineColor(sf::Color(200, 170, 75));
            m_window.draw(box);

            sf::Text txt(m_font, labels[i], 24);
            txt.setFillColor(sf::Color::White);
            const sf::FloatRect tb = txt.getLocalBounds();
            txt.setOrigin({
                tb.position.x + tb.size.x * 0.5f,
                tb.position.y + tb.size.y * 0.5f
            });
            txt.setPosition(pos);
            m_window.draw(txt);
        }
        return;
    }

    if (menu.currentScreen() == MenuScreen::Options ||
        menu.currentScreen() == MenuScreen::PauseOptions) {
        sf::Text label(m_font, "Game Volume", 30);
        label.setFillColor(sf::Color::White);
        const sf::FloatRect labelBounds = label.getLocalBounds();
        label.setOrigin({
            labelBounds.position.x + labelBounds.size.x * 0.5f,
            labelBounds.position.y + labelBounds.size.y * 0.5f
        });
        label.setPosition({ centerX, OPTIONS_LABEL_Y });
        m_window.draw(label);

        sf::Text value(m_font, "Sound: " + std::to_string(menu.volume()) + "%", 24);
        value.setFillColor(sf::Color(200, 200, 200));
        const sf::FloatRect valueBounds = value.getLocalBounds();
        value.setOrigin({
            valueBounds.position.x + valueBounds.size.x * 0.5f,
            valueBounds.position.y + valueBounds.size.y * 0.5f
        });
        value.setPosition({ centerX, OPTIONS_VALUE_Y });
        m_window.draw(value);

        drawVolumeControl(menu);
        drawSpriteButton(
            m_backBtnTex,
            menuButtonPosition(menu, static_cast<std::size_t>(OptionsItem::Back))
        );
        return;
    }

    float creditY = MENU_START_Y;
    if (menu.currentScreen() == MenuScreen::Credits) {
        for (const std::string& line : menu.creditsLines()) {
            sf::Text creditText(m_font, line, 26);
            creditText.setFillColor(sf::Color(200, 200, 200));
            const sf::FloatRect bounds = creditText.getLocalBounds();
            creditText.setOrigin({
                bounds.position.x + bounds.size.x * 0.5f,
                bounds.position.y + bounds.size.y * 0.5f
            });
            creditText.setPosition({ centerX, creditY });
            m_window.draw(creditText);
            creditY += CREDIT_LINE_SPACING;
        }
    }

    for (std::size_t index = 0; index < menu.itemCount(); ++index) {
        const sf::IntRect* rect = (menu.currentScreen() == MenuScreen::Main)
                                      ? &m_mainBtnRect[index]
                                      : nullptr;
        const float scale = (menu.currentScreen() == MenuScreen::Main) ? MENU_BTN_SCALE : 1.0f;
        drawSpriteButton(
            menuButtonTexture(menu, index),
            menuButtonPosition(menu, index),
            rect,
            scale
        );
    }
}

sf::RenderWindow& Renderer::getWindow() {
    return m_window;
}

const sf::View& Renderer::getUiView() const noexcept {
    return m_uiView;
}

bool Renderer::windowHasFocus() const {
    return m_window.hasFocus();
}

void Renderer::updateBackground(float dt) noexcept {
    if (dt > 0.0f) {
        m_bgTime += dt;
    }
}

void Renderer::drawParallaxLayer(const sf::Texture& tex, float parallaxFactor, float driftSpeedPx) {
    const sf::Vector2u texSize = tex.getSize();
    if (texSize.x == 0 || texSize.y == 0) {
        return;
    }

    const float scale = UI_HEIGHT / static_cast<float>(texSize.y);
    const float scaledW = static_cast<float>(texSize.x) * scale;

    const float totalScroll = m_mapView.getCenter().x * parallaxFactor + m_bgTime * driftSpeedPx;
    float offsetX = -std::fmod(totalScroll, scaledW);
    if (offsetX > 0.0f) {
        offsetX -= scaledW;
    }

    sf::Sprite layer(tex);
    layer.setScale({ scale, scale });

    for (float x = offsetX; x < UI_WIDTH; x += scaledW) {
        layer.setPosition({ x, 0.0f });
        m_window.draw(layer);
    }
}

void Renderer::drawBackground() {
    m_window.setView(m_uiView);
    drawParallaxLayer(m_bgLayer1Tex, 0.15f, 18.0f);
    drawParallaxLayer(m_bgLayer2Tex, 0.30f, 30.0f);
    drawParallaxLayer(m_bgLayer3Tex, 0.50f, 45.0f);
}

void Renderer::drawMenuBackground() {
    m_window.setView(m_uiView);
    constexpr std::array<float, 5> speeds = { 0.0f, 10.0f, 22.0f, 36.0f, 48.0f };
    for (std::size_t i = 0; i < m_menuBgLayers.size(); ++i) {
        drawParallaxLayer(m_menuBgLayers[i], 0.0f, speeds[i]);
    }
}

void Renderer::drawAnimatedPoint(sf::Vector2f centerPos, float scale) {
    constexpr int   COIN_FRAME_W     = 16;
    constexpr int   COIN_FRAME_H     = 17;
    constexpr int   COIN_FRAME_COUNT = 8;
    constexpr float COIN_ANIM_FPS    = 10.0f;

    const int frame = static_cast<int>(m_bgTime * COIN_ANIM_FPS) % COIN_FRAME_COUNT;
    sf::Sprite coin(m_coinTex);
    coin.setTextureRect(sf::IntRect(
        { frame * COIN_FRAME_W, 0 },
        { COIN_FRAME_W, COIN_FRAME_H }
    ));
    coin.setOrigin({ COIN_FRAME_W * 0.5f, COIN_FRAME_H * 0.5f });
    coin.setScale({ scale, scale });
    coin.setPosition(centerPos);
    m_window.draw(coin);
}

void Renderer::drawMap(const TileMap& tileMap) {
    m_window.setView(m_mapView);

    const sf::Vector2f viewCenter = m_mapView.getCenter();
    const sf::Vector2f viewSize   = m_mapView.getSize();
    const int minCol = std::max(0, static_cast<int>((viewCenter.x - viewSize.x * 0.5f) / TILE_SIZE) - 1);
    const int maxCol = std::min(tileMap.width() - 1, static_cast<int>((viewCenter.x + viewSize.x * 0.5f) / TILE_SIZE) + 1);

    for (int row = 0; row < tileMap.height(); ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            const int cell = tileMap.at(col, row);
            if (cell == EMPTY_TILE) {
                continue;
            }

            const float x = static_cast<float>(col) * TILE_SIZE;
            const float y = static_cast<float>(row) * TILE_SIZE;

            if (cell == WALL_TILE) {
                const int tileIdx = selectGroundTile(tileMap, col, row);
                const int tilePx  = static_cast<int>(TILE_SIZE);
                sf::Sprite groundSprite(m_groundTex);
                groundSprite.setTextureRect(sf::IntRect(
                    { tileIdx * GROUND_TILE_STRIDE, 0 },
                    { tilePx, tilePx }
                ));
                groundSprite.setPosition({ x, y });
                m_window.draw(groundSprite);
            } else if (cell == PLATFORM_TILE) {
                sf::RectangleShape block({ TILE_SIZE, TILE_SIZE });
                block.setPosition({ x, y });
                block.setFillColor(sf::Color(218, 158, 42));
                block.setOutlineThickness(-1.5f);
                block.setOutlineColor(sf::Color(125, 80, 18));
                m_window.draw(block);
            } else if (cell == COIN_TILE) {
                drawAnimatedPoint({ x + TILE_SIZE * 0.5f, y + TILE_SIZE * 0.5f }, 1.5f);
            } else if (cell == GOAL_TILE) {
                sf::RectangleShape pole({ 4.0f, TILE_SIZE });
                pole.setPosition({ x + TILE_SIZE * 0.5f - 2.0f, y });
                pole.setFillColor(sf::Color(230, 230, 230));
                m_window.draw(pole);

                if (row > 0 && tileMap.at(col, row - 1) != GOAL_TILE) {
                    sf::RectangleShape flag({ 20.0f, 14.0f });
                    flag.setPosition({ x + TILE_SIZE * 0.5f + 2.0f, y + 3.0f });
                    flag.setFillColor(sf::Color(60, 205, 75));
                    m_window.draw(flag);
                }
            }
        }
    }
}

void Renderer::drawEnemies(const std::vector<Enemy>& enemies) {
    m_window.setView(m_mapView);
    constexpr float SLIME_SCALE = 1.6f;

    for (const Enemy& enemy : enemies) {
        if (!enemy.active) {
            continue;
        }
        const sf::Texture& tex = (enemy.direction >= 0.0f) ? m_slimeRightTex : m_slimeLeftTex;
        const sf::Vector2u texSize = tex.getSize();
        if (texSize.x == 0 || texSize.y == 0) {
            continue;
        }

        const int frameW = static_cast<int>(texSize.x) / Enemy::FRAME_COUNT;
        const int frameH = static_cast<int>(texSize.y);
        const int frame  = std::clamp(enemy.animFrame, 0, Enemy::FRAME_COUNT - 1);

        sf::Sprite slime(tex);
        slime.setTextureRect(sf::IntRect({ frame * frameW, 0 }, { frameW, frameH }));
        slime.setOrigin({ static_cast<float>(frameW) * 0.5f, static_cast<float>(frameH) });
        slime.setScale({ SLIME_SCALE, SLIME_SCALE });
        slime.setPosition({
            enemy.body.position.x,
            enemy.body.position.y + enemy.collision.halfExtents.y
        });
        m_window.draw(slime);
    }
}

void Renderer::drawMushrooms(const std::vector<Mushroom>& mushrooms) {
    m_window.setView(m_mapView);
    constexpr int   FRAME_SIZE  = 32;
    constexpr float BOTTOM_PAD  = 4.0f;

    for (const Mushroom& m : mushrooms) {
        if (!m.active) {
            continue;
        }
        const int frame = std::clamp(m.animFrame, 0, 15);
        sf::Sprite sprite(m_mushroomTex);
        sprite.setTextureRect(sf::IntRect({ frame * FRAME_SIZE, 0 }, { FRAME_SIZE, FRAME_SIZE }));
        sprite.setOrigin({ FRAME_SIZE * 0.5f, static_cast<float>(FRAME_SIZE) - BOTTOM_PAD });
        sprite.setScale({ m.facingRight ? -1.0f : 1.0f, 1.0f });
        sprite.setPosition({
            m.body.position.x,
            m.body.position.y + m.collision.halfExtents.y
        });
        m_window.draw(sprite);
    }
}

void Renderer::drawProjectiles(const std::vector<Projectile>& projectiles) {
    m_window.setView(m_mapView);
    for (const Projectile& proj : projectiles) {
        if (!proj.active) {
            continue;
        }
        sf::CircleShape orb(proj.radius);
        orb.setOrigin({ proj.radius, proj.radius });
        orb.setPosition({ proj.position.x, proj.position.y });
        orb.setFillColor(sf::Color(190, 75, 235));
        orb.setOutlineThickness(1.5f);
        orb.setOutlineColor(sf::Color(90, 20, 120));
        m_window.draw(orb);
    }
}

void Renderer::drawPotions(const std::vector<HealthPotion>& potions) {
    m_window.setView(m_mapView);
    const sf::Vector2u texSize = m_potionTex.getSize();
    if (texSize.x == 0 || texSize.y == 0) {
        return;
    }

    constexpr float BOTTOM_PAD = 7.0f;
    sf::Sprite sprite(m_potionTex);
    sprite.setOrigin({
        static_cast<float>(texSize.x) * 0.5f,
        static_cast<float>(texSize.y) - BOTTOM_PAD
    });

    for (const HealthPotion& p : potions) {
        if (!p.active) {
            continue;
        }
        sprite.setPosition({ p.position.x, p.position.y + p.halfExtents.y });
        m_window.draw(sprite);
    }
}

void Renderer::drawTraps(const std::vector<Trap>& traps) {
    m_window.setView(m_mapView);
    constexpr int FRAME_SIZE = 32;

    for (const Trap& trap : traps) {
        if (trap.state == TrapState::Attacking) {
            const int frame = std::clamp(trap.animFrame, 0, Trap::ATK_FRAME_COUNT - 1);
            sf::Sprite atk(m_trapAtkTex);
            atk.setTextureRect(sf::IntRect({ frame * FRAME_SIZE, 0 }, { FRAME_SIZE, FRAME_SIZE }));
            atk.setOrigin({ FRAME_SIZE * 0.5f, FRAME_SIZE * 0.5f });
            atk.setPosition({ trap.position.x, trap.position.y });
            m_window.draw(atk);
        } else {
            sf::Sprite block(m_trapBlockTex);
            block.setOrigin({ FRAME_SIZE * 0.5f, FRAME_SIZE * 0.5f });
            block.setPosition({ trap.position.x, trap.position.y });
            m_window.draw(block);
        }
    }
}

void Renderer::drawPlayer(const Player& player) {
    if (!player.active) {
        return;
    }

    // Blink briefly while invulnerable after taking damage.
    if (player.invulnTimer > 0.0f && static_cast<int>(player.invulnTimer * 12.0f) % 2 == 0) {
        return;
    }

    const AnimPair* pair = &m_walkAnim;
    float bottomPad = 8.0f;
    if (player.animState == PlayerAnimState::Running) {
        pair = &m_runAnim;
        bottomPad = 9.0f;
    } else if (player.animState == PlayerAnimState::Jumping) {
        pair = &m_jumpAnim;
        bottomPad = 9.0f;
    }

    const std::vector<sf::Texture>& frames = player.facingRight ? pair->east : pair->west;
    if (frames.empty()) {
        return;
    }

    const std::size_t idx = std::min(static_cast<std::size_t>(player.animFrame), frames.size() - 1);
    sf::Sprite sprite(frames[idx]);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    sprite.setOrigin({ bounds.size.x * 0.5f, bounds.size.y - bottomPad });
    sprite.setPosition({
        player.body.position.x,
        player.body.position.y + player.collision.halfExtents.y
    });

    if (player.glowTimer > 0.0f) {
        const float pulse = 0.55f + 0.45f * std::sin(player.glowTimer * 16.0f);
        const float glowRadius = 24.0f + 4.0f * pulse;
        sf::CircleShape halo(glowRadius);
        halo.setOrigin({ glowRadius, glowRadius });
        halo.setPosition({ player.body.position.x, player.body.position.y });
        halo.setFillColor(sf::Color(255, 225, 90, static_cast<std::uint8_t>(140.0f * pulse)));
        m_window.draw(halo);
        sprite.setColor(sf::Color(255, 245, 175));
    } else if (player.isIntangible()) {
        sprite.setColor(sf::Color(180, 235, 255, 190));
    }

    m_window.draw(sprite);
}

void Renderer::drawHud(const Player& player, int bestCoins, int round) {
    m_window.setView(m_uiView);

    auto drawBar = [this](sf::Vector2f pos, sf::Vector2f size, float ratio, sf::Color fillColor) {
        const float clamped = std::clamp(ratio, 0.0f, 1.0f);

        sf::RectangleShape background(size);
        background.setPosition(pos);
        background.setFillColor(sf::Color(25, 25, 25, 200));
        background.setOutlineThickness(1.5f);
        background.setOutlineColor(sf::Color(60, 60, 60, 220));
        m_window.draw(background);

        if (clamped > 0.0f) {
            sf::RectangleShape fill({ size.x * clamped, size.y });
            fill.setPosition(pos);
            fill.setFillColor(fillColor);
            m_window.draw(fill);
        }
    };

    constexpr float BASE_BAR_WIDTH = 220.0f;
    const float hpBarWidth = BASE_BAR_WIDTH * (player.maxHp / Player::INITIAL_HP);
    const float staminaBarWidth = BASE_BAR_WIDTH * (player.maxStamina / Player::INITIAL_STAMINA);
    const float hpRatio = (player.maxHp > 0.0f) ? (player.hp / player.maxHp) : 0.0f;
    const float staminaRatio = (player.maxStamina > 0.0f) ? (player.stamina / player.maxStamina) : 0.0f;

    drawBar({ 20.0f, 20.0f }, { BASE_BAR_WIDTH, 18.0f }, hpRatio, sf::Color(220, 45, 45));
    drawBar({ 20.0f, 46.0f }, { BASE_BAR_WIDTH, 14.0f }, staminaRatio, sf::Color(0, 210, 235));
    drawBar({ 20.0f, 20.0f }, { hpBarWidth, 18.0f }, hpRatio, sf::Color(220, 45, 45));
    drawBar({ 20.0f, 46.0f }, { staminaBarWidth, 14.0f }, staminaRatio, sf::Color(0, 210, 235));

    drawAnimatedPoint({ 32.0f, 82.0f }, 1.5f);

    sf::Text countText(m_font, std::to_string(player.coins), 22);
    countText.setFillColor(sf::Color::Black);
    const sf::FloatRect tb = countText.getLocalBounds();
    countText.setOrigin({ tb.position.x, tb.position.y + tb.size.y * 0.5f });
    countText.setPosition({ 52.0f, 82.0f });
    m_window.draw(countText);

    drawAnimatedPoint({ 122.0f, 82.0f }, 1.5f);

    sf::Text bestText(m_font, std::to_string(bestCoins), 22);
    bestText.setFillColor(sf::Color::Black);
    const sf::FloatRect bb = bestText.getLocalBounds();
    bestText.setOrigin({ bb.position.x, bb.position.y + bb.size.y * 0.5f });
    bestText.setPosition({ 142.0f, 82.0f });
    m_window.draw(bestText);

    sf::Text roundText(m_font, "ROUND: " + std::to_string(round), 20);
    roundText.setFillColor(sf::Color::Black);
    roundText.setPosition({ 20.0f, 102.0f });
    m_window.draw(roundText);
}

void Renderer::drawFade(float alpha) {
    const float clamped = std::clamp(alpha, 0.0f, 1.0f);
    if (clamped <= 0.0f) {
        return;
    }
    m_window.setView(m_uiView);
    sf::RectangleShape overlay({ UI_WIDTH, UI_HEIGHT });
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(clamped * 255.0f + 0.5f)));
    m_window.draw(overlay);
}

void Renderer::beginFrame(sf::Color clearColor) {
    m_window.clear(clearColor);
}

void Renderer::endFrame() {
    m_window.display();
}
