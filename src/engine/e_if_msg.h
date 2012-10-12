/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_MSG_H
#define ENGINE_IF_MSG_H

void msg_pack_start_system(int msg, int flags);

void msg_pack_start(int msg, int flags);

void msg_pack_int(int i);

void msg_pack_string(const char *p, int limit);

void msg_pack_raw(const void *data, int size);

void msg_pack_end();

typedef struct
{
	int msg;
	int flags;
	const unsigned char *data;
	int size;
} MSG_INFO;

const MSG_INFO *msg_get_info();

/* message unpacking */
int msg_unpack_start(const void *data, int data_size, int *is_system);

int msg_unpack_int();

const char *msg_unpack_string();

const unsigned char *msg_unpack_raw(int size);

int msg_unpack_error();


#endif
