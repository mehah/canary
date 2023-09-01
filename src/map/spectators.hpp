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
#include <concepts>
#include <utility>

template <class Type, class BaseClass>
concept CheckType = std::is_base_of<BaseClass, Type>::value;

class Creature;
class Player;

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

	template <typename T, typename std::enable_if<std::is_same<Creature, T>::value || std::is_same<Player, T>::value>::type* = nullptr>
	Spectators find(const Position &centerPos, bool multifloor = false, int32_t minRangeX = 0, int32_t maxRangeX = 0, int32_t minRangeY = 0, int32_t maxRangeY = 0);

	template <typename T, typename std::enable_if<std::is_base_of<Creature, T>::value>::type* = nullptr>
	Spectators filter();

	bool contains(const Creature* creature) const;

	bool erase(const Creature* creature);

	template <class F>
	bool erase_if(F fnc);

	void insert(Creature* creature);

	bool empty() const {
		return creatures.empty();
	}

	size_t size() const {
		return creatures.size();
	}

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
