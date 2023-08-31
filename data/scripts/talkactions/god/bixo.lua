local bixo = TalkAction("/bixo")

function bixo.onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	elseif player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end


	local chance = 0 -- chance de colocar um demon num sqm
	local distSummon = 7 -- um summon a cada 7 sqms (em x e em y)

	for i = 32357, 32657 do -- x inicial, x final
		for j = 32137, 32537 do -- y inicial, y final
			local position = Position({ x = i, y = j, z = 7 })
			if i % distSummon == 0 and j % distSummon == 0 then
				Game.createMonster("Demon", position, true, false, player)
			else
				if math.random(1, 100) <= 5 then
					Game.createMonster("Demon", position)
					position:sendMagicEffect(CONST_ME_TELEPORT)
				end

				if math.random(1, 100) <= chance then
					Game.createMonster("Cat", position)
					position:sendMagicEffect(CONST_ME_TELEPORT)
				end

				if math.random(1, 100) <= chance then
					Game.createMonster("Orc", position)
					position:sendMagicEffect(CONST_ME_TELEPORT)
				end

				if math.random(1, 100) <= chance then
					Game.createMonster("Elf", position)
					position:sendMagicEffect(CONST_ME_TELEPORT)
				end

				if math.random(1, 100) <= chance then
					Game.createMonster("Elf Scout", position)
					position:sendMagicEffect(CONST_ME_TELEPORT)
				end
			end
		end
	end
	return false
end

bixo:separator(" ")
bixo:groupType("god")
bixo:register()
