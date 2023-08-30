/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "pch.hpp"
#include "spectators.hpp"
#include "creatures/creature.hpp"
#include "game/movement/position.hpp"

static phmap::flat_hash_map<Position, std::vector<Creature*>, Position::Hasher> spectators;
static phmap::flat_hash_map<Position, std::vector<Creature*>, Position::Hasher> spectatorPlayers;

std::pair<uint8_t, uint8_t> getZMinMaxRange(uint8_t z, bool multiFloor) {
	uint8_t minRangeZ = z;
	uint8_t maxRangeZ = z;

	if (multiFloor) {
		// underground (8->15)
		if (z > MAP_INIT_SURFACE_LAYER) {
			minRangeZ = std::max<int32_t>(z - MAP_LAYER_VIEW_LIMIT, 0);
			maxRangeZ = std::min<int32_t>(z + MAP_LAYER_VIEW_LIMIT, MAP_MAX_LAYERS - 1);
		} else if (z == MAP_INIT_SURFACE_LAYER - 1) {
			minRangeZ = 0;
			maxRangeZ = (MAP_INIT_SURFACE_LAYER - 1) + MAP_LAYER_VIEW_LIMIT;
		} else if (z == MAP_INIT_SURFACE_LAYER) {
			minRangeZ = 0;
			maxRangeZ = MAP_INIT_SURFACE_LAYER + MAP_LAYER_VIEW_LIMIT;
		} else {
			minRangeZ = 0;
			maxRangeZ = MAP_INIT_SURFACE_LAYER;
		}
	}

	return std::make_pair(minRangeZ, maxRangeZ);
}

void Spectators::put(Creature* creature, bool allTiles) {
	if (!creature || !creature->getParent()) {
		return;
	}

	const auto &centerPos = creature->getPosition();

	const int_fast32_t minY = centerPos.y - MAP_MAX_VIEW_PORT_Y - 1;
	const int_fast32_t minX = centerPos.x - MAP_MAX_VIEW_PORT_X - 1;
	const int_fast32_t maxY = centerPos.y + MAP_MAX_VIEW_PORT_Y;
	const int_fast32_t maxX = centerPos.x + MAP_MAX_VIEW_PORT_X;

	const auto &[minZ, maxZ] = getZMinMaxRange(centerPos.z, true);

	if (allTiles) {
		for (int_fast32_t ny = minY; ++ny <= maxY;) {
			for (int_fast32_t nx = minX; ++nx <= maxX;) {
				for (int_fast32_t nz = minZ - 1; ++nz <= maxZ;) {
					const auto &pos = Position(minX, minY, minZ);
					spectators.try_emplace(pos).first->second.emplace_back(creature);
					if (creature->getPlayer()) {
						spectatorPlayers.try_emplace(pos).first->second.emplace_back(creature);
					}
				}
			}
		}
	}
}
