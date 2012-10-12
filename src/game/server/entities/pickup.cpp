#include <engine/e_server_interface.h>
#include <engine/e_config.h>
#include <game/generated/g_protocol.hpp>
#include <game/server/gamecontext.hpp>
#include "pickup.hpp"

//////////////////////////////////////////////////
// pickup
//////////////////////////////////////////////////
PICKUP::PICKUP(int _type, int _subtype)
: ENTITY(NETOBJTYPE_PICKUP)
{
	type = _type;
	subtype = _subtype;
	proximity_radius = phys_size;

	reset();

	// TODO: should this be done here?
	game.world.insert_entity(this);
}

void PICKUP::reset()
{
	if (data->pickups[type].spawndelay > 0)
		spawntick = server_tick() + server_tickspeed() * data->pickups[type].spawndelay;
	else
		spawntick = -1;
}

void PICKUP::tick()
{
	// wait for respawn
	if(spawntick > 0)
	{
		if(server_tick() > spawntick)
		{
			// respawn
			spawntick = -1;

			if(type == POWERUP_WEAPON)
				game.create_sound(pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}
	// Check if a player intersected us
	CHARACTER *chr = game.world.closest_character(pos, 20.0f, 0);
	if(chr)
	{
	  //bool sound = false;
		// player picked us up, is someone was hooking us, let them go
		int respawntime = -1;
		switch (type)
		{//Waffen und freeze
		case POWERUP_HEALTH:
			//game.create_sound(pos, SOUND_PICKUP_HEALTH);
			//game.world.destroy_entity(this);
			chr->freeze(server_tickspeed()*3);
			break;
		case POWERUP_ARMOR:
		//	game.create_sound(pos, SOUND_PICKUP_ARMOR);
		//	game.world.destroy_entity(this);
			break;
			/*	case POWERUP_ARMOR:
					for(int i=WEAPON_SHOTGUN;i<NUM_WEAPONS;i++)
					{
						if (chr->weapons[i].got)
						{
							if(!(chr->freeze_time && i == WEAPON_NINJA))
							{
								chr->weapons[i].got = false;
								chr->weapons[i].ammo = 0;
								sound = true;
							}
						}
					}
					chr->ninja.activationdir=vec2(0,0);
					chr->ninja.activationtick=-500;
					chr->ninja.currentmovetime=0;
					if (sound)
					{
						chr->last_weapon = WEAPON_GUN;  
						game.create_sound(pos, SOUND_PICKUP_ARMOR);
					}
					if(!chr->freeze_time) chr->active_weapon = WEAPON_HAMMER;
					break;*/

		case POWERUP_WEAPON:
			if(subtype >= 0 && subtype < NUM_WEAPONS)
			{
				if(chr->weapons[subtype].ammo !=-1 || !chr->weapons[subtype].got)
				{
					chr->weapons[subtype].got = true;
					chr->weapons[subtype].ammo = -1;
					respawntime = server_tickspeed();

					// TODO: data compiler should take care of stuff like this
					if(subtype == WEAPON_GRENADE)
						game.create_sound(pos, SOUND_PICKUP_GRENADE);
					else if(subtype == WEAPON_SHOTGUN)
						game.create_sound(pos, SOUND_PICKUP_SHOTGUN);
					else if(subtype == WEAPON_RIFLE)
						game.create_sound(pos, SOUND_PICKUP_SHOTGUN);

					if(chr->player)
                    	game.send_weapon_pickup(chr->player->client_id, subtype);
				}
			}
			break;
		case POWERUP_NINJA:
			{
				// activate ninja on target player
				chr->ninja.activationtick = server_tick();
				chr->weapons[WEAPON_NINJA].got = true;
				chr->weapons[WEAPON_NINJA].ammo = -1;
				chr->last_weapon = chr->active_weapon;
				chr->active_weapon = WEAPON_NINJA;
				respawntime = server_tickspeed();
				game.create_sound(pos, SOUND_PICKUP_NINJA);

				// loop through all players, setting their emotes
				ENTITY *ents[64];
				int num = game.world.find_entities(vec2(0, 0), 1000000, ents, 64, NETOBJTYPE_CHARACTER);
				for (int i = 0; i < num; i++)
				{
					CHARACTER *c = (CHARACTER *)ents[i];
					if (c != chr)
					{
						c->emote_type = EMOTE_SURPRISE;
						c->emote_stop = server_tick() + server_tickspeed();
					}
				}

				chr->emote_type = EMOTE_ANGRY;
				chr->emote_stop = server_tick() + 1200 * server_tickspeed() / 1000;
				
				break;
			}
		default:
			break;
		};

		if(respawntime >= 0)
		{
			spawntick = server_tick() + server_tickspeed() ;
		}
	}
}

void PICKUP::snap(int snapping_client)
{
	if(spawntick != -1)
		return;

	NETOBJ_PICKUP *up = (NETOBJ_PICKUP *)snap_new_item(NETOBJTYPE_PICKUP, id, sizeof(NETOBJ_PICKUP));
	up->x = (int)pos.x;
	up->y = (int)pos.y;
	up->type = type; // TODO: two diffrent types? what gives?
	up->subtype = subtype;
}
