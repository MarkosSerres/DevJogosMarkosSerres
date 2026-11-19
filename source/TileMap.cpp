#include "TileMap.hpp"
#include "Dungeon2D.hpp"
#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string_view>

namespace {
    void carveRoomsFromLeaves(const std::unique_ptr<BSPNode>& node, int width, int height, std::vector<int>& cells) {
        if (!node) {
            return;
        }

        if (node->isLeaf()) {
            int startCol = std::max(0, node->room.left);
            int endCol   = std::min(width, node->room.right);
            int startRow = std::max(0, node->room.top);
            int endRow   = std::min(height, node->room.bottom);

            for (int r = startRow; r < endRow; ++r) {
                for (int c = startCol; c < endCol; ++c) {
                    cells[r * width + c] = EMPTY_TILE;
                }
            }
            return;
        }

        carveRoomsFromLeaves(node->left, width, height, cells);
        carveRoomsFromLeaves(node->right, width, height, cells);
    }
}

void TileMap::loadFromDungeon(const Dungeon2D& dungeon) {
    if (dungeon.width <= 0 || dungeon.height <= 0) {
        m_width = 0;
        m_height = 0;
        m_cells.clear();
        return;
    }

    m_width = dungeon.width;
    m_height = dungeon.height;
    m_cells.assign(m_width * m_height, WALL_TILE);

    if (dungeon.root) {
        carveRoomsFromLeaves(dungeon.root, m_width, m_height, m_cells);
    }

    for (const auto& corridor : dungeon.corridors) {
        int minCol = std::min(corridor.left, corridor.right);
        int maxCol = std::max(corridor.left, corridor.right);
        int minRow = std::min(corridor.top, corridor.bottom);
        int maxRow = std::max(corridor.top, corridor.bottom);

        int startCol = std::max(0, minCol);
        int endCol   = std::min(m_width - 1, maxCol);
        int startRow = std::max(0, minRow);
        int endRow   = std::min(m_height - 1, maxRow);

        for (int r = startRow; r <= endRow; ++r) {
            for (int c = startCol; c <= endCol; ++c) {
                m_cells[r * m_width + c] = EMPTY_TILE;
            }
        }
    }
}

bool TileMap::saveCSV(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    for (int r = 0; r < m_height; ++r) {
        for (int c = 0; c < m_width; ++c) {
            file << m_cells[r * m_width + c];
            if (c + 1 < m_width) {
                file << ',';
            }
        }
        file << '\n';
    }

    return file.good();
}

bool TileMap::loadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::vector<int> tempCells;
    int tempWidth = -1;
    int tempHeight = 0;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            return false;
        }

        // Trailing comma check: row must not end with a trailing comma
        if (line.back() == ',') {
            return false;
        }

        int rowCols = 0;
        size_t start = 0;

        while (start <= line.size()) {
            size_t commaPos = line.find(',', start);
            size_t len = (commaPos == std::string::npos) ? (line.size() - start) : (commaPos - start);

            if (len == 0) {
                return false;
            }

            std::string_view token(line.data() + start, len);
            int value = 0;
            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
            if (ec != std::errc{} || ptr != token.data() + token.size()) {
                return false;
            }

            tempCells.push_back(value);
            rowCols++;

            if (commaPos == std::string::npos) {
                break;
            }
            start = commaPos + 1;
        }

        if (tempHeight == 0) {
            tempWidth = rowCols;
            if (tempWidth <= 0) {
                return false;
            }
        } else if (rowCols != tempWidth) {
            return false;
        }

        tempHeight++;
    }

    if (tempHeight <= 0 || tempWidth <= 0) {
        return false;
    }

    if (tempCells.size() != static_cast<size_t>(tempWidth * tempHeight)) {
        return false;
    }

    m_width = tempWidth;
    m_height = tempHeight;
    m_cells = std::move(tempCells);
    return true;
}

int TileMap::width() const noexcept {
    return m_width;
}

int TileMap::height() const noexcept {
    return m_height;
}

int TileMap::at(int col, int row) const {
    assert(col >= 0 && col < m_width);
    assert(row >= 0 && row < m_height);
    return m_cells[row * m_width + col];
}

bool TileMap::isWall(int col, int row) const {
    assert(col >= 0 && col < m_width);
    assert(row >= 0 && row < m_height);
    return at(col, row) == WALL_TILE;
}

bool TileMap::isSolid(int col, int row) const {
    if (col < 0 || col >= m_width || row < 0 || row >= m_height) {
        return false;
    }
    const int val = m_cells[row * m_width + col];
    return val == WALL_TILE || val == PLATFORM_TILE;
}

void TileMap::setCell(int col, int row, int value) {
    if (col < 0 || col >= m_width || row < 0 || row >= m_height) {
        return;
    }
    m_cells[row * m_width + col] = value;
}

std::vector<Vector2D> TileMap::loadPlatformLevel(unsigned seed) {
    m_width = MAP_WIDTH;
    m_height = MAP_HEIGHT;
    m_cells.assign(static_cast<std::size_t>(m_width * m_height), EMPTY_TILE);

    std::mt19937 rng(seed);
    std::vector<Vector2D> enemySpawns;

    auto fillGround = [this](int startCol, int endCol, int topRow) {
        for (int c = startCol; c <= endCol; ++c) {
            for (int r = topRow; r < m_height; ++r) {
                setCell(c, r, WALL_TILE);
            }
        }
    };

    auto fillRow = [this](int startCol, int endCol, int row, int tile) {
        for (int c = startCol; c <= endCol; ++c) {
            setCell(c, row, tile);
        }
    };

    // Left boundary wall so the player cannot walk off the left edge.
    for (int row = 0; row < m_height; ++row) {
        setCell(0, row, WALL_TILE);
    }

    // Safe starting zone.
    int groundRow = m_height - 3; // row 13
    fillGround(1, 7, groundRow);
    int col = 8;

    std::uniform_int_distribution<int> segDist(0, 4);
    std::uniform_int_distribution<int> stepDist(-2, 2);
    std::uniform_int_distribution<int> pitDist(2, 3);
    std::uniform_int_distribution<int> lenDist(5, 8);
    std::uniform_int_distribution<int> coinChance(0, 1);

    const int goalZoneStart = m_width - 14;

    while (col < goalZoneStart - 6) {
        const int segType = segDist(rng);

        if (segType == 0) {
            // Elevated/stepped terrain with optional coins and enemy.
            const int delta = stepDist(rng);
            groundRow = std::clamp(groundRow + delta, m_height - 5, m_height - 2);
            const int segLen = lenDist(rng);
            const int endCol = std::min(col + segLen - 1, goalZoneStart - 1);
            fillGround(col, endCol, groundRow);

            if (coinChance(rng) == 1 && endCol - col >= 3) {
                fillRow(col + 1, endCol - 1, groundRow - 2, COIN_TILE);
            }
            if (endCol - col >= 4) {
                enemySpawns.push_back(toWorld(col + (endCol - col) / 2, groundRow - 1));
            }
            col = endCol + 1;
        } else if (segType == 1) {
            // Pit followed by safe landing platform.
            const int pitWidth = pitDist(rng);
            col += pitWidth;
            const int landLen = lenDist(rng);
            const int endCol = std::min(col + landLen - 1, goalZoneStart - 1);
            groundRow = std::clamp(groundRow + (stepDist(rng) > 0 ? 1 : 0), m_height - 4, m_height - 2);
            fillGround(col, endCol, groundRow);

            // Coins arching over the pit.
            fillRow(col - pitWidth, col - 1, groundRow - 3, COIN_TILE);
            if (endCol - col >= 4) {
                enemySpawns.push_back(toWorld(col + 2, groundRow - 1));
            }
            col = endCol + 1;
        } else if (segType == 2) {
            // Ground + multi-tier floating platforms with coins & upper enemy.
            const int segLen = 8;
            const int endCol = std::min(col + segLen - 1, goalZoneStart - 1);
            fillGround(col, endCol, groundRow);

            const int plat1Row = std::max(4, groundRow - 3);
            fillRow(col + 1, col + 4, plat1Row, PLATFORM_TILE);
            fillRow(col + 2, col + 3, plat1Row - 2, COIN_TILE);

            if (plat1Row >= 6) {
                const int plat2Row = plat1Row - 3;
                fillRow(col + 4, std::min(col + 7, endCol), plat2Row, PLATFORM_TILE);
                fillRow(col + 5, std::min(col + 6, endCol), plat2Row - 1, COIN_TILE);
            }
            enemySpawns.push_back(toWorld(col + 5, groundRow - 1));
            col = endCol + 1;
        } else if (segType == 3) {
            // Wide pit bridged by a floating stepping-stone island.
            col += 2;
            const int islandRow = std::clamp(groundRow - 1, m_height - 5, m_height - 3);
            fillRow(col, col + 2, islandRow, PLATFORM_TILE);
            fillRow(col, col + 2, islandRow - 2, COIN_TILE);
            col += 5; // 2-tile gap after island

            const int endCol = std::min(col + 4, goalZoneStart - 1);
            fillGround(col, endCol, groundRow);
            col = endCol + 1;
        } else {
            // Stepped brick obstacle / mini-pyramid on ground.
            const int segLen = 7;
            const int endCol = std::min(col + segLen - 1, goalZoneStart - 1);
            fillGround(col, endCol, groundRow);
            if (endCol - col >= 5) {
                setCell(col + 2, groundRow - 1, WALL_TILE);
                setCell(col + 3, groundRow - 1, WALL_TILE);
                setCell(col + 3, groundRow - 2, WALL_TILE);
                setCell(col + 4, groundRow - 1, WALL_TILE);
                fillRow(col + 2, col + 4, groundRow - 4, COIN_TILE);
            }
            col = endCol + 1;
        }
    }

    // Final stretch: flat ground, staircase and goal flagpole.
    const int finalGroundRow = m_height - 3;
    fillGround(col, m_width - 1, finalGroundRow);

    const int stairBase = m_width - 12;
    for (int step = 0; step < 5; ++step) {
        for (int r = finalGroundRow - 1 - step; r < finalGroundRow; ++r) {
            setCell(stairBase + step, r, WALL_TILE);
        }
    }

    const int flagCol = m_width - 5;
    for (int r = finalGroundRow - 6; r < finalGroundRow; ++r) {
        setCell(flagCol, r, GOAL_TILE);
    }

    return enemySpawns;
}

const int* TileMap::data() const noexcept {
    return m_cells.data();
}

void TileMap::toIndices(const Vector2D& world, int& col, int& row) const noexcept {
    col = static_cast<int>(std::floor(world.x / TILE_SIZE));
    row = static_cast<int>(std::floor(world.y / TILE_SIZE));
}

Vector2D TileMap::toWorld(int col, int row) const noexcept {
    return Vector2D{
        (static_cast<float>(col) + 0.5f) * TILE_SIZE,
        (static_cast<float>(row) + 0.5f) * TILE_SIZE
    };
}

void TileMap::randomEmptyCell(int& col, int& row) const {
    std::vector<int> emptyIndices;
    emptyIndices.reserve(m_cells.size());

    for (int i = 0; i < static_cast<int>(m_cells.size()); ++i) {
        if (m_cells[i] == EMPTY_TILE) {
            emptyIndices.push_back(i);
        }
    }

    assert(!emptyIndices.empty());

    if (emptyIndices.empty()) {
        col = 0;
        row = 0;
        return;
    }

    int chosen = emptyIndices[std::rand() % emptyIndices.size()];
    col = chosen % m_width;
    row = chosen / m_width;
}
