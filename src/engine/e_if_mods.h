/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_MOD_H
#define ENGINE_IF_MOD_H

void mods_console_init();
void mods_init();

void mods_shutdown();

void mods_client_enter(int cid);

void mods_client_drop(int cid);

void mods_client_direct_input(int cid, void *input);

void mods_client_predicted_input(int cid, void *input);

void mods_tick();

void mods_presnap();

void mods_snap(int cid);

void mods_postsnap();

void mods_connected(int client_id);

const char *mods_net_version();

const char *mods_version();

void mods_message(int msg, int client_id);

void mods_set_authed(int client_id);

void mods_set_logout(int client_id);

void mods_set_resistent(int client_id, int status);

#endif
