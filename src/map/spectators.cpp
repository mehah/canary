/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "spectators.hpp"
#include "game/game.hpp"

phmap::flat_hash_map < Position, std::pair<CreatureVector, CreatureVector>> spectatorCache;
phmap::flat_hash_map<Position, std::pair<CreatureVector, CreatureVector>> playersSpectatorCache;

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

std::vector<Creature*> Spectators::get() {
	if (update) {
		update = false;
		std::sort(creatures.begin(), creatures.end());
		creatures.erase(std::unique(creatures.begin(), creatures.end()), creatures.end());
	}

	return creatures;
}

std::vector<Creature*> get(const Position& centerPos, bool multifloor, bool onlyPlayers, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	return Spectators().find(centerPos, multifloor, onlyPlayers, minRangeX, maxRangeX, minRangeY, maxRangeY).get();
}

Spectators Spectators::find(const Position &centerPos, bool multifloor, bool onlyPlayers, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {

	auto &hashmap = onlyPlayers ? playersSpectatorCache : spectatorCache; 

	if (minRangeX == -MAP_MAX_VIEW_PORT_X && maxRangeX == MAP_MAX_VIEW_PORT_X && minRangeY == -MAP_MAX_VIEW_PORT_Y && maxRangeY == MAP_MAX_VIEW_PORT_Y) {
			auto it = hashmap.find(centerPos);
			if (it != hashmap.end()) {
			const auto &list = multifloor ? it->second.first : it->second.second;
			creatures.insert(creatures.end(), list.begin(), list.end());
				return *this;
			}
	}

	const auto &[minRangeZ, maxRangeZ] = getZMinMaxRange(centerPos.z, multifloor);

	if (!creatures.empty())
		update = true;

	minRangeX = (minRangeX == 0 ? -MAP_MAX_VIEW_PORT_X : -minRangeX);
	maxRangeX = (maxRangeX == 0 ? MAP_MAX_VIEW_PORT_X : maxRangeX);
	minRangeY = (minRangeY == 0 ? -MAP_MAX_VIEW_PORT_Y : -minRangeY);
	maxRangeY = (maxRangeY == 0 ? MAP_MAX_VIEW_PORT_Y : maxRangeY);

	int_fast32_t min_y = centerPos.y + minRangeY;
	int_fast32_t min_x = centerPos.x + minRangeX;
	int_fast32_t max_y = centerPos.y + maxRangeY;
	int_fast32_t max_x = centerPos.x + maxRangeX;

	int32_t minoffset = centerPos.getZ() - maxRangeZ;
	uint16_t x1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (min_x + minoffset)));
	uint16_t y1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (min_y + minoffset)));

	int32_t maxoffset = centerPos.getZ() - minRangeZ;
	uint16_t x2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (max_x + maxoffset)));
	uint16_t y2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (max_y + maxoffset)));

	int32_t startx1 = x1 - (x1 % FLOOR_SIZE);
	int32_t starty1 = y1 - (y1 % FLOOR_SIZE);
	int32_t endx2 = x2 - (x2 % FLOOR_SIZE);
	int32_t endy2 = y2 - (y2 % FLOOR_SIZE);

	const auto startLeaf = g_game().map.getQTNode(startx1, starty1);
	const QTreeLeafNode* leafS = startLeaf;
	const QTreeLeafNode* leafE;

	auto &specCache = spectatorCache.try_emplace(centerPos).first->second;
	auto &list = multifloor ? specCache.first : specCache.second;

	for (int_fast32_t ny = starty1; ny <= endy2; ny += FLOOR_SIZE) {
		leafE = leafS;
		for (int_fast32_t nx = startx1; nx <= endx2; nx += FLOOR_SIZE) {
			if (leafE) {
				const auto &node_list = (onlyPlayers ? leafE->player_list : leafE->creature_list);
				for (Creature* creature : node_list) {
					const Position &cpos = creature->getPosition();
					if (minRangeZ > cpos.z || maxRangeZ < cpos.z) {
						continue;
					}

					int_fast16_t offsetZ = Position::getOffsetZ(centerPos, cpos);
					if ((min_y + offsetZ) > cpos.y || (max_y + offsetZ) < cpos.y || (min_x + offsetZ) > cpos.x || (max_x + offsetZ) < cpos.x) {
						continue;
					}

					list.emplace_back(creature);
				}
				leafE = leafE->leafE;
			} else {
				leafE = g_game().map.getQTNode(nx + FLOOR_SIZE, ny);
			}
		}

		if (leafS) {
			leafS = leafS->leafS;
		} else {
			leafS = g_game().map.getQTNode(startx1, ny + FLOOR_SIZE);
		}
	}

	 creatures.insert(creatures.end(), list.begin(), list.end());

	return *this;
}
