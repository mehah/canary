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

// #define SPECTATOR_USE_HASH_SET

#ifdef SPECTATOR_USE_HASH_SET
using SpectatorList = phmap::flat_hash_set<Creature*>;
#else
using SpectatorList = std::vector<Creature*>;
#endif

class Spectators {
public:
	static void clearCache();
	Spectators find(const Position &centerPos, bool multifloor = false, bool onlyPlayers = false, int32_t minRangeX = 0, int32_t maxRangeX = 0, int32_t minRangeY = 0, int32_t maxRangeY = 0);

	bool erase(const Creature* creature);

	auto begin() {
		update();
		return creatures.begin();
	}

	auto end() {
		return creatures.end();
	}

private:
	void update();

	SpectatorList creatures;
	bool needUpdate = false;
};
