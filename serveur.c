#include "serveur.h"

int main(int argc, string * argv)
{
  user users[MAX_USERS];
  memset(users, 0, sizeof(user) * MAX_USERS);
  idx iusr = 0;
  analyse_ldc(argc);
  int sock = answer(argv[1]);
  int retval;
  fd_set rfds;
  int maxsock = sock;
  while (1)
    {
      reset_fds(sock, users, iusr, &rfds);
      retval = select(maxsock + 1, &rfds, NULL, NULL, NULL);
      if (retval < 0)
	{
	  perror("select");
	  exit(1);
	}
      else
	{
	  if (FD_ISSET(sock, &rfds))
	    {
	      new_conn(sock, users, &iusr, &maxsock);
	    }
	  check_users_fds(users, &iusr, &rfds);
	}
    }
}

void usage(string message)
/* Affiche un message d'erreur et quitte */
{
  fprintf(stderr, "%s\n", message);
  exit(1);
}

void analyse_ldc(int argc)
/* Vérifie que les arguments sont corrects */
{
  if (argc > 2)
    {
      usage("Trop d'arguments.");
    }
  if (argc < 2)
    {
      usage("Pas assez d'arguments. Précisez le numéro de port.");
    }
}

int answer(string port)
/* Renvoie un socket lié au port en argument et met le programme
   en écoute sur ce socket. */
{
  struct addrinfo * info, * p;
  struct addrinfo indices;
  int fd, t;

  memset(&indices, 0, sizeof indices);
  indices.ai_flags = AI_PASSIVE;     // accepter de toutes les machines
  indices.ai_family = AF_INET;       // seulement IPv4
  indices.ai_socktype = SOCK_STREAM; // seulement TCP

  t = getaddrinfo(0, port, &indices, &info);
  if (t != 0)
    {
      usage("answer: cannot get info on port");
    }

  // Ouvrir la socket
  for(p = info; p != 0; p = p->ai_next)
    {
      fd = socket(p->ai_family, p->ai_socktype, 0); // fabriquer la socket
      if (fd >= 0)
	{
	  break;
	}
    }
  if (p == 0)
    {
      usage("answer: pas moyen d'avoir une socket");
    }

  t = 1; // re-utiliser l'adresse
  t = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof t);
  if (t < 0)
    {
      perror("setsockopt");
      fprintf(stderr, "answer: on continue quand meme\n");
    }

  if (bind(fd, p->ai_addr, p->ai_addrlen) < 0)
    { // donne lui un role
      close(fd);
      usage("answer: pas moyen de binder la socket");
    }

  freeaddrinfo(info);

  t = listen(fd, 1);	// on veut des connexions par ici
  if (t < 0)
    {
      close(fd);
      usage("answer: pas moyen d'ecouter sur la socket");
    }
  return fd;
}

int reset_fds(int sock, user * users, idx iusr, fd_set * rfdspt)
/* Réinitialise le set de descripteurs de fichiers sur lequel pointe rfdspt 
   et lui ajoute sock et tous les sockets contenus dans users. */
{
  FD_ZERO(rfdspt);
  FD_SET(sock, rfdspt);
  for (idx i = 0; i < iusr; i++)
    {
      if (users[i].csock)
	{
	  FD_SET(users[i].csock, rfdspt);
	}
      if (users[i].fsock)
	{
	  FD_SET(users[i].fsock, rfdspt);
	}
    }
  return 0;
}

void new_conn(int sock, user * users, idx * iusrpt, int * maxsockpt)
/* Procède à l'enregistrement d'une nouvelle  connexion.
   Si la connexion concerne un nouvel utilisateur et si le nombre maximum
   d'utilisateurs est atteint, le nouvel utilisateur est prévenu avant d'être
   déconnecté.
   Si la connexion est une connexion de données pour la transmission d'un
   fichier, le socket est ajouté dans le champ fsock de l'utilisateur 
   correspondant dans le tableau users. 
 */
{
  int t = accept(sock, NULL, NULL);
  if (t > *maxsockpt)
    {
      *maxsockpt = t;
    }
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, t);
  if (!strcmp(buffer, "NEW\n"))
    {
      if (*iusrpt == MAX_USERS)
	{
	  write(t, "UMAX\n", 5);
	  close(t);
	  return;
	}
      else
	{
	  write(t, "OK\n", 3);
	  users[(*iusrpt)++].csock = t;
	}
    }
  else if (!strcmp(buffer, "DATA\n"))
    {
      sock_read_line(buffer, MAX_BUFF, t);
      Last(buffer) = 0;
      for (idx i = 0; i < *iusrpt; i++)
	{
	  if (!strcmp(users[i].name, buffer))
	    {
	      users[i].fsock = t;
	      return;
	    }
	}
    }
}

int check_users_fds(user * users, idx * iusrpt, fd_set * rfdspt)
/* Parcourt le tableau users pour voir quels descripteurs de fichier peuvent
   être lus et utilise client_input pour les lire. */
{
  for (idx i = 0; i < *iusrpt; i++)
    {
      if (FD_ISSET(users[i].csock, rfdspt))
	{
	  client_input(users[i].csock, users[i].name, users, iusrpt);
	}
      if (FD_ISSET(users[i].fsock, rfdspt))
	{
	  client_input(users[i].fsock, users[i].name, users, iusrpt);
	}
    }
  return 0;
}

int client_input(int src_sock, string src_usrname, user * users, idx * iusrpt)
/* Lit ce qu'un client a envoyé et appelle la procédure de traitement
   correspondante. */
{
  char buffer[MAX_BUFF];
  int t;
  t = sock_read_line(buffer, MAX_BUFF, src_sock);
  if (t == -1)
    {
      remove_user(src_sock, users, iusrpt);
    }
  if (!strcmp(buffer, "MSG\n"))
    {
      process_msg(src_sock, src_usrname, users, *iusrpt);
    }
  else if (!strcmp(buffer, "NAME\n"))
    {
      process_name(src_sock, src_usrname, users, *iusrpt);
    }
  else if (!strcmp(buffer, "INFO\n"))
    {
      process_info(src_sock, src_usrname, users, iusrpt);
    }
  else if (!strcmp(buffer, "FT\n"))
    {
      process_ft(src_sock, src_usrname, users, *iusrpt);
    }
  return 0;
}

void remove_user(int sock, user * users, idx * iusrpt)
/* Si sock correspond au socket de communication d'un utilisateur, supprime
   cet utilisateur tableau users, ferme, le socket associé et réorganise le
   tableau users. 
   Si sock correspond au sock de données d'un utilisateur, ferme le socket
   et met à jour le tableau users pour mettre le socket de données de cet
   utilisateur à 0. */
{
  idx i = 0;
  while(users[i].csock != sock)
    {
      if (users[i].fsock == sock)
	{
	  users[i].fsock = 0;
	  close(sock);
	  return;
	}
      i++;
    }
  for (; i < *iusrpt and i < MAX_USERS - 1; i++)
    {
      users[i].name = users[i + 1].name;
      users[i].csock = users[i + 1].csock;
      users[i].fsock = users[i + 1].fsock;
    }
  if (i == (MAX_USERS - 1)) //Si users était plein, mise à 0 du dernier.
    {
      users[i].name = 0;
      users[i].csock = 0;
      users[i].fsock = 0;
    }
  (*iusrpt)--;
  close(sock);
}

void process_msg(int src_sock, string src_usrname, user * users, idx iusr)
/* Lit le type de message envoyé et appelle la fonction de traitement
   correspondante. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, src_sock);
  if (!strcmp(buffer, "PBL\n"))
    {
      process_pbl_msg(src_sock, src_usrname, users, iusr);
    }
  else if (!strcmp(buffer, "PVT\n"))
    {
      process_pvt_msg(src_sock, src_usrname, users, iusr);
    }
}

void process_pbl_msg(int src_sock, string src_usrname, user * users,
		     idx iusr)
/* Lit le message public envoyé par l'utilisateur et l'envoie à tous les
   utilisateurs. */
{
  char msg[MAX_BUFF];
  sock_read_line(msg, MAX_BUFF, src_sock);
  if (!strcmp(msg, "\n"))
    {
      return;
    }
  for (idx i = 0; i < iusr; i++)
    {
      write(users[i].csock, "MSG\nPBL\n", 8);
      write(users[i].csock, src_usrname, strlen(src_usrname));
      write(users[i].csock, "\n", 1);
      write(users[i].csock, msg, strlen(msg));
    }
}

void process_pvt_msg(int src_sock, string src_usrname, user * users,
		     idx iusr)
/* Lit le message privé envoyé par l'utilisateur et le nom du destinataire 
   puis l'envoie à son destinataire. */
{
  char dest_usrname[MAX_USERNAME];
  char msg[MAX_BUFF];
  sock_read_line(dest_usrname, MAX_USERNAME, src_sock);
  Last(dest_usrname) = 0;
  sock_read_line(msg, MAX_BUFF, src_sock);
  int dest_sock = get_usrcsock(dest_usrname, users, iusr);
  if (dest_sock > 0)
    {
      write(dest_sock, "MSG\nPVT\n", 8);
      write(dest_sock, src_usrname, strlen(src_usrname));
      write(dest_sock, "\n", 1);
      write(dest_sock, msg, strlen(msg));
      write(src_sock, "MSG\nPV_OK\n", 10);
      write(src_sock, dest_usrname, strlen(dest_usrname));
      write(src_sock, "\n", 1);
      write(src_sock, msg, strlen(msg));
    }
  else
    {
      write(src_sock, "MSG\nPV_NOK\n", 11);
      write(src_sock, dest_usrname, strlen(dest_usrname));
      write(src_sock, "\n", 1);
    }
}

void process_name(int src_sock, string src_usrname, user *users,
		  idx iusr)
/* Lit le type de requète sur le nom et appelle la fonction de traitement
   correspondante. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, src_sock);
  if (!strcmp(buffer, "CHG\n"))
    {
      process_chgname(src_sock, src_usrname, users, iusr);
    }
  /*else if (!strcmp(buffer, "SAVE\n"))
    {
      // Réservé pour une implémentation ultérieure.
      }*/
}

void process_chgname(int src_sock, string src_usrname, user * users,
		     idx iusr)
/* Lit le nom demandé par l'utilisateur et vérifie qu'il n'est pas déjà pris. 
   S'il est déjà pris l'utilisateur est averti.
   Sinon, l'utilisateur est également averti, le tableau users est mis à jour
   et un message est envoyé à tous les utilisateurs en fonction du contexte
   (changement de nom ou nouvelle connexion). */
{
  char buffer[MAX_BUFF];
  char msg[MAX_BUFF * 2];
  idx iclient;
  sock_read_line(buffer, MAX_BUFF, src_sock);
  Last(buffer) = 0;
  for (idx i = 0; i < iusr; i++)
    {
      //Si le nom existe déjà.
      if (users[i].name and !strcmp(buffer, users[i].name))
	{
	  write(src_sock, "NAME\nTAKEN\n", 11);
	  write(src_sock, buffer, strlen(buffer));
	  write(src_sock, "\n", 1);
	  return;
	}
      if (users[i].csock == src_sock)
	{
	  iclient = i;
	}
    }
  /*Le nom est valide, on signifie à l'utilisateur que son nom est accepté
    on avertit tout le monde du changement de nom ou de l'arrivée du 
    nouvel utilisateur et on met users à jour. */
  write(src_sock, "NAME\nOK\n", 8);
  write(src_sock, buffer, strlen(buffer));
  write(src_sock, "\n", 1);
  users[iclient].name = strdup(buffer);
  if (src_usrname == 0)
    {
      sprintf(msg, "%s vient de se connecter.\n\n", buffer);
      src_usrname = strndup(buffer, MAX_USERNAME);
    }
  else
    {
      sprintf(msg, "%s a changé son nom en %s\n\n", src_usrname, buffer);
    }
  for (idx i = 0; i < MAX_USERS; i++) 
    {
      if (users[i].name)
	{
	  write(users[i].csock, "INFO\n", 5);
	  write(users[i].csock, msg, strlen(msg));
	}
    }
}

void process_info(int src_sock, string src_usrname, user *users, idx *iusrpt)
/* Lit le type de demande d'information et appelle la fonction de traitement
   correspondante. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, src_sock);
  if (!strcmp(buffer, "LIST\n"))
    {
      send_usrlist(src_sock, users, *iusrpt);
    }
  else if (!strcmp(buffer, "HELP\n"))
    {
      send_help(src_sock);
    }
  else if (!strcmp(buffer, "QUIT\n"))
    {
      process_quit(src_sock, src_usrname, users, iusrpt);
    }
}

void send_usrlist(int sock, user * users, idx iusr)
/* Envoie la liste des utilisateurs connectés à l'utilisateur. */
{
  write(sock, "INFO\n", 5);
  write(sock, "Liste des utilisateurs connectés :\n", 36);
  for (idx i = 0; i < MAX_USERS; i++) 
    {
      if (users[i].csock)
	{
	  write(sock, users[i].name, strlen(users[i].name));
	  write(sock, " ", 1);
	}
    }
  write(sock, "\n\n", 2);
}

void send_help(int sock)
/* Envoie le résumé des commandes à l'utilisateur. */
{
  write(sock, "INFO\n", 5);
  string aide = "Liste des commandes :\n"
    "/t (ou /tell) nom msg : envoie msg en privé à nom.\n"
    "/l ou /list : affiche la liste des utilisateurs connectés.\n"
    "/h ou /help : affiche cette aide.\n"
    "/n (ou /name) nom : change le nom d'utilisateur pour nom.\n"
    "/q ou /quit : quitte.\n"
    "/f (ou /file) f nom : envoie le fichier f à nom.\n\n";
    write(sock, aide, strlen(aide));
}

void process_quit(int sock, string name, user * users, idx * iusrpt)
/* Déconnecte l'utilisateur, met à jour le tableau users et envoie un message
   aux utilisateurs restants. */
{
  char msg[MAX_BUFF];
  remove_user(sock, users, iusrpt);
  for (idx i = 0; i < *iusrpt; i ++)
    {
      sprintf(msg, "%s s'est déconnecté.\n\n", name);
      write(users[i].csock, "INFO\n", 5);
      write(users[i].csock, msg, strlen(msg));
    }
}

void process_ft(int src_sock, string src_usrname, user *users, idx iusr)
/* Lit le type de message de transfert de fichier et appelle la fonction de
   traitement correspondante. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, src_sock);
  if (!strcmp(buffer, "ASK\n"))
    {
      srv_process_ftask(src_sock, src_usrname, users, iusr);
    }
  if (!strcmp(buffer, "ACC\n"))
    {
      srv_process_ftacc(src_sock, users, iusr);
    }
  if (!strcmp(buffer, "NACC\n"))
    {
      srv_process_ftnacc(src_sock, users, iusr);
    }
  if (!strcmp(buffer, "START\n"))
    {
      transmit_file(src_sock, users, iusr);
    }
  if (!strcmp(buffer, "END\n"))
    {
      process_ftend(src_sock, users, iusr);
    }
}

void srv_process_ftask(int src_sock, string src_usr, user* users, idx iusr)
/* Lit et transmet la demande de transfert de fichier au destinataire. Envoie
   un message à l'utilisateur si le destinataire n'existe pas. */
{
  char fname[MAX_BUFF];
  char dest_usr[MAX_USERNAME];
  char msg[MAX_BUFF * 2];
  sock_read_line(fname, MAX_BUFF, src_sock);
  sock_read_line(dest_usr, MAX_USERNAME, src_sock);
  Last(dest_usr) = 0;
  int dest_sock = get_usrcsock(dest_usr, users, iusr);
  if (dest_sock > 0)
    {
      write(dest_sock, "FT\nASK\n", 7);
      write(dest_sock, fname, strlen(fname));
      write(dest_sock, src_usr, strlen(src_usr));
      write(dest_sock, "\n", 1);
    }
  else
    {
      sprintf(msg, "Impossible d'envoyer un fichier à %s : aucun utilisateur"
	      " de ce nom n'est connecté.\n\n", dest_usr);
      write(src_sock, "INFO\n", 5);
      write(src_sock, msg, strlen(msg));
    }
}

void srv_process_ftacc(int sock, user * users, idx iusr)
/* Lit et transmet le message d'acceptation du transfert à l'émetteur.*/
{
  char fname[MAX_BUFF];
  char src_usr[MAX_USERNAME];
  char dest_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_BUFF, sock);
  sock_read_line(src_usr, MAX_BUFF, sock);
  Last(src_usr) = 0;
  sock_read_line(dest_usr, MAX_BUFF, sock);
  int src_sock = get_usrcsock(src_usr, users, iusr);
  if (src_sock > 0)
    {
      write(src_sock, "FT\nACC\n", 7);
      write(src_sock, fname, strlen(fname));
      write(src_sock, dest_usr, strlen(dest_usr));
    }
}

void srv_process_ftnacc(int sock, user *users, idx iusr)
/* Lit et transmet le message de refus du transfert de fichier à l'émetteur. 
 */
{
  char fname[MAX_FNAME];
  char src_usr[MAX_USERNAME];
  char dest_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  sock_read_line(src_usr, MAX_USERNAME, sock);
  Last(src_usr) = 0;
  sock_read_line(dest_usr, MAX_USERNAME, sock);
  int src_sock = get_usrcsock(src_usr, users, iusr);
  if (src_sock > 0)
    {
      write(src_sock, "FT\nNACC\n", 8);
      write(src_sock, fname, strlen(fname));
      write(src_sock, dest_usr, strlen(dest_usr));
    }
}

void transmit_file(int src_sock, user * users, idx iusr)
/* Crée un processus enfant qui transmet le fichier au destinataire. */
{
  int t = fork();
  if (t == 0)
    {
      char fname[MAX_FNAME];
      char dest_usrname[MAX_USERNAME]; 
      sock_read_line(fname, MAX_BUFF, src_sock);
      sock_read_line(dest_usrname, MAX_USERNAME, src_sock); 
      Last(dest_usrname) = 0;
      int dest_sock;
      for (idx i = 0; i < iusr; i++)
	{
	  if (!strcmp(users[i].name, dest_usrname))
	    {
	      dest_sock = users[i].fsock;
	    }
	}
      char ftbuff[MAX_BUFF];
      t = read(src_sock, ftbuff, MAX_BUFF);
      while (t == MAX_BUFF)
	{
	  write(dest_sock, ftbuff, MAX_BUFF);
	  t = read(src_sock, ftbuff, MAX_BUFF);
	}
      if (t > 0) // Ecrire les derniers octets lus.
	{
	  write(dest_sock, ftbuff, t);
	}
      exit(0);
    }
  for (idx i = 0; i < iusr; i++)
    {
      if (users[i].fsock == src_sock)
	{
	  users[i].fsock = 0;
	  close(src_sock);
	  break;
	}
    }
}

void process_ftend(int sock, user *users, idx iusr)
/* Transmet le message de fin de transfert de fichier à l'émetteur et au 
   destinataire. */
{
  char fname[MAX_FNAME];
  char src_usrname[MAX_USERNAME];
  char dest_usrname[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  sock_read_line(src_usrname, MAX_USERNAME, sock);
  Last(src_usrname) = 0;
  sock_read_line(dest_usrname, MAX_USERNAME, sock);
  Last(dest_usrname) = 0;
  int src_csock = get_usrcsock(src_usrname, users, iusr);
  int dest_csock = get_usrcsock(dest_usrname, users, iusr);
  write(src_csock, "FT\nSRC_END\n", 11);
  write(src_csock, fname, strlen(fname));
  write(src_csock, dest_usrname, strlen(dest_usrname));
  write(src_csock, "\n", 1);
  write(dest_csock, "FT\nDEST_END\n", 12);
  write(dest_csock, fname, strlen(fname));
  write(dest_csock, src_usrname, strlen(src_usrname));
  write(dest_csock, "\n", 1);
}

int get_usrcsock(string name, user * users, idx iusr)
/* Renvoie le socket de communication de l'utilisateur dont le nom est name 
   ou -1 s'il n'y a pas d'utilisateur de ce nom dans le tableau users. */
{
  for (idx i = 0; i < iusr; i++)
    {
      if (!strcmp(users[i].name, name))
	{
	  return users[i].csock;
	}
    }
  return -1;
}
