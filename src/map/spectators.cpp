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
#include "game/game.hpp"

static phmap::flat_hash_map<Position, std::shared_ptr<PositionSpectator>> spectators;

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

void MultiSpectatorArea::find(const Position &centerPos, bool multifloor, bool onlyPlayers, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	if (locked) {
		throw std::runtime_error("Unable to search for spectators, MultiSpectatorArea is closed.");
	}

	const auto &specs = Spectators::get(centerPos, multifloor, onlyPlayers, minRangeX, maxRangeX, minRangeY, maxRangeY);
	if (creatures.empty()) {
		creatures = specs;
	} else {
		creatures.insert(creatures.end(), specs.begin(), specs.end());
	}
}

std::vector<Creature*> MultiSpectatorArea::get() {
	if (!locked) {
		locked = true;
		std::sort(creatures.begin(), creatures.end());
		creatures.erase(std::unique(creatures.begin(), creatures.end()), creatures.end());
	}

	return creatures;
}

std::vector<Creature*> PositionSpectator::get(bool multifloor, bool onlyPlayers, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	const auto &floor = multifloor ? multiFloor : currentFloor;
	const auto &creatures = onlyPlayers ? floor.players : floor.creatures;

	minRangeX = (minRangeX == 0 ? MAP_MAX_VIEW_PORT_X : minRangeX);
	maxRangeX = (maxRangeX == 0 ? MAP_MAX_VIEW_PORT_X : maxRangeX);
	minRangeY = (minRangeY == 0 ? MAP_MAX_VIEW_PORT_Y : minRangeY);
	maxRangeY = (maxRangeY == 0 ? MAP_MAX_VIEW_PORT_Y : maxRangeY);

	if (minRangeX == MAP_MAX_VIEW_PORT_X && maxRangeX == MAP_MAX_VIEW_PORT_X && minRangeY == MAP_MAX_VIEW_PORT_Y && maxRangeY == MAP_MAX_VIEW_PORT_Y) {
		return creatures;
	}

	std::vector<Creature*> newCreatures;
	for (const auto creature : creatures) {
		if (position.x - creature->getPosition().x >= -minRangeX
			&& position.y - creature->getPosition().y >= -minRangeY
			&& position.x - creature->getPosition().x <= maxRangeX
			&& position.y - creature->getPosition().y <= maxRangeY) {
			newCreatures.emplace_back(creature);
		}
	}

	return newCreatures;
}
void PositionSpectator::addCreature(Creature* creature) {
	if (!creature) {
		return;
	}

	multiFloor.creatures.emplace_back(creature);
	if (creature->getPlayer()) {
		multiFloor.players.emplace_back(creature);
	}

	if (creature->getPosition().z == position.z) {
		currentFloor.creatures.emplace_back(creature);
		if (creature->getPlayer()) {
			currentFloor.players.emplace_back(creature);
		}
	}
}

void PositionSpectator::removeCreature(Creature* creature) {
	if (!creature) {
		return;
	}

	auto it = std::find(multiFloor.creatures.begin(), multiFloor.creatures.end(), creature);
	if (it != multiFloor.creatures.end()) {
		multiFloor.creatures.erase(it);
	}

	if (creature->getPlayer()) {
		auto it = std::find(multiFloor.players.begin(), multiFloor.players.end(), creature);
		if (it != multiFloor.players.end()) {
			multiFloor.players.erase(it);
		}
	}

	if (creature->getPosition().z == position.z) {
		auto it = std::find(currentFloor.creatures.begin(), currentFloor.creatures.end(), creature);
		if (it != currentFloor.creatures.end()) {
			currentFloor.creatures.erase(it);
		}

		if (creature->getPlayer()) {
			auto it = std::find(currentFloor.players.begin(), currentFloor.players.end(), creature);
			if (it != currentFloor.players.end()) {
				currentFloor.players.erase(it);
			}
		}
	}
}

std::vector<Creature*> Spectators::get(const Position &centerPos, bool multifloor, bool onlyPlayers, int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY) {
	auto it = spectators.find(centerPos);
	if (it != spectators.end()) {
		return it->second->get(multifloor, onlyPlayers, minRangeX, maxRangeX, minRangeY, maxRangeY);
	}

	const int_fast32_t minY = centerPos.y - MAP_MAX_VIEW_PORT_Y;
	const int_fast32_t minX = centerPos.x - MAP_MAX_VIEW_PORT_X;
	const int_fast32_t maxY = centerPos.y + MAP_MAX_VIEW_PORT_Y;
	const int_fast32_t maxX = centerPos.x + MAP_MAX_VIEW_PORT_X;

	const auto &[minZ, maxZ] = getZMinMaxRange(centerPos.z, true);

	int32_t minoffset = centerPos.getZ() - maxZ;
	uint16_t x1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (minX + minoffset)));
	uint16_t y1 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (minY + minoffset)));

	int32_t maxoffset = centerPos.getZ() - minZ;
	uint16_t x2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (maxX + maxoffset)));
	uint16_t y2 = std::min<uint32_t>(0xFFFF, std::max<int32_t>(0, (maxY + maxoffset)));

	int32_t startx1 = x1 - (x1 % FLOOR_SIZE);
	int32_t starty1 = y1 - (y1 % FLOOR_SIZE);
	int32_t endx2 = x2 - (x2 % FLOOR_SIZE);
	int32_t endy2 = y2 - (y2 % FLOOR_SIZE);

	const auto &spec = std::make_shared<PositionSpectator>(centerPos);
	spectators.emplace(centerPos, spec);

	for (int_fast32_t ny = starty1; ny <= endy2; ++ny) {
		for (int_fast32_t nx = startx1; nx <= endx2; ++nx) {
			for (int_fast32_t nz = minZ; nz <= maxZ; ++nz) {
				if (auto tile = g_game().map.getTile(nx, ny, nz)) {
					const auto &creatures = tile->getCreatures();
					if (!creatures || creatures->empty()) {
						continue;
					}

					for (const auto creature : *tile->getCreatures()) {
						spec->addCreature(creature);
					}

					tile->positionSpectators.emplace_back(spec);
				}
			}
		}
	}

	return spec->get(multifloor, onlyPlayers, minRangeX, maxRangeX, minRangeY, maxRangeY);
}
