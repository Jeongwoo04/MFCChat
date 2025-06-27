#pragma once

class Player
{
public:
	Protocol::PlayerInfo _info;
	weak_ptr<GameSession>	ownerSession; // Cycle
};

