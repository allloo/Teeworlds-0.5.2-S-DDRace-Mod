/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_server_interface.h>
#include <engine/e_config.h>
#include <game/generated/g_protocol.hpp>
#include <game/server/gamecontext.hpp>
#include "laser.hpp"

//////////////////////////////////////////////////
// laser
//////////////////////////////////////////////////
LASER::LASER(vec2 pos, vec2 direction, float start_energy, int owner, int type)
: ENTITY(NETOBJTYPE_LASER)
{
	this->pos = pos;
	this->owner = owner;
	this->type = type;
	energy = start_energy;
	dir = direction;
	bounces = 0;
	do_bounce();
	
	game.world.insert_entity(this);
}


bool LASER::hit_character(vec2 from, vec2 to)
{
	vec2 at;
	CHARACTER *owner_char = game.get_player_char(owner);
	CHARACTER *hit = game.world.intersect_character(pos, to, 0.0f, at, owner_char);
	if(!hit)
		return false;

	this->from = from;
	pos = at;
	energy = -1;

	if (type==1)
	{
		hit->core.vel+=normalize(owner_char->core.pos-hit->core.pos)*10;
	}
	else
	{
		if(hit->adminfreeze==false)
		{
		hit->unfreeze();
		}
	}

	return true;
}

void LASER::do_bounce()
{
	eval_tick = server_tick();
	
	if(energy < 0)
	{
		game.world.destroy_entity(this);
		return;
	}
	
	vec2 to = pos + dir*energy;
	vec2 org_to = to;
	vec2 coltile;
	
	int res;
	res = col_intersect_line(pos, to, &coltile, &to);
	
	if(res)
	{
		if(!hit_character(pos, to))
		{
			// intersected
			from = pos;
			pos = to;

			vec2 temp_pos = pos;
			vec2 temp_dir = dir*4.0f;

			int f;
			if(res == -1) {
				f = col_get(round(coltile.x), round(coltile.y));
				col_set(round(coltile.x), round(coltile.y), COLFLAG_SOLID);
			}
 			move_point(&temp_pos, &temp_dir, 1.0f, 0);
			if(res == -1) {
				col_set(round(coltile.x), round(coltile.y), f);
			}

			pos = temp_pos;
			dir = normalize(temp_dir);
			
			energy -= distance(from, pos) + tuning.laser_bounce_cost;
			bounces++;
			
			if(bounces > tuning.laser_bounce_num)
			{
				energy = -1;
			}

			//game.create_explosion(to, owner, -1, false);
				
			game.create_sound(pos, SOUND_RIFLE_BOUNCE);
		}
	}
	else
	{
		if(!hit_character(pos, to))
		{
			from = pos;
			pos = to;
			energy = -1;
		}
	}
		
	//dbg_msg("laser", "%d done %f %f %f %f", server_tick(), from.x, from.y, pos.x, pos.y);
}
	
void LASER::reset()
{
	game.world.destroy_entity(this);
}

void LASER::tick()
{
	if(server_tick() > eval_tick+(server_tickspeed()*tuning.laser_bounce_delay)/1000.0f)
	{
		do_bounce();
	}
}

void LASER::snap(int snapping_client)
{
	if(networkclipped(snapping_client))
		return;

	NETOBJ_LASER *obj = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id, sizeof(NETOBJ_LASER));
	obj->x = (int)pos.x;
	obj->y = (int)pos.y;
	obj->from_x = (int)from.x;
	obj->from_y = (int)from.y;
	obj->start_tick = eval_tick;
}
