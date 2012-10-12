#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.hpp>

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/

class GAMECONTROLLER
{
	vec2 spawn_points[3][64];
	int num_spawn_points[3];
protected:
	struct SPAWNEVAL
	{
		SPAWNEVAL()
		{
			got = false;
			friendly_team = -1;
			pos = vec2(100,100);
		}
			
		vec2 pos;
		bool got;
		int friendly_team;
		float score;
	};

	float evaluate_spawn_pos(SPAWNEVAL *eval, vec2 pos);
	void evaluate_spawn_type(SPAWNEVAL *eval, int type);
	bool evaluate_spawn(class PLAYER *p, vec2 *pos);

	void resetgame();
	
	char map_wish[128];
	
	int warmup;
	
public:
	const char *gametype;
	
	GAMECONTROLLER();
	virtual ~GAMECONTROLLER();
	
	void do_warmup(int seconds);
	
	void startround();
	void endround();
	void change_map(const char *to_map);

	/*
	
	*/	
	virtual void tick();
	
	virtual void snap(int snapping_client);
	
	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.
			
		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.
			
		Returns:
			bool?
	*/
	virtual bool on_entity(int index, int x, int y);

	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.
			
		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.
			
		Returns:
			bool?
	*/
	virtual int get_index(int x, int y);
	virtual void connector(int x, int y, class DOOR *target);
	/*
		Function: on_character_spawn
			Called when a character spawns into the game world.
			
		Arguments:
			chr - The character that was spawned.
	*/
	virtual void on_character_spawn(class CHARACTER *chr);

	//
	virtual bool can_spawn(class PLAYER *p, vec2 *pos);

	/*
	
	*/	
	virtual const char *get_team_name(int team);
	int clampteam(int team);

	virtual void post_reset();

	virtual bool is_race() const;
};

#endif
