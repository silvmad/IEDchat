#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <ncurses.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <locale.h>

#include "commod.h"

enum {_W_CHAT, _W_CHAT_B, _W_INPUT, _W_INPUT_B};

// Codes d'erreur renvoyés par la fonction send_to_server
enum err {INV_CMD = 1, F_NEX};

#define MAX_LIGNE 1024
#define MAX_BUFF 1024
#define MAX_USERNAME 256
#define MAX_FNAME 256
#define MAX_PW 64

#define W_CHAT wins[_W_CHAT]
#define W_CHAT_B wins[_W_CHAT_B]
#define W_INPUT wins[_W_INPUT]
#define W_INPUT_B wins[_W_INPUT_B]

#define Last(X) ((X)[strlen(X) - 1])

void usage(string);
void analyse_ldc(int);
WINDOW ** init_ncurse(void);
int send_username(string, int);
int dial(string, string);
void user_input(WINDOW**, int, string, idx*);
void print_err(WINDOW*, enum err, string);
int key_input(WINDOW*, string, idx*);
int send_to_server(int, string, int*);
void server_input(int, WINDOW*, string, string*);
void rcpt_msg(int, WINDOW*);
void rcpt_name(int, WINDOW*, string);
void rcpt_info(int, WINDOW*);
void rcpt_ft(int, WINDOW*, string, string*);
int sock_read_line(string, int, int);
void cl_process_ftask(int, WINDOW*, string, string*);
void receive_ft(string, string, string, string*);
int create_dest_file(string);
void send_file(int, string, WINDOW*, string*);
void cl_process_ftnacc(int, WINDOW*);
void src_end_ft(int, WINDOW*);
void dest_end_ft(int, WINDOW*);
#endif
