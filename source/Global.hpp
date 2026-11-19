#ifndef GAME_GLOBAL_HPP
#define GAME_GLOBAL_HPP

/**
 * @file Global.hpp
 * @brief Project-wide identifiers, constants and representation conventions.
 *
 * This file will be the single source of reference for the project;
 *   no other file should redefine values listed here; 
 *   all files must always refer to symbols defined here.
 *
 * ---------------------------------------------------------------------------
 * COORDINATE CONVENTIONS (for the whole project)
 * ---------------------------------------------------------------------------
 *  - World, physical, coordinates are continuous, measured in PIXELS, stored
 *    as float. The Y axis grows DOWNWARD (screen convention).
 *  - The tile grid is discrete. The cell containing a world position is
 *        col = floor(world.x / TILE_SIZE),  row = floor(world.y / TILE_SIZE).
 *    The world position is the reference; a cell index is always derived from
 *    it and never stored as an independent second copy of a position.
 *  - The centre of tile (col,row) in world coordinates is
 *        ( (col + 0.5) * TILE_SIZE , (row + 0.5) * TILE_SIZE ).
 *
 * ---------------------------------------------------------------------------
 * TILE MATRIX REPRESENTATION (used by TileMap and its CSV files)
 * ---------------------------------------------------------------------------
 *  - One integer per cell. EMPTY_TILE marks a free cell, WALL_TILE a solid one.
 */

/// Integer type used for every entity identifier.
using EntityId = int;

// --- Reserved entity identifiers ------------------------------------------
/// Identifier reserved for the single player entity. It is below the
/// auto-generated range, so it can never collide with a generated id.
constexpr EntityId PLAYER_ID = 100;
/// First identifier handed out to automatically numbered entities.
constexpr EntityId INITIAL_ENTITY_ID = 1000;

// --- Tile matrix legend ----------------------------------------------------
constexpr int EMPTY_TILE    = 0;   ///< A free, walkable/air cell.
constexpr int WALL_TILE     = 1;   ///< Solid ground or brick block.
constexpr int PLATFORM_TILE = 2;   ///< Solid bonus/platform block.
constexpr int COIN_TILE     = 3;   ///< Collectible coin cell.
constexpr int GOAL_TILE     = 4;   ///< Level goal flagpole cell.

// --- Spatial constants -----------------------------------------------------
/// Common default size, in pixels, of a tile, a sprite and an NPC: they are
/// all the same, 32 x 32.
constexpr float TILE_SIZE = 32.0f;

// --- Numeric tolerance -----------------------------------------------------
/// Tolerance used in floating-point comparisons.
constexpr float EPSILON = 0.0001f;

// --- Platformer world & physics constants ----------------------------------
constexpr int   MAP_WIDTH      = 120;
constexpr int   MAP_HEIGHT     = 16;
constexpr int   SEED           = 42;
constexpr float FIXED_DT       = 1.0f / 60.0f;
constexpr int   FRAME_RATE     = 60;
constexpr float GRAVITY        = 1100.0f;
constexpr float MAX_FALL_SPEED = 650.0f;

constexpr const char* FLOOR_TEX       = "spriteChao.png";
constexpr const char* COIN_TEX        = "pontos-sheet.png";
constexpr const char* SLIME_RIGHT_TEX = "slimeDireita-Sheet.png";
constexpr const char* SLIME_LEFT_TEX  = "slimeEsquerda-Sheet.png";
constexpr const char* MUSHROOM_TEX    = "cogumelo-Sheet-Sheet.png";
constexpr const char* POTION_TEX      = "pocaoDeVida.png";
constexpr const char* TRAP_BLOCK_TEX  = "armadilhasBloco.png";
constexpr const char* TRAP_ATK_TEX    = "armadilhaAtk-Sheet.png";
constexpr const char* COLLECT_SFX         = "liecio-collect-points-190037.mp3";
constexpr const char* VICTORY_SFX         = "vitoria.mp3";
constexpr const char* DAMAGE_SFX          = "Dano.mp3";
constexpr const char* DEFEAT_SFX          = "derrota.mp3";
constexpr const char* TRAP_SFX            = "armadilha.mp3";
constexpr const char* MUSHROOM_ATTACK_SFX = "cogumeloAtaque.mp3";
constexpr const char* SLIME_JUMP_SFX      = "slimePulo.mp3";
constexpr const char* SLIME_DEATH_SFX     = "slimeMorte.mp3";
constexpr const wchar_t* GAME_MUSIC   = L"The Knight\u2019s Quiet Fire  Medieval Fantasy Music for Peaceful Focus - Gates of Vortalania.mp3";
constexpr const char* BG_LAYER1_TEX   = "camada1.png";
constexpr const char* BG_LAYER2_TEX   = "camada2.png";
constexpr const char* BG_LAYER3_TEX   = "camada3.png";
constexpr const char* WALL_TEX        = "TileSet.png";
constexpr const char* PLAYER_TEX      = "player.png";
constexpr const char* CHEST_TEX       = "Bau.png";
constexpr const char* PLAY_BTN_TEX    = "jogar.png";
constexpr const char* OPTIONS_BTN_TEX = "suporte.png";
constexpr const char* CREDITS_BTN_TEX = "sobre.png";
constexpr const char* EXIT_BTN_TEX    = "sair.png";
constexpr const char* BACK_BTN_TEX    = "backButton.png";
constexpr const char* MENU_FONT       = "font.ttf";

#endif // GAME_GLOBAL_HPP
