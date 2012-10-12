#include <string.h>
#include <new>
#include <engine/e_server_interface.h>
#include "gamecontext.hpp"
#include <game/collision.hpp>
#include <game/layers.hpp>
#include <engine/e_config.h>


extern "C"
{
	#include <engine/e_memheap.h>
}

GAMECONTEXT game;
TUNING_PARAMS tuning;

GAMECONTEXT::GAMECONTEXT()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		players[i] = 0;
	
	controller = 0;
	vote_closetime = 0;
	tunes = true;
}

GAMECONTEXT::~GAMECONTEXT()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		delete players[i];
}

void GAMECONTEXT::clear()
{
	bool cheats = this->cheats;
	bool tunes = this->tunes;
	this->~GAMECONTEXT();
	mem_zero(this, sizeof(*this));
	new (this) GAMECONTEXT();
	this->cheats = cheats;
	this->tunes = tunes;
}


class CHARACTER *GAMECONTEXT::get_player_char(int client_id)
{
	if(client_id < 0 || client_id >= MAX_CLIENTS || !players[client_id])
		return 0;
	return players[client_id]->get_character();
}

void GAMECONTEXT::create_damageind(vec2 p, float angle, int amount)
{
	float a = 3 * 3.14159f / 2 + angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(amount+2));
		NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)events.create(NETEVENTTYPE_DAMAGEIND, sizeof(NETEVENT_DAMAGEIND));
		if(ev)
		{
			ev->x = (int)p.x;
			ev->y = (int)p.y;
			ev->angle = (int)(f*256.0f);
		}
	}
}

void GAMECONTEXT::create_hammerhit(vec2 p)
{
	// create the event
	NETEVENT_HAMMERHIT *ev = (NETEVENT_HAMMERHIT *)events.create(NETEVENTTYPE_HAMMERHIT, sizeof(NETEVENT_HAMMERHIT));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}


void GAMECONTEXT::create_explosion(vec2 p, int owner, int weapon, bool bnodamage)
{
	// create the event
	NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)events.create(NETEVENTTYPE_EXPLOSION, sizeof(NETEVENT_EXPLOSION));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}

	if (!bnodamage)
	{
		// deal damage
		CHARACTER *ents[64];
		float radius = 135.0f;
		float innerradius = 48.0f;
		int num = game.world.find_entities(p, radius, (ENTITY**)ents, 64, NETOBJTYPE_CHARACTER);
		for(int i = 0; i < num; i++)
		{
			vec2 diff = ents[i]->pos - p;
			vec2 forcedir(0,1);
			float l = length(diff);
			if(l)
				forcedir = normalize(diff);
			l = 1-clamp((l-innerradius)/(radius-innerradius), 0.0f, 1.0f);
		}
	}
}

/*
void create_smoke(vec2 p)
{
	// create the event
	EV_EXPLOSION *ev = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}*/

void GAMECONTEXT::create_playerspawn(vec2 p)
{
	// create the event
	NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)events.create(NETEVENTTYPE_SPAWN, sizeof(NETEVENT_SPAWN));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}

void GAMECONTEXT::create_death(vec2 p, int cid)
{
	// create the event
	NETEVENT_DEATH *ev = (NETEVENT_DEATH *)events.create(NETEVENTTYPE_DEATH, sizeof(NETEVENT_DEATH));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
		ev->cid = cid;
	}
}

void GAMECONTEXT::create_sound(vec2 pos, int sound, int mask)
{
	if (sound < 0)
		return;

	// create a sound
	NETEVENT_SOUNDWORLD *ev = (NETEVENT_SOUNDWORLD *)events.create(NETEVENTTYPE_SOUNDWORLD, sizeof(NETEVENT_SOUNDWORLD), mask);
	if(ev)
	{
		ev->x = (int)pos.x;
		ev->y = (int)pos.y;
		ev->soundid = sound;
	}
}

void GAMECONTEXT::create_sound_global(int sound, int target)
{
	if (sound < 0)
		return;

	NETMSG_SV_SOUNDGLOBAL msg;
	msg.soundid = sound;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(target);
}


void GAMECONTEXT::send_chat_target(int to, const char *text)
{
	NETMSG_SV_CHAT msg;
	msg.team = 0;
	msg.cid = -1;
	msg.message = text;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(to);
}


void GAMECONTEXT::send_chat(int chatter_cid, int team, const char *text)
{
	/*
	if(chatter_cid >= 0 && chatter_cid < MAX_CLIENTS)
		dbg_msg("chat", "%d:%d:%s: %s", chatter_cid, team, server_clientname(chatter_cid), text);
	else
		dbg_msg("chat", "*** %s", text);
	*/

	if(team == CHAT_ALL)
	{
		NETMSG_SV_CHAT msg;
		msg.team = 0;
		msg.cid = chatter_cid;
		msg.message = text;
		msg.pack(MSGFLAG_VITAL);
		server_send_msg(-1);
	}
	else
	{
		NETMSG_SV_CHAT msg;
		msg.team = 1;
		msg.cid = chatter_cid;
		msg.message = text;
		msg.pack(MSGFLAG_VITAL);

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(game.players[i] && game.players[i]->team == team)
				server_send_msg(i);
		}
	}
}

void GAMECONTEXT::send_emoticon(int cid, int emoticon)
{
	NETMSG_SV_EMOTICON msg;
	msg.cid = cid;
	msg.emoticon = emoticon;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(-1);
}

void GAMECONTEXT::send_weapon_pickup(int cid, int weapon)
{
	NETMSG_SV_WEAPONPICKUP msg;
	msg.weapon = weapon;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(cid);
}


void GAMECONTEXT::send_broadcast(const char *text, int cid)
{
	NETMSG_SV_BROADCAST msg;
	msg.message = text;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(cid);
}

// 
void GAMECONTEXT::start_vote(const char *desc, const char *command)
{
	// check if a vote is already running
	if(vote_closetime)
		return;

	// reset votes
	vote_enforce = VOTE_ENFORCE_UNKNOWN;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i])
			players[i]->vote = 0;
	}
	
	// start vote
	vote_closetime = time_get() + time_freq()*25;
	str_copy(vote_description, desc, sizeof(vote_description));
	str_copy(vote_command, command, sizeof(vote_description));
	send_vote_set(-1);
	send_vote_status(-1);
}


void GAMECONTEXT::end_vote()
{
	vote_closetime = 0;
	send_vote_set(-1);
}

void GAMECONTEXT::send_vote_set(int cid)
{
	NETMSG_SV_VOTE_SET msg;
	if(vote_closetime)
	{
		msg.timeout = (vote_closetime-time_get())/time_freq();
		msg.description = vote_description;
		msg.command = vote_command;
	}
	else
	{
		msg.timeout = 0;
		msg.description = "";
		msg.command = "";
	}
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(cid);
}

void GAMECONTEXT::send_vote_status(int cid)
{
	NETMSG_SV_VOTE_STATUS msg = {0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i])
		{
			msg.total++;
			if(players[i]->vote > 0)
				msg.yes++;
			else if(players[i]->vote < 0)
				msg.no++;
			else
				msg.pass++;
		}
	}	

	msg.pack(MSGFLAG_VITAL);
	server_send_msg(cid);
	
}

void GAMECONTEXT::abort_vote_kick_on_disconnect(int client_id)
{
	if(vote_closetime && !strncmp(vote_command, "kick ", 5) && atoi(&vote_command[5]) == client_id)
		vote_closetime = -1;
}

void GAMECONTEXT::tick()
{
	world.core.tuning = tuning;
	world.tick();

	//if(world.paused) // make sure that the game object always updates
	controller->tick();
		
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i])
			players[i]->tick();
	}
	
	// update voting
	if(vote_closetime)
	{
		// abort the kick-vote on player-leave
		if(vote_closetime == -1)
		{
			send_chat(-1, GAMECONTEXT::CHAT_ALL, "Vote aborted");
			end_vote();
		}
		else
		{
			// count votes
			int total = 0, yes = 0, no = 0;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(players[i])
				{
					total++;
					if(players[i]->vote > 0)
						yes++;
					else if(players[i]->vote < 0)
						no++;
				}
			}
		
			if(vote_enforce == VOTE_ENFORCE_YES || yes >= total/2+1)// || (time_get() > vote_closetime && yes>no))
			{
				console_execute_line(vote_command,-1);
				end_vote();
				send_chat(-1, GAMECONTEXT::CHAT_ALL, "Vote passed");
			
				if(players[vote_creator])
					players[vote_creator]->last_votecall = 0;
			}
			else if(vote_enforce == VOTE_ENFORCE_NO || time_get() > vote_closetime || no >= total/2+1 || yes+no == total)
			{
				end_vote();
				send_chat(-1, GAMECONTEXT::CHAT_ALL, "Vote failed");
			}
		}
	}
}

void GAMECONTEXT::snap(int client_id)
{
	world.snap(client_id);
	controller->snap(client_id);
	events.snap(client_id);
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i])
			players[i]->snap(client_id);
	}
}

//handle gametype
GAMECONTROLLER_RACE::GAMECONTROLLER_RACE()
{
	gametype = "[S]-DDRace";
}

void GAMECONTROLLER_RACE::tick()
{
	GAMECONTROLLER::tick();
}

//handle game (hooks)

struct VOTEOPTION
{
	VOTEOPTION *next;
	VOTEOPTION *prev;
	char command[1];
};

static HEAP *voteoption_heap = 0;
static VOTEOPTION *voteoption_first = 0;
static VOTEOPTION *voteoption_last = 0;

void send_tuning_params(int cid)
{
	msg_pack_start(NETMSGTYPE_SV_TUNEPARAMS, MSGFLAG_VITAL);
	int *params = (int *)&tuning;
	for(unsigned i = 0; i < sizeof(tuning)/sizeof(int); i++)
		msg_pack_int(params[i]);
	msg_pack_end();
	server_send_msg(cid);
}

// Server hooks
void mods_client_direct_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id]->on_direct_input((NETOBJ_PLAYER_INPUT *)input);
}

void mods_set_authed(int client_id)
{
	if(game.players[client_id])
		game.players[client_id]->authed = true;
}

void mods_set_logout(int client_id)
{
	if(game.players[client_id])
	{
		if(game.players[client_id]->authed == 1) 
		{
			game.players[client_id]->authed = 0;
		}
	}
}

void mods_set_resistent(int client_id, int status)
{
	if(game.players[client_id])
		game.players[client_id]->resistent = status;
}

void mods_client_predicted_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id]->on_predicted_input((NETOBJ_PLAYER_INPUT *)input);
}

// Server hooks
void mods_tick()
{
	game.tick();

#ifdef CONF_DEBUG
	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			NETOBJ_PLAYER_INPUT input = {0};
			input.direction = (i&1)?-1:1;
			game.players[MAX_CLIENTS-i-1]->on_predicted_input(&input);
		}
	}
#endif
}

void mods_snap(int client_id)
{
	game.snap(client_id);
}

void mods_client_enter(int client_id)
{
	game.players[client_id]->respawn();
	dbg_msg("New-Player", "Join Player: %d Name: %s", client_id, server_clientname(client_id));

	game.send_chat_target(client_id, "DDRace by Stitch626 for Small Server Hosting");	
	char buf[512];
	str_format(buf, sizeof(buf), "%s entered and joined the Server", server_clientname(client_id));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf); 

	if(config.sv_welcome[0]==0)
	{}
	else
		game.send_chat_target(client_id, config.sv_welcome);

	dbg_msg("Team", "Cid: %d Player: %s   Join Team: %d", client_id, server_clientname(client_id), game.players[client_id]->team);
}

void mods_connected(int client_id)
{
	game.players[client_id] = new(client_id) PLAYER(client_id);
	if(config.sv_tournament_mode)
		game.players[client_id]->team = -1;
	else
		game.players[client_id]->team = 0;

	// send motd
	NETMSG_SV_MOTD msg;
	msg.message = config.sv_motd;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(client_id);
}

void mods_client_drop(int client_id)
{
	game.abort_vote_kick_on_disconnect(client_id);
	game.players[client_id]->on_disconnect();
	delete game.players[client_id];
	game.players[client_id] = 0;
}

void mods_message(int msgtype, int client_id)
{
	void *rawmsg = netmsg_secure_unpack(msgtype);
	PLAYER *p = game.players[client_id];
	
	if(!rawmsg)
	{
		return;
	}
	
	if(msgtype == NETMSGTYPE_CL_SAY)
	{
		NETMSG_CL_SAY *msg = (NETMSG_CL_SAY *)rawmsg;
		int team = msg->team;
		if(team)
			team = p->team;
		else
			team = GAMECONTEXT::CHAT_ALL;
		
		if(p->last_chat+time_freq() < time_get())
		{
			if(str_length(msg->message)>370)
			game.send_chat_target(client_id,"Your Message is too long");
			if(msg->message[0] == '/')
			{
				if(!str_comp_nocase(msg->message, "/info"))
				{
					game.send_chat_target(client_id, "|-----------------------------INFO----------------|");
					game.send_chat_target(client_id, "|   Small DDRace mod by Stitch626");
					game.send_chat_target(client_id, "|   Copyright © Stitch626  {626}");
					game.send_chat_target(client_id, "|   Beta Versioned: 1.0  Build Date:  23.3.2012");
					game.send_chat_target(client_id, "|      /info /rank /top5");
					game.send_chat_target(client_id, "|-----------------------------INFO----------------|");
				}
				else if(!strncmp(msg->message, "/top5", 5) && !config.sv_championship) 
				{
					const char *pt = msg->message;
					int number = 0;
					pt += 6;
					while(*pt && *pt >= '0' && *pt <= '9')
					{
						number = number*10+(*pt-'0');
						pt++;
					}
					if(number)
						((GAMECONTROLLER_RACE*)game.controller)->score.top5_draw(client_id, number);
					else
						((GAMECONTROLLER_RACE*)game.controller)->score.top5_draw(client_id, 0);
				}
				else if(!str_comp_nocase(msg->message, "/rank"))
				{
					char buf[512];
					const char *name = msg->message;
					name += 6;
					int pos=0;
					PLAYER_SCORE *pscore;
					
					pscore = ((GAMECONTROLLER_RACE*)game.controller)->score.search_name(server_clientname(client_id), pos);

					if(pscore && pos > -1 && pscore->score != -1)
					{
						float time = pscore->score;

						str_format(buf, sizeof(buf), "%d. %s",!config.sv_championship?pos:0, pscore->name);
						if (!config.sv_championship)
							game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
						else
							game.send_chat_target(client_id, buf);

						if ((int)time/60 >= 1)
							str_format(buf, sizeof(buf), "Time: %d minute%s %f seconds", (int)time/60, (int)time/60 != 1 ? "s" : "" , time-((int)time/60)*60);
						else
							str_format(buf, sizeof(buf), "Time: %f seconds", time-((int)time/60)*60);
					}
					else
						str_format(buf, sizeof(buf), "%s is not ranked", strcmp(msg->message, "/rank")?name:server_clientname(client_id));

					game.send_chat_target(client_id, buf);
				}
				else
					game.send_chat_target(client_id, "No such command!");
				
			}
			else
				if (game.players[client_id]->muted==0)
					game.send_chat(client_id, team, msg->message);
				else
					game.send_chat_target(client_id, "You are muted yet.");
			game.players[client_id]->last_chat = time_get();
		}
	}
	else if(msgtype == NETMSGTYPE_CL_CALLVOTE)
		{
		int64 now = time_get();
		if(game.vote_closetime)
		{
			game.send_chat_target(client_id, "Wait for current vote to end before calling a new one.");
			return;
		}
		
		int64 timeleft = p->last_votecall + time_freq()*60 - now;
		if(timeleft > 0)
		{
			char chatmsg[512] = {0};
			str_format(chatmsg, sizeof(chatmsg), "You must wait %d seconds before making another vote", (timeleft/time_freq())+1);
			game.send_chat_target(client_id, chatmsg);
			return;
		}
		
		char chatmsg[512] = {0};
		char desc[512] = {0};
		char cmd[512] = {0};
		NETMSG_CL_CALLVOTE *msg = (NETMSG_CL_CALLVOTE *)rawmsg;
		if(str_comp_nocase(msg->type, "option") == 0)
		{
			if (config.sv_votes==1)
			{
				VOTEOPTION *option = voteoption_first;
				while(option)
				{
					if(str_comp_nocase(msg->value, option->command) == 0)
					{
						str_format(chatmsg, sizeof(chatmsg), "%s called vote to change server option '%s'", server_clientname(client_id), option->command);
						str_format(desc, sizeof(desc), "%s", option->command);
						str_format(cmd, sizeof(cmd), "%s", option->command);
						break;
					}

					option = option->next;
				}
				
				if(!option)
				{
					str_format(chatmsg, sizeof(chatmsg), "'%s' isn't an option on this server", msg->value);
					game.send_chat_target(client_id, chatmsg);
					return;
				}
			}
		}
		else if(str_comp_nocase(msg->type, "kick") == 0)
		{
			if(!config.sv_vote_kick)
			{
				game.send_chat_target(client_id, "Server does not allow voting to kick players");
				return;
			}
			//int kick_id = atoi(msg->value);
			int cid1 = atoi(msg->value);
			if(cid1 < 0 || cid1 >= MAX_CLIENTS || !game.players[cid1])
			{
				game.send_chat_target(client_id, "Invalid client id to kick");
				return;
			}
			if(game.players[cid1]->authed)
			{
				game.send_chat_target(client_id, "You can't kick Admins!");
				return;
			}
			
			str_format(chatmsg, sizeof(chatmsg), "%s called for vote to kick '%s'", server_clientname(client_id), server_clientname(cid1));
			str_format(desc, sizeof(desc), "Kick '%s'", server_clientname(cid1));
			str_format(cmd, sizeof(cmd), "kick %d", cid1);
			if (!config.sv_vote_kick_bantime)
				str_format(cmd, sizeof(cmd), "kick %d", cid1);
			else
				str_format(cmd, sizeof(cmd), "ban %d %d", cid1, config.sv_vote_kick_bantime);
		}
		
		if(cmd[0])
		{
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, chatmsg);
			game.start_vote(desc, cmd);
			p->vote = 1;
			game.vote_creator = client_id;
			p->last_votecall = now;
			game.send_vote_status(-1);
		}
	}
	else if(msgtype == NETMSGTYPE_CL_VOTE)
	{
		if(!game.vote_closetime)
			return;

		if(p->vote == 0)
		{
			NETMSG_CL_VOTE *msg = (NETMSG_CL_VOTE *)rawmsg;
			p->vote = msg->vote;
			game.send_vote_status(-1);
			char eventname[32];
			if (p->vote ==-1)
				str_format(eventname,sizeof(eventname),"no");
			else if (p->vote ==1)
				str_format(eventname,sizeof(eventname),"yes");
		}
	}
	else if (msgtype == NETMSGTYPE_CL_SETTEAM && !game.world.paused)
	{
		NETMSG_CL_SETTEAM *msg = (NETMSG_CL_SETTEAM *)rawmsg;
		
		if(p->last_setteam+time_freq()*3 > time_get())
			return;

		// Switch team on given client and kill/respawn him
		p->last_setteam = time_get();
		p->set_team(msg->team);
	}
	else if (msgtype == NETMSGTYPE_CL_CHANGEINFO || msgtype == NETMSGTYPE_CL_STARTINFO)
	{
		NETMSG_CL_CHANGEINFO *msg = (NETMSG_CL_CHANGEINFO *)rawmsg;
		
		if(p->last_changeinfo+time_freq()*5 > time_get())
			return;
			
		p->last_changeinfo = time_get();
		if (!p->color_set) // Fluxid Color Change
		{
			p->use_custom_color = msg->use_custom_color;
			p->color_body = msg->color_body;
			p->color_feet = msg->color_feet;
			p->color_set = true;
		}

		// check for invalid chars
		unsigned char *name = (unsigned char *)msg->name;
		while (*name)
		{
			if(*name < 32)
				*name = ' ';
			name++;
		}

		// copy old name
		static int counter;
		char oldname[MAX_NAME_LENGTH];
		if(counter < 3)
		{
			str_copy(oldname, server_clientname(client_id), MAX_NAME_LENGTH);
			server_setclientname(client_id, msg->name);
				if(msgtype == NETMSGTYPE_CL_CHANGEINFO && strcmp(oldname, server_clientname(client_id)) != 0 && counter < 3)
				{
					char chattext[256];
					str_format(chattext, sizeof(chattext), "%s changed name to %s", oldname, server_clientname(client_id));
					game.send_chat(-1, GAMECONTEXT::CHAT_ALL, chattext);
					counter++;
				}
		}
		
		// set skin
		str_copy(p->skin_name, msg->skin, sizeof(p->skin_name));
		
		if(msgtype == NETMSGTYPE_CL_STARTINFO)
		{
			// send vote options
			NETMSG_SV_VOTE_CLEAROPTIONS clearmsg;
			clearmsg.pack(MSGFLAG_VITAL);
			server_send_msg(client_id);
			VOTEOPTION *current = voteoption_first;
			while(current)
			{
				NETMSG_SV_VOTE_OPTION optionmsg;
				optionmsg.command = current->command;
				optionmsg.pack(MSGFLAG_VITAL);
				server_send_msg(client_id);
				current = current->next;
			}
			
			// send tuning parameters to client
			send_tuning_params(client_id);

			//
			NETMSG_SV_READYTOENTER m;
			m.pack(MSGFLAG_VITAL|MSGFLAG_FLUSH);
			server_send_msg(client_id);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_EMOTICON && !game.world.paused)
	{
		NETMSG_CL_EMOTICON *msg = (NETMSG_CL_EMOTICON *)rawmsg;
		
		if(p->last_emote+time_freq()*3 > time_get())
			return;
			
		p->last_emote = time_get();
		
		game.send_emoticon(client_id, msg->emoticon);
	}
	else if (msgtype == NETMSGTYPE_CL_KILL && !game.world.paused)
	{
		if(p->last_kill+time_freq()*3 > time_get())
			return;
		
		p->last_kill = time_get();
		p->kill_character(WEAPON_SELF);
		p->respawn_tick = server_tick()+server_tickspeed()*3;
		if(game.controller->is_race())
			p->respawn_tick = server_tick();
	}
}

static void con_tune_param(void *result, void *user_data, int cid)
{
		const char *param_name = console_arg_string(result, 0);
		float new_value = console_arg_float(result, 1);

		if(tuning.set(param_name, new_value))
		{
			dbg_msg("tuning", "%s changed to %.2f", param_name, new_value);
			send_tuning_params(-1);
		}
		else
			server_send_rcon_line(cid, "No such tuning parameter");	
}

static void con_tune_reset(void *result, void *user_data, int cid)
{
		TUNING_PARAMS p;
		tuning = p;
		send_tuning_params(-1);
		dbg_msg("tuning"," resettet");
}

static void con_tune_dump(void *result, void *user_data, int cid)
{
	for(int i = 0; i < tuning.num(); i++)
	{
		float v;
		tuning.get(i, &v);
		char buf[512];
		str_format(buf, sizeof(buf), "%s %.2f", tuning.names[i], v);
			server_send_rcon_line(cid, buf);
	}
}

static void con_pause(void *result, void *user_data, int)
{
	game.world.paused = true;
	dbg_msg("Game", "Game paused by Admin");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, "Game paused by Admin");
}

static void con_start(void *result, void *user_data, int)
{
	game.world.paused = false;
	dbg_msg("Game", "Game started by Admin");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, "Game started by Admin");
}

static void con_freeze(void *result, void *user_data, int cid)
{
	
 	int cid1 = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(!game.players[cid1])
	{server_send_rcon_line(cid, "Invalid Client ID!");return;}
	int time = console_arg_int(result, 1)*server_tickspeed();
	if (game.players[cid1]->authed)
	{
		server_send_rcon_line(cid, "You can´t Freeze Admins!");
		return;
	}
	if(game.players[cid1])
	{
		CHARACTER* chr = game.players[cid1]->get_character();
		if(chr)
		{
			chr->freeze(time);
			chr->adminfreeze=true;
			char buf[512];
			str_format(buf, sizeof(buf), "%s Frozen by admin for %d seconds", server_clientname(cid1),console_arg_int(result, 1));
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
		}
	}
}
static void con_restart(void *result, void *user_data, int cid)
{
	if(console_arg_num(result))
		game.controller->do_warmup(console_arg_int(result, 0));
	else
		game.controller->startround();
}

static void con_say(void *result, void *user_data, int cid)
{
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, console_arg_string(result, 0));
}

static void con_kill_pl(void *result, void *user_data, int cid)
{
		int cid1 = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
		if(!game.players[cid1])
		{server_send_rcon_line(cid, "Invalid Client ID!");return;}

		if (game.players[cid1]->authed)
		{
			server_send_rcon_line(cid, "You can´t Kill Admins!");
			return;
		}

		game.players[cid1]->kill_character(WEAPON_GAME);
		char buf[512];
		str_format(buf, sizeof(buf), "%s killed by Console", server_clientname(cid1));
		game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}
static void con_mute(void *result, void *user_data, int cid)
{	
	int cid1 = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(!game.players[cid1])
	{server_send_rcon_line(cid, "Invalid Client ID!");return;}
	int seconds = console_arg_int(result, 1);
	char buf[512];
	if (game.players[cid1]->authed)
	{
		server_send_rcon_line(cid, "You can´t Mute Admins!");
		return;
	}
	if (seconds<10)
		seconds=10;

		if (game.players[cid1]->muted<seconds*server_tickspeed())
			game.players[cid1]->muted=seconds*server_tickspeed();
		else
			seconds=game.players[cid1]->muted/server_tickspeed();
		str_format(buf, sizeof(buf), "%s muted by Console for %d seconds", server_clientname(cid1), seconds);
		game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}
static void con_unmute(void *result, void *user_data, int cid)
{	
	int cid1 = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(!game.players[cid1])
	{server_send_rcon_line(cid, "Invalid Client ID!");return;}
	if(game.players[cid1]) 
	{
		game.players[cid1]->muted=0;
		char buf[512];
		str_format(buf, sizeof(buf), "%s unmuted by Console", server_clientname(cid1));
		game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
	}
}
static void con_addvote(void *result, void *user_data, int cid)
{
	int len = strlen(console_arg_string(result, 0));
	
	if(!voteoption_heap)
		voteoption_heap = memheap_create();
	
	VOTEOPTION *option = (VOTEOPTION *)memheap_allocate(voteoption_heap, sizeof(VOTEOPTION) + len);
	option->next = 0;
	option->prev = voteoption_last;
	if(option->prev)
		option->prev->next = option;
	voteoption_last = option;
	if(!voteoption_first)
		voteoption_first = option;
	
	mem_copy(option->command, console_arg_string(result, 0), len+1);
	dbg_msg("Votes", "added option '%s'", option->command);

 NETMSG_SV_VOTE_OPTION optionmsg;
 optionmsg.command = option->command;
 optionmsg.pack(MSGFLAG_VITAL);
 server_send_msg(-1);

}

static void con_vote(void *result, void *user_data, int cid)
{
	if(str_comp_nocase(console_arg_string(result, 0), "yes") == 0)
		game.vote_enforce = GAMECONTEXT::VOTE_ENFORCE_YES;
	else if(str_comp_nocase(console_arg_string(result, 0), "no") == 0)
		game.vote_enforce = GAMECONTEXT::VOTE_ENFORCE_NO;
	dbg_msg("Votes", "forcing vote %s", console_arg_string(result, 0));
}

void mods_console_init()
{	
	MACRO_REGISTER_COMMAND("tune", "si", CFGFLAG_SERVER, con_tune_param, 0, "");
	MACRO_REGISTER_COMMAND("tune_reset", "", CFGFLAG_SERVER, con_tune_reset, 0, "");
	MACRO_REGISTER_COMMAND("tune_dump", "", CFGFLAG_SERVER, con_tune_dump, 0, "");
	MACRO_REGISTER_COMMAND("restart", "i", CFGFLAG_SERVER, con_restart, 0, "");
	MACRO_REGISTER_COMMAND("say", "r", CFGFLAG_SERVER, con_say, 0, "");
	MACRO_REGISTER_COMMAND("addvote", "r", CFGFLAG_SERVER, con_addvote, 0, "");
	MACRO_REGISTER_COMMAND("vote", "r", CFGFLAG_SERVER, con_vote, 0, "");
	MACRO_REGISTER_COMMAND("freeze", "ii", CFGFLAG_SERVER, con_freeze, 0, "");
	MACRO_REGISTER_COMMAND("kill_pl", "i", CFGFLAG_SERVER, con_kill_pl, 0, "");
	MACRO_REGISTER_COMMAND("kill", "i", CFGFLAG_SERVER, con_kill_pl, 0, "");
	MACRO_REGISTER_COMMAND("pause", "", CFGFLAG_SERVER, con_pause, 0, "");
	MACRO_REGISTER_COMMAND("start", "", CFGFLAG_SERVER, con_start, 0, "");
	MACRO_REGISTER_COMMAND("mute", "ii", CFGFLAG_SERVER, con_mute, 0, "");
	MACRO_REGISTER_COMMAND("unmute", "i", CFGFLAG_SERVER, con_unmute, 0, "");
}

void mods_init()
{
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		snap_set_staticsize(i, netobj_get_size(i));
	layers_init();
	col_init();
	char buf[512];

	str_format(buf, sizeof(buf), "data/maps/%s.cfg", config.sv_map);
	console_execute_file(buf);

	str_format(buf, sizeof(buf), "data/maps/%s.map.cfg", config.sv_map);
	console_execute_file(buf);

	str_format(buf, sizeof(buf), "maps/%s.cfg", config.sv_map);
	console_execute_file(buf);

	str_format(buf, sizeof(buf), "maps/%s.map.cfg", config.sv_map);
	console_execute_file(buf);

	game.controller = new GAMECONTROLLER_RACE;

	// setup core world
	//for(int i = 0; i < MAX_CLIENTS; i++)
	//	game.players[i].core.world = &game.world.core;

	// create all entities from the game layer
	MAPITEM_LAYER_TILEMAP *tmap = layers_game_layer();
	TILE *tiles = (TILE *)map_get_data(tmap->data);
	
	for(int y = 0; y < tmap->height; y++)
	{
		for(int x = 0; x < tmap->width; x++)
		{
			int index = tiles[y*tmap->width+x].index;
			
			if(index >= ENTITY_OFFSET)
			{
				game.controller->on_entity(index-ENTITY_OFFSET, x ,y);
				//game.controller->on_entity(game.controller->get_index(x,y), pos, x ,y);
			}
		}
	}
	//
	//game.world.insert_entity(game.controller);

#ifdef CONF_DEBUG
	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			mods_connected(MAX_CLIENTS-i-1);
			mods_client_enter(MAX_CLIENTS-i-1);
			if(game.controller->is_teamplay())
				game.players[MAX_CLIENTS-i-1]->team = i&1;
		}
	}
#endif

}

void mods_shutdown()
{
	delete game.controller;
	game.controller = 0;
	game.clear();
}

void mods_presnap() {}
void mods_postsnap()
{
	game.events.clear();
}

extern "C" const char *mods_net_version() { return "0.5 b67d1f1a1eea234e"; }
extern "C" const char *mods_version() { return "0.5.2"; }
