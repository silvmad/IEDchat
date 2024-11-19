#ifndef SERVEUR_H
#define SERVEUR_H

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "commod.h"

#define MAX_USERS 4
#define MAX_BUFF 1024
#define MAX_USERNAME 256
#define MAX_FNAME 256
#define MAX_PW 64

#define Last(X) ((X)[strlen(X) - 1])

/* Cette structure définit un utilisateur, csock est le socket utilisé pour
   la communication, fsock le socket utilisé pour le transfer de fichier, 
   name est le nom d'utilisateur. */
typedef struct {int csock; int fsock; string name;} user;

void usage(string);
void analyse_ldc(int);
int answer(string port);
int reset_fds(int, user*, idx, fd_set*);
void new_conn(int, user*, idx*, int*);
int check_users_fds(user*, idx*, fd_set*);
int client_input(int, string, user*, idx*);
void remove_user(int, user*, idx*);
int sock_read_line(string, int, int);
void process_msg(int, string, user*, idx);
void process_pbl_msg(int, string, user*, idx);
void process_pvt_msg(int, string, user*, idx);
void process_name(int, string, user*, idx);
void process_chgname(int, string, user*, idx);
void process_info(int, string, user*, idx*);
void send_usrlist(int, user*, idx);
void process_quit(int, string, user*, idx*);
void send_help(int);
void process_ft(int, string, user*, idx);
void srv_process_ftask(int, string, user*, idx);
void srv_process_ftacc(int, user*, idx);
void srv_process_ftnacc(int, user*, idx);
void transmit_file(int, user*, idx);
void process_ftend(int, user*, idx);
int get_usrcsock(string, user*, idx);

#endif
