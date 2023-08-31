/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include "pch.hpp"

class Creature;
struct Position;

class SpectatorsGroup {
public:
	void find(const Position &centerPos, bool multifloor = false, bool onlyPlayers = false, int32_t minRangeX = 0, int32_t maxRangeX = 0, int32_t minRangeY = 0, int32_t maxRangeY = 0);
	std::vector<Creature*> get();

private:
	std::vector<Creature*> creatures;
	bool update = false;
};

class Spectators {
public:
	Spectators(const Position &pos) :
		position(pos) { }

	std::vector<Creature*> get(bool multifloor = false, bool onlyPlayers = false, int32_t minRangeX = 0, int32_t maxRangeX = 0, int32_t minRangeY = 0, int32_t maxRangeY = 0);

	void addCreature(Creature* creature);
	void removeCreature(Creature* creature);
	std::vector<SpectatorsPtr> getWatchmen() const {
		return watchmen;
	}

private:
	struct Floor {
		std::vector<Creature*> creatures;
		std::vector<Creature*> players;
	};

	Position position;

	Floor currentFloor;
	Floor multiFloor;

	std::vector<SpectatorsPtr> watchmen;

	friend class SpectatorsCache;
};

class SpectatorsCache {
public:
	static std::vector<Creature*> get(const Position &centerPos, bool multifloor = false, bool onlyPlayers = false, int32_t minRangeX = 0, int32_t maxRangeX = 0, int32_t minRangeY = 0, int32_t maxRangeY = 0);
};
