/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
#include <game/mapitems.hpp>

#include <game/generated/g_protocol.hpp>

#include "entities/pickup.hpp"
#include "entities/light.hpp"
#include "entities/drager.hpp"
#include "entities/gun.hpp"
#include "entities/projectile.hpp"
#include "entities/door.hpp"
#include "entities/trigger.hpp"


#include "gamecontroller.hpp"
#include "gamecontext.hpp"
#include <game/layers.hpp>


GAMECONTROLLER::GAMECONTROLLER()
{
	gametype = "unknown";

	do_warmup(config.sv_warmup);
	map_wish[0] = 0;
	
	num_spawn_points[0] = 0;
	num_spawn_points[1] = 0;
	num_spawn_points[2] = 0;
}

GAMECONTROLLER::~GAMECONTROLLER()
{
}

float GAMECONTROLLER::evaluate_spawn_pos(SPAWNEVAL *eval, vec2 pos)
{
	float score = 0.0f;
	CHARACTER *c = (CHARACTER *)game.world.find_first(NETOBJTYPE_CHARACTER);
	for(; c; c = (CHARACTER *)c->typenext())
	{
		// team mates are not as dangerous as enemies
		float scoremod = 1.0f;
		if(eval->friendly_team != -1 && c->team == eval->friendly_team)
			scoremod = 0.5f;
			
		float d = distance(pos, c->pos);
		if(d == 0)
			score += 1000000000.0f;
		else
			score += 1.0f/d;
	}
	
	return score;
}

void GAMECONTROLLER::evaluate_spawn_type(SPAWNEVAL *eval, int t)
{
	// get spawn point
	for(int i  = 0; i < num_spawn_points[t]; i++)
	{
		vec2 p = spawn_points[t][i];
		float s = evaluate_spawn_pos(eval, p);
		if(!eval->got || eval->score > s)
		{
			eval->got = true;
			eval->score = s;
			eval->pos = p;
		}
	}
}

bool GAMECONTROLLER::can_spawn(PLAYER *player, vec2 *out_pos)
{
	SPAWNEVAL eval;
	
	// spectators can't spawn
	if(player->team == -1)
		return false;
	

	evaluate_spawn_type(&eval, 0);
	evaluate_spawn_type(&eval, 1);
	evaluate_spawn_type(&eval, 2);
	
	*out_pos = eval.pos;
	return eval.got;
}


bool GAMECONTROLLER::on_entity(int index, int x, int y)
{
	int type = -1;
	int subtype = 0;

	vec2 pos(x*32.0f+16.0f, y*32.0f+16.0f);

	int sides[8];
	sides[0]=get_index(x,y+1);
	sides[1]=get_index(x+1,y+1);
	sides[2]=get_index(x+1,y);
	sides[3]=get_index(x+1,y-1);
	sides[4]=get_index(x,y-1);
	sides[5]=get_index(x-1,y-1);
	sides[6]=get_index(x-1,y);
	sides[7]=get_index(x-1,y+1);
	
	if(index == ENTITY_SPAWN)
		spawn_points[0][num_spawn_points[0]++] = pos;
	else if(index == ENTITY_SPAWN_RED)
		spawn_points[1][num_spawn_points[1]++] = pos;
	else if(index == ENTITY_SPAWN_BLUE)
		spawn_points[2][num_spawn_points[2]++] = pos;
	else if(index >= ENTITY_LASER_FAST_CW && index <= ENTITY_LASER_FAST_CCW)
	{
		int sides2[8];
		sides2[0]=get_index(x,y+2);
		sides2[1]=get_index(x+2,y+2);
		sides2[2]=get_index(x+2,y);
		sides2[3]=get_index(x+2,y-2);
		sides2[4]=get_index(x,y-2);
		sides2[5]=get_index(x-2,y-2);
		sides2[6]=get_index(x-2,y);
		sides2[7]=get_index(x-2,y+2);

		float ang_speed;
		int ind=index-ENTITY_LASER_STOP;
		int m;
		if (ind<0)
		{
			ind=-ind;
			m=1;
		}
		else if(ind==0)
			m=0;
		else
			m=-1;

		
		if (ind==0)
			ang_speed=0.0f;
		else if (ind==1)
			ang_speed=pi/360;
		else if (ind==2)
			ang_speed=pi/180;
		else if (ind==3)
			ang_speed=pi/90;
		ang_speed*=m;

		for(int i=0; i<8;i++)
		{
			if (sides[i] >= ENTITY_LASER_SHORT && sides[i] <= ENTITY_LASER_LONG)
			{
				LIGHT *lgt = new LIGHT(pos, pi/4*i,32*3 + 32*(sides[i] - ENTITY_LASER_SHORT)*3);
				lgt->ang_speed=ang_speed;
				if (sides2[i]>=ENTITY_LASER_C_SLOW && sides2[i]<=ENTITY_LASER_C_FAST)
				{
					lgt->speed=1+(sides2[i]-ENTITY_LASER_C_SLOW)*2;
					lgt->cur_length=lgt->length;
				}
				else if(sides2[i]>=ENTITY_LASER_O_SLOW && sides2[i]<=ENTITY_LASER_O_FAST)
				{
					lgt->speed=1+(sides2[i]-ENTITY_LASER_O_SLOW)*2;
					lgt->cur_length=0;
				}
				else
					lgt->cur_length=lgt->length;
			}
		}
			
		
	}
	else if(index>=ENTITY_DRAGER_WEAK && index <=ENTITY_DRAGER_STRONG)
	{
		new DRAGER(pos,1.0f*(index-ENTITY_DRAGER_WEAK+1));
	}
	else if(index==ENTITY_PLASMA)
	{
		new GUN(pos);
	}
	else if(index>=ENTITY_CRAZY_SHOTGUN_U_EX && index<=ENTITY_CRAZY_SHOTGUN_L_EX)
	{
		for (int i = 0;i<4;i++)
		{
			if (index-ENTITY_CRAZY_SHOTGUN_U_EX==i)
			{
				float deg = i*(pi/2);
				PROJECTILE *proj = new PROJECTILE(WEAPON_SHOTGUN,
					-1,
					pos,
					vec2(sin(deg),cos(deg)),
					-2,
					true, PROJECTILE::PROJECTILE_FLAGS_EXPLODE,
					0, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);
				proj->bouncing=2-(i % 2);

			}
		}
	}
	else if(index>=ENTITY_CRAZY_SHOTGUN_U && index<=ENTITY_CRAZY_SHOTGUN_L)
	{
		for (int i = 0;i<4;i++)
		{
			if (index-ENTITY_CRAZY_SHOTGUN_U==i)
			{
			float deg = i*(pi/2);
			PROJECTILE *proj = new PROJECTILE(WEAPON_SHOTGUN,
				-1,
				pos,
				vec2(sin(deg),cos(deg)),
				-2,
				true, 0,
				0, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);
			proj->bouncing=2-(i % 2);
			}
		}
	}
	else if(index==ENTITY_DOOR)
	{
		for(int i=0; i<8;i++)
		{
			if (sides[i] >= ENTITY_LASER_SHORT && sides[i] <= ENTITY_LASER_LONG)
			{
				DOOR *target = new DOOR(pos, pi/4*i, 32*3 + 32*(sides[i] - ENTITY_LASER_SHORT)*3, false);
				for (int e=0; e<8;e++)
					if (e!=i)
						connector(x,y,target);
			}
		}
	}

	if(index == ENTITY_ARMOR_1)
		type = POWERUP_ARMOR;
	else if(index == ENTITY_HEALTH_1)
		type = POWERUP_HEALTH;
	else if(index == ENTITY_WEAPON_SHOTGUN)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_SHOTGUN;
	}
	else if(index == ENTITY_WEAPON_GRENADE)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_GRENADE;
	}
	else if(index == ENTITY_WEAPON_RIFLE)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_RIFLE;
	}
	else if(index == ENTITY_POWERUP_NINJA)
	{
		type = POWERUP_NINJA;
		subtype = WEAPON_NINJA;
	}
	if(index == ENTITY_WEAPON_RIFLE)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_RIFLE;
	}
	
	if(type != -1)
	{
		PICKUP *pickup = new PICKUP(type, subtype);
		pickup->pos = pos;
		return true;
	}

	return false;
}

int GAMECONTROLLER::get_index(int x, int y)
{
	MAPITEM_LAYER_TILEMAP *tmap = layers_game_layer();
	TILE *tiles = (TILE *)map_get_data(tmap->data);
	int index = tiles[y*tmap->width+x].index;
	return index-ENTITY_OFFSET;
}


void GAMECONTROLLER::endround()
{
	if(warmup) // game can't end when we are running warmup
		return;
		
	game.world.paused = true;
}

void GAMECONTROLLER::resetgame()
{
	game.world.reset_requested = true;
}

const char *GAMECONTROLLER::get_team_name(int team)
{
	if(team == 0)
		return "game";
	else
		return "spectators";
}

static void get_side_pos(int side, int &x, int &y)
{
	if (side==0)
	{
		x=0;
		y=1;
	}
	else if(side==1)
	{
		x=1;
		y=1;
	}
	else if(side==2)
	{
		x=1;
		y=0;
	}
	else if(side==3)
	{
		x=1;
		y=-1;
	}
	else if(side==4)
	{
		x=0;
		y=-1;
	}
	else if(side==5)
	{
		x=-1;
		y=-1;
	}
	else if(side==6)
	{
		x=-1;
		y=0;
	}
	else if(side==7)
	{
		x=-1;
		y=+1;
	}
}

void GAMECONTROLLER::connector(int x, int y, DOOR *target)
{
	int sides[8];
	sides[0]=game.controller->get_index(x,y+1);
	sides[1]=game.controller->get_index(x+1,y+1);
	sides[2]=game.controller->get_index(x+1,y);
	sides[3]=game.controller->get_index(x+1,y-1);
	sides[4]=game.controller->get_index(x,y-1);
	sides[5]=game.controller->get_index(x-1,y-1);
	sides[6]=game.controller->get_index(x-1,y);
	sides[7]=game.controller->get_index(x-1,y+1);
	for (int i=0;i<8;i++)
	{
		int tx;
		int ty;
		get_side_pos(i,tx,ty);
		if (sides[i]==ENTITY_CONNECTOR_D+(i+4) % 8)
			connector(x+tx,y+ty, target);
		else if(sides[i]==ENTITY_TRIGGER)
		{
			vec2 pos((x+tx)*32.0f+16.0f, (y+ty)*32.0f+16.0f);
			new TRIGGER(pos, target);
		}
	}



}

void GAMECONTROLLER::startround()
{
	resetgame();
	
	game.world.paused = false;

	dbg_msg("game","start round type='%s'", gametype);
}

void GAMECONTROLLER::change_map(const char *to_map)
{
	str_copy(map_wish, to_map, sizeof(map_wish));
	endround();
}

void GAMECONTROLLER::post_reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i])
		{
			game.players[i]->respawn();
		}
	}
}

void GAMECONTROLLER::on_character_spawn(class CHARACTER *chr)
{	
	// give default weapons
	chr->weapons[WEAPON_HAMMER].got = 1;
	chr->weapons[WEAPON_HAMMER].ammo = -1;
	chr->weapons[WEAPON_GUN].got = 1;
	chr->weapons[WEAPON_GUN].ammo = -1;
}

void GAMECONTROLLER::do_warmup(int seconds)
{
	warmup = seconds*server_tickspeed();
}

void GAMECONTROLLER::tick()
{
	// do warmup
	if(warmup)
	{
		warmup--;
		if(!warmup)
			startround();
	}
		
	server_setbrowseinfo(gametype, -1);
}

void GAMECONTROLLER::snap(int snapping_client)
{
	NETOBJ_GAME *gameobj = (NETOBJ_GAME *)snap_new_item(NETOBJTYPE_GAME, 0, sizeof(NETOBJ_GAME));
	gameobj->paused = game.world.paused;
	gameobj->warmup = warmup;
	gameobj->teamscore_red = 0;
	gameobj->teamscore_blue = 0;
}

int GAMECONTROLLER::clampteam(int team)
{
	if(team < 0) // spectator
		return -1;
	return  0;
}

GAMECONTROLLER *gamecontroller = 0;

bool GAMECONTROLLER::is_race() const
{	return true;   }