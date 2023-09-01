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

phmap::flat_hash_map<Position, std::pair<SpectatorList, SpectatorList>> spectatorCache;
phmap::flat_hash_map<Position, std::pair<SpectatorList, SpectatorList>> playersSpectatorCache;

void Spectators::clearCache() {
	spectatorCache.clear();
	playersSpectatorCache.clear();
}

void Spectators::update() {
	if (!needUpdate) {
		return;
	}

	needUpdate = false;
#ifndef SPECTATOR_USE_HASH_SET
	std::sort(creatures.begin(), creatures.end());
	creatures.erase(std::unique(creatures.begin(), creatures.end()), creatures.end());
#endif
}

template <typename T, typename std::enable_if<std::is_base_of<Creature, T>::value>::type*>
Spectators Spectators::filter() {
	update();
	auto specs = Spectators();
	for (const auto &c : creatures) {
		if (std::is_same_v<T, Player> && c->getPlayer() || std::is_same_v<T, Monster> && c->getMonster() || std::is_same_v<T, Npc> && c->getNpc()) {
#ifdef SPECTATOR_USE_HASH_SET
			specs.creatures.emplace(c);
#else
			specs.creatures.emplace_back(c);
#endif
		}
	}

	return specs;
}

template <typename T, typename std::enable_if<std::is_same<Creature, T>::value || std::is_same<Player, T>::value>::type*>
Spectators Spectators::find(const Position &centerPos, bool multifloor, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	if (!creatures.empty()) {
		needUpdate = true;
	}

	const bool onlyPlayers = std::is_same_v<T, Player>;

	auto &hashmap = onlyPlayers ? playersSpectatorCache : spectatorCache;

	minRangeX = (minRangeX == 0 ? -MAP_MAX_VIEW_PORT_X : -minRangeX);
	maxRangeX = (maxRangeX == 0 ? MAP_MAX_VIEW_PORT_X : maxRangeX);
	minRangeY = (minRangeY == 0 ? -MAP_MAX_VIEW_PORT_Y : -minRangeY);
	maxRangeY = (maxRangeY == 0 ? MAP_MAX_VIEW_PORT_Y : maxRangeY);

	const auto &it = hashmap.find(centerPos);
	if (it != hashmap.end()) {
		const auto &list = multifloor ? it->second.first : it->second.second;

		if (minRangeX == -MAP_MAX_VIEW_PORT_X && maxRangeX == MAP_MAX_VIEW_PORT_X && minRangeY == -MAP_MAX_VIEW_PORT_Y && maxRangeY == MAP_MAX_VIEW_PORT_Y) {
#ifdef SPECTATOR_USE_HASH_SET
			creatures.insert(list.begin(), list.end());
#else
			creatures.insert(creatures.end(), list.begin(), list.end());
#endif
		} else {
			for (const auto creature : list) {
				if (centerPos.x - creature->getPosition().x >= minRangeX
					&& centerPos.y - creature->getPosition().y >= minRangeY
					&& centerPos.x - creature->getPosition().x <= maxRangeX
					&& centerPos.y - creature->getPosition().y <= maxRangeY) {
#ifdef SPECTATOR_USE_HASH_SET
					creatures.emplace(creature);
#else
					creatures.emplace_back(creature);
#endif
				}
			}
		}

		return *this;
	}

	uint8_t minRangeZ = centerPos.z;
	uint8_t maxRangeZ = centerPos.z;

	if (multifloor) {
		if (centerPos.z > MAP_INIT_SURFACE_LAYER) {
			minRangeZ = std::max<int32_t>(centerPos.z - MAP_LAYER_VIEW_LIMIT, 0);
			maxRangeZ = std::min<int32_t>(centerPos.z + MAP_LAYER_VIEW_LIMIT, MAP_MAX_LAYERS - 1);
		} else if (centerPos.z == MAP_INIT_SURFACE_LAYER - 1) {
			minRangeZ = 0;
			maxRangeZ = (MAP_INIT_SURFACE_LAYER - 1) + MAP_LAYER_VIEW_LIMIT;
		} else if (centerPos.z == MAP_INIT_SURFACE_LAYER) {
			minRangeZ = 0;
			maxRangeZ = MAP_INIT_SURFACE_LAYER + MAP_LAYER_VIEW_LIMIT;
		} else {
			minRangeZ = 0;
			maxRangeZ = MAP_INIT_SURFACE_LAYER;
		}
	}

	const int_fast32_t min_y = centerPos.y + minRangeY;
	const int_fast32_t min_x = centerPos.x + minRangeX;
	const int_fast32_t max_y = centerPos.y + maxRangeY;
	const int_fast32_t max_x = centerPos.x + maxRangeX;

	const int32_t minoffset = centerPos.getZ() - maxRangeZ;
	const uint16_t x1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (min_x + minoffset)));
	const uint16_t y1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (min_y + minoffset)));

	const int32_t maxoffset = centerPos.getZ() - minRangeZ;
	const uint16_t x2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (max_x + maxoffset)));
	const uint16_t y2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (max_y + maxoffset)));

	const int32_t startx1 = x1 - (x1 % FLOOR_SIZE);
	const int32_t starty1 = y1 - (y1 % FLOOR_SIZE);
	const int32_t endx2 = x2 - (x2 % FLOOR_SIZE);
	const int32_t endy2 = y2 - (y2 % FLOOR_SIZE);

	const auto startLeaf = g_game().map.getQTNode(startx1, starty1);
	const QTreeLeafNode* leafS = startLeaf;
	const QTreeLeafNode* leafE;

	auto &specCache = hashmap.try_emplace(centerPos).first->second;
	auto &list = multifloor ? specCache.first : specCache.second;

	for (int_fast32_t ny = starty1; ny <= endy2; ny += FLOOR_SIZE) {
		leafE = leafS;
		for (int_fast32_t nx = startx1; nx <= endx2; nx += FLOOR_SIZE) {
			if (leafE) {
				const auto &node_list = (onlyPlayers ? leafE->player_list : leafE->creature_list);
				for (const auto creature : node_list) {
					const auto &cpos = creature->getPosition();
					if (minRangeZ > cpos.z || maxRangeZ < cpos.z) {
						continue;
					}

					int_fast16_t offsetZ = Position::getOffsetZ(centerPos, cpos);
					if ((min_y + offsetZ) > cpos.y || (max_y + offsetZ) < cpos.y || (min_x + offsetZ) > cpos.x || (max_x + offsetZ) < cpos.x) {
						continue;
					}

#ifdef SPECTATOR_USE_HASH_SET
					list.emplace(creature);
#else
					list.emplace_back(creature);
#endif
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

#ifdef SPECTATOR_USE_HASH_SET
	creatures.insert(list.begin(), list.end());
#else
	creatures.insert(creatures.end(), list.begin(), list.end());
#endif

	return *this;
}

void Spectators::insert(Creature* creature) {
#ifdef SPECTATOR_USE_HASH_SET
	creatures.emplace(creature);
#else
	creatures.emplace_back(creature);
#endif
}

bool Spectators::contains(const Creature* creature) const {
#ifdef SPECTATOR_USE_HASH_SET
	return creatures.contains(creature);
#else
	return std::find(creatures.begin(), creatures.end(), creature) != creatures.end();
#endif
}

template <class F>
bool Spectators::erase_if(F fnc) {
#ifdef SPECTATOR_USE_HASH_SET
	return phmap::erase_if(creatures, std::move(fnc)) > 0;
#else
	return std::erase_if(creatures, std::move(fnc)) > 0;
#endif
}

bool Spectators::erase(const Creature* creature) {
	update();
#ifdef SPECTATOR_USE_HASH_SET
	return creatures.erase(creature) > 0;
#else
	const auto &it = std::find(creatures.begin(), creatures.end(), creature);
	if (it != creatures.end()) {
		creatures.erase(it);
		return true;
	}
	return false;
#endif
}
