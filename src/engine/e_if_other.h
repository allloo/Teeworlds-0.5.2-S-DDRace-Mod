/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_OTHER_H
#define ENGINE_IF_OTHER_H

/*
	Title: Engine Interface
*/

#include <base/system.h>

enum 
{
	SERVER_TICK_SPEED=50,
	MAX_CLIENTS=16,//max_clients definition!!!
	
	SNAP_CURRENT=0,
	SNAP_PREV=1,

	MASK_NONE=0,
	MASK_SET,
	MASK_ZERO,

	SNDFLAG_LOOP=1,
	SNDFLAG_POS=2,
	SNDFLAG_ALL=3,
	
	MAX_NAME_LENGTH=32
};

enum
{
	SRVFLAG_PASSWORD = 0x1,
};

/*
	Structure: SNAP_ITEM
*/
typedef struct
{
	int type;
	int id;
	int datasize;
} SNAP_ITEM;

/*
	Structure: CLIENT_INFO
*/
typedef struct
{
	const char *name;
	int latency;
} CLIENT_INFO;

typedef struct PERFORMACE_INFO_t
{
	const char *name;
	struct PERFORMACE_INFO_t *parent;
	struct PERFORMACE_INFO_t *first_child;
	struct PERFORMACE_INFO_t *next_child;
	int tick;
	int64 start;
	int64 total;
	int64 biggest;
	int64 last_delta;
} PERFORMACE_INFO;

void perf_init();
void perf_next();
void perf_start(PERFORMACE_INFO *info);
void perf_end();
void perf_dump();

int gfx_init();
void gfx_shutdown();
void gfx_swap();
int gfx_window_active();
int gfx_window_open();

void gfx_set_vsync(int val);

int snd_init();
int snd_shutdown();
int snd_update();

int map_load(const char *mapname);
void map_unload();

void map_set(void *m);

int map_is_loaded();

int map_num_items();

void *map_find_item(int type, int id);

void *map_get_item(int index, int *type, int *id);

void map_get_type(int type, int *start, int *num);

void *map_get_data(int index);

void *map_get_data_swapped(int index);

void *snap_new_item(int type, int id, int size);

int snap_num_items(int snapid);

void *snap_get_item(int snapid, int index, SNAP_ITEM *item);

void *snap_find_item(int snapid, int type, int id);

void snap_invalidate_item(int snapid, int index);

void snap_input(void *data, int size);

void snap_set_staticsize(int type, int size);

/* message packing */
enum
{
	MSGFLAG_VITAL=1,
	MSGFLAG_FLUSH=2,
	MSGFLAG_NORECORD=4,
	MSGFLAG_RECORD=8,
	MSGFLAG_NOSEND=16
};

int server_send_msg(int client_id); /* client_id == -1 == broadcast */

int client_send_msg();

int snap_new_id();

void snap_free_id(int id);

void map_unload_data(int index);

#endif
