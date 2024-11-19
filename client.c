#include "client.h"
#include "commod.h"

int main(int argc, string * argv)
{
  setlocale(LC_ALL, "fr_FR.UTF-8");
  analyse_ldc(argc);
  int sock = dial(argv[1], argv[2]);
  write(sock, "NEW\n", 4);
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, sock);
  if (!strcmp(buffer, "UMAX\n"))
    {
      usage("Impossible de se connecter : nombre maximum d'utilisateurs "
	    "atteint.");
    }
  char username[MAX_USERNAME];
  send_username(username, sock);
  WINDOW ** wins = init_ncurse();
  fd_set rfds;
  int retval;
  char ligne[MAX_LIGNE];
  idx il = 0;
  while (1)
    {
      wrefresh(W_INPUT); //Pour remettre le curseur dans la zone d'input.
      FD_ZERO(&rfds);
      FD_SET(0, &rfds);
      FD_SET(sock, &rfds);
      retval = select(sock + 1, &rfds, NULL, NULL, NULL);
      if (retval < 0)
	{
	  perror("select");
	  continue;
	}
      if (FD_ISSET(0, &rfds))
	{
	  user_input(wins, sock, ligne, &il);
	}
      if(FD_ISSET(sock, &rfds))
	{
	  server_input(sock, W_CHAT, username, argv); 
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
  if (argc > 3)
    {
      usage("Trop d'arguments.");
    }
  if (argc < 3)
    {
      usage("Pas assez d'arguments.");
    }
}

int send_username(string name, int sock)
/* Récupère le nom de l'utilisateur, et l'envoie au serveur pour 
   vérification.
   Recommence l'opération si le nom est déjà pris. 
   Met à jour le nom d'utilisateur une fois une fois celui-ci accepté par le
   serveur.*/
{
  char buffer[MAX_BUFF];
  printf("Entrez votre nom : ");
  fgets(name, MAX_USERNAME, stdin);
  write(sock, "NAME\nCHG\n", 9);
  write(sock, name, strlen(name));
  sock_read_line(buffer, MAX_BUFF, sock);
  sock_read_line(buffer + strlen(buffer), MAX_BUFF, sock);
  while (!strcmp(buffer, "NAME\nTAKEN\n"))
    {
      printf("Ce nom est déjà pris, veuillez en entrer un autre : ");
      fgets(name, MAX_USERNAME, stdin);
      write(sock, "NAME\nCHG\n", 9);
      write(sock, name, strlen(name));
      sock_read_line(buffer, MAX_BUFF, sock);
    }
  if (strcmp(buffer, "NAME\nOK\n"))
    {
      close(sock);
      usage("Erreur de communication avec le serveur.");
    }
  sock_read_line(name, MAX_USERNAME, sock);
  Last(name) = 0;
  return 0;
}
      

WINDOW ** init_ncurse(void)
/* Initialise l'interface ncurses */
{
  initscr(); // Démarre curses.
  cbreak(); //pas de line buffer
  noecho(); //pas d'echo
  keypad(stdscr, TRUE); //flèches, F1, F2...
  refresh();
  
  WINDOW **wins = malloc(sizeof(WINDOW*) * 4);
  W_CHAT = newwin(LINES - 5, COLS - 2, 1, 1);
  W_CHAT_B = newwin(LINES - 3, COLS, 0, 0);
  W_INPUT = newwin(1, COLS - 2, LINES - 2, 1);
  W_INPUT_B = newwin(3, COLS, LINES - 3, 0);
  
  box(W_CHAT_B, 0, 0);
  wrefresh(W_CHAT_B);
  box(W_INPUT_B, 0, 0);
  wrefresh(W_INPUT_B);
  wprintw(W_INPUT, "> ");
  wrefresh(W_INPUT);
  
  scrollok(W_CHAT, TRUE);
  scrollok(W_INPUT, TRUE);

  return wins;
}

int dial(string machine, string port)
/* Renvoie une connexion vers la machine et sur le port en argument */
{
  struct addrinfo * info, * p;
  struct addrinfo indices;
  int toserver, t;

  memset(&indices, 0, sizeof indices);
  indices.ai_family = AF_INET;       // seulement IPv4
  indices.ai_socktype = SOCK_STREAM; // seulement TCP

  t = getaddrinfo(machine, port, &indices, &info);
  if (t != 0)			// erreur
    {
      return -1;
    }

  // trouver une adresse qui fonctionne dans les reponses
  for(p = info; p != 0; p = p->ai_next)
    {
      toserver = socket(p->ai_family, p->ai_socktype, 0);
      if (toserver >= 0)
	break;
    }
  if (p == 0) // rien ne marche
    {
      return -1;
    }

  // la connecter
  t = connect(toserver, info->ai_addr, info->ai_addrlen);
  if (t < 0)
    {
      freeaddrinfo(info);
      close(toserver);
      fprintf(stderr, "pas pu connecter");
      perror("connect");
      return -1;
    }

  freeaddrinfo(info);
  return toserver;
}

void user_input(WINDOW ** wins, int sock, string ligne, idx *ilpt)
/* Récupère l'entrée utilisateur puis l'analyse et l'envoie au serveur si
   nécessaire. */
{
  int t;
  t = key_input(W_INPUT, ligne, ilpt);
  if (t == 1) // Envoie au serveur.
    {
      t = send_to_server(sock, ligne, ilpt);
      if (t > 0)
	{
	  print_err(W_CHAT, t, ligne);
	}
      memset(ligne, 0, MAX_LIGNE);
      *ilpt = 0;     
    }
}

void print_err(WINDOW * wchat, enum err code, string ligne)
/* Imprime un message d'erreur dans la fenêtre de chat correspondant au code
   d'erreur en argument. */
{
  if (code == INV_CMD)
    {
      char buffer[MAX_BUFF * 2];
      Last(ligne) = 0;
      sprintf(buffer, "La commande \"%s\" est invalide. Tapez /h pour un"
	      " récapitulatif des commandes.\n", ligne);
      wprintw(wchat, buffer);
    }
  else if (code == F_NEX)
    {
      wprintw(wchat, "Le fichier que vous tentez d'envoyer n'existe"
	      " pas.\n");
    }
  wrefresh(wchat);
}

int key_input(WINDOW * win_input, string ligne, idx * ilpt)
/* Lit l'entrée utilisateur, l'affiche dans la fenêtre d'input et remplit la
   chaîne ligne avec, ilpt est un pointeur vers l'index du prochain 
   caractère à écrire dans ligne. 
   Renvoie 1 si l'utilisateur a tapé entrée (déclenche l'envoi du message au 
   serveur) */
{
  int ch = getch();
  if (ch == KEY_BACKSPACE)
    {
      if (*ilpt == 0)
	{
	  return 0;
	}
      waddch(win_input, '\b');
      wdelch(win_input);
      // Permet d'effacer un caractère utf8 de liste.
      if (ligne[(*ilpt) - 1] < 0)
	{
	  // Tant que l'octet commençe par 10
	  while (ligne[(*ilpt) - 1] < -64) 
	    {
	      ligne[(*ilpt)--] = 0;
	    }
	  ligne[(*ilpt)--] = 0;
	}
      else
	{
	  ligne[(*ilpt)--] = 0;
	}
    }
  else if (ch <= 255 and ch >= 0)
    {
      waddch(win_input, ch);
      ligne[(*ilpt)++] = ch;
      if (ch == '\n')
	{
	  ligne[*ilpt] = 0;
	  wprintw(win_input, "> ");
	  wrefresh(win_input);
	  return 1;
	}
    }
  wrefresh(win_input);
  return 0;
}

int send_to_server(int sock, string ligne, int* ilpt)
/* Envoie ligne au serveur, précédé d'un code correspondant au type de
   message.
   Renvoie un code d'erreur si la commande n'existe pas ou si le fichier
   dont le transfert a été demandé n'existe pas. */
{
  string commande, arg1;
  string * sptr = malloc(sizeof(string));
  char buffer[MAX_BUFF];
  /* Les commandes commencent par / si ligne ne commence pas par / il s'agit
     d'un simple message à transmettre à tout le monde. */
  if (ligne[0] != '/')
    {
      write(sock, "MSG\nPBL\n", 8);
      write(sock, ligne, strlen(ligne));
    }
  // Sinon il s'agit d'une commande à interpréter.
  else
    {
      commande = strtok_r(ligne, " ", sptr);
      if (!strcmp(commande, "/l\n") or !strcmp(commande, "/list\n"))
	{
	  write(sock, "INFO\nLIST\n", 10);
	}
      else if (!strcmp(commande, "/h\n") or !strcmp(commande, "/help\n"))
	{
	  write(sock, "INFO\nHELP\n", 10);
	}
      else if (!strcmp(commande, "/t") or !strcmp(commande, "/tell"))
	{
	  write(sock, "MSG\nPVT\n", 8);
	  arg1 = strtok_r(NULL, " ", sptr);
	  sprintf(buffer, "%s\n", arg1);
	  write(sock, buffer, strlen(buffer));
	  write(sock, *sptr, strlen(*sptr));
	}
      else if (!strcmp(commande, "/n") or !strcmp(commande, "/name"))
	{
	  write(sock, "NAME\nCHG\n", 9);
	  write(sock, *sptr, strlen(*sptr));
	}
      else if (!strcmp(commande, "/q\n") or !strcmp(commande, "/quit\n"))
	{
	  write(sock, "INFO\nQUIT\n", 10);
	  endwin();
	  exit(0);
	}
      else if (!strcmp(commande, "/f") or !strcmp(commande, "/file"))
	{
	  int t;
	  arg1 = strtok_r(NULL, " ", sptr);
	  t = open(arg1, O_RDONLY);
	  if (t < 0)
	    {
	      return F_NEX;
	    }
	  close(t);
	  write(sock, "FT\nASK\n", 7);
	  write(sock, arg1, strlen(arg1));
	  write(sock, "\n", 1);
	  write(sock, *sptr, strlen(*sptr));
	}
      else // Commande inconnue.
	{
	  return INV_CMD;
	}
    }
  return 0;
}

void server_input(int sock, WINDOW * win_chat, string nom, string * argv)
/* Lit la première ligne d'un message envoyé par le serveur et appelle la 
   fonction de traitement correspondante. */
{
  char buffer[MAX_BUFF];
  int t;
  t = sock_read_line(buffer, MAX_BUFF, sock);
  if (t == -1)
    {
      endwin();
      puts("Vous avez été déconnecté du serveur.");
      exit(1);
    }
  if (!strcmp(buffer, "MSG\n"))
    {
      rcpt_msg(sock, win_chat);
    }
  else if (!strcmp(buffer, "NAME\n"))
    {
      rcpt_name(sock, win_chat, nom);
    }
  else if (!strcmp(buffer, "INFO\n"))
    {
      rcpt_info(sock, win_chat);
    }
  else if (!strcmp(buffer, "FT\n"))
    {
      rcpt_ft(sock, win_chat, nom, argv);
    }
  wrefresh(win_chat);
}

void rcpt_msg(int sock, WINDOW *win_chat)
/* Traite la réception d'un message et l'affiche différemment en fonction du 
   type de message. */
{
  char buffer[MAX_BUFF];
  char usrname[MAX_USERNAME];
  sock_read_line(buffer, MAX_BUFF, sock);
  if (!strcmp(buffer, "PBL\n"))
    {
      sock_read_line(usrname, MAX_USERNAME, sock);
      Last(usrname) = 0;
      sock_read_line(buffer, MAX_BUFF, sock);
      wprintw(win_chat, "[%s] : %s", usrname, buffer);
    }
  else if (!strcmp(buffer, "PVT\n"))
    {
      sock_read_line(usrname, MAX_USERNAME, sock);
      Last(usrname) = 0;
      sock_read_line(buffer, MAX_BUFF, sock);
      wprintw(win_chat, "Reçu de [%s] : %s", usrname, buffer);
    }
  else if (!strcmp(buffer, "PV_OK\n"))
    {
      sock_read_line(usrname, MAX_USERNAME, sock);
      Last(usrname) = 0;
      sock_read_line(buffer, MAX_BUFF, sock);
      wprintw(win_chat, "A [%s] : %s", usrname, buffer);
    }
  else if (!strcmp(buffer, "PV_NOK\n"))
    {
      sock_read_line(usrname, MAX_USERNAME, sock);
      Last(usrname) = 0;
      wprintw(win_chat, "Impossible d'envoyer un message à [%s] : Aucun "
	      "utilisateur de ce nom.\n", usrname);
    }
}

void rcpt_name(int sock, WINDOW *win_chat, string usrname)
/* Traite la réception du retour d'une demande de changement de nom. 
   Met à jour le nom si le nom demandé est libre, affiche un message si le 
   nom demandé est pris. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, sock);
  if (!strcmp(buffer, "OK\n"))
    {
      sock_read_line(buffer, MAX_USERNAME, sock);
      Last(buffer) = 0;
      strncpy(usrname, buffer, MAX_USERNAME);
    }
  if (!strcmp(buffer, "TAKEN\n"))
    {
      sock_read_line(buffer, MAX_USERNAME, sock);
      Last(buffer) = 0;
      wprintw(win_chat, "Le nom %s n'est pas disponible.\n", buffer);
    }
}

void rcpt_info(int sock, WINDOW *win_chat)
/* Traite la réception d'une information du serveur. Affiche cette
   information ligne par ligne. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, sock);
  while(strcmp(buffer, "\n"))
    {
      wprintw(win_chat, "%s", buffer);
      sock_read_line(buffer, MAX_BUFF, sock);
    }
}

void rcpt_ft(int sock, WINDOW *win_chat, string usrname, string * argv)
/* Traite la réception d'un message de type transfert de fichier.
   Appelle la fonction de traitement correspondante. */
{
  char buffer[MAX_BUFF];
  sock_read_line(buffer, MAX_BUFF, sock);
  if (!strcmp(buffer, "ASK\n"))
    {
      cl_process_ftask(sock, win_chat, usrname, argv);
    }
  else if (!strcmp(buffer, "ACC\n"))
    {
      send_file(sock, usrname, win_chat, argv);
    }
  else if (!strcmp(buffer, "NACC\n"))
    {
      cl_process_ftnacc(sock, win_chat);
    }
  else if (!strcmp(buffer, "SRC_END\n"))
    {
      src_end_ft(sock, win_chat);
    }
  else if (!strcmp(buffer, "DEST_END\n"))
    {
      dest_end_ft(sock, win_chat);
    }
}

void cl_process_ftask(int sock, WINDOW * win_chat, string nom, string * argv)
/* Traite la réception d'une demande de transfert de fichier.
   Demande a l'utilisateur s'il accepte le transfer, si oui procède à la 
   réception du fichier. */
{
  char fname[MAX_FNAME];
  char src_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  Last(fname) = 0;
  sock_read_line(src_usr, MAX_USERNAME, sock);
  Last(src_usr) = 0;
  wprintw(win_chat, "%s veut vous envoyer le fichier %s, acceptez vous "
	  "(o/n) ?\n", src_usr, fname);
  wrefresh(win_chat);
  int rep = getch();
  while (rep != 'o' and rep != 'n')
    {
      wprintw(win_chat, "Veuillez répondre avec o/n\n");
      wrefresh(win_chat);
      rep = getch();
    }
  if (rep == 'o')
    {
      receive_ft(fname, src_usr, nom, argv);
    }
  else
    {
      write(sock, "FT\nNACC\n", 8);
      write(sock, fname, strlen(fname));
      write(sock, "\n", 1);
      write(sock, src_usr, strlen(src_usr));
      write(sock, "\n", 1);
      write(sock, nom, strlen(nom));
      write(sock, "\n", 1);
    }
}

void receive_ft(string fname, string src_usr, string nom, string * argv)
/* Procède à la réception d'un fichier. 
   La réception se fait dans un processus enfant afin que l'utilisateur 
   puisse continuer à recevoir et envoyer des messages pendant le transfert.
*/
{
  int t = fork();
  if (t == 0)
    {
      int ffd = create_dest_file(fname);
      int fsock = dial(argv[1], argv[2]);
      write(fsock, "DATA\n", 5);
      write(fsock, nom, strlen(nom));
      write(fsock, "\n", 1);
      write(fsock, "FT\nACC\n", 7);
      write(fsock, fname, strlen(fname));
      write(fsock, "\n", 1);
      write(fsock, src_usr, strlen(src_usr));
      write(fsock, "\n", 1);
      write(fsock, nom, strlen(nom));
      write(fsock, "\n", 1);
      char ftbuff[MAX_BUFF];

      t = read(fsock, ftbuff, MAX_BUFF);
      while (t == MAX_BUFF)
	{
	  write(ffd, ftbuff, MAX_BUFF);
	  t = read(fsock, ftbuff, MAX_BUFF);
	}
      if (t > 0) // Ecrire les derniers octets lus.
	{
	  write(ffd, ftbuff, t);
	}
      close(ffd);
      write(fsock, "FT\nEND\n", 7);
      write(fsock, fname, strlen(fname));
      write(fsock, "\n", 1);
      write(fsock, src_usr, strlen(src_usr));
      write(fsock, "\n", 1);
      write(fsock, nom, strlen(nom));
      write(fsock, "\n", 1);
      exit(0);
    }
}

int create_dest_file(string fname)
/* Crée et ouvre un fichier dont le nom est fname, si un fichier nommé fname
   existe déjà, crée un fichier du même nom avec un suffixe numérique entre
   parenthèses. Ce suffixe est incrémenté jusqu'à trouver un nom valide. */
{
  int ffd = open(fname, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU);
  char new_fname[MAX_FNAME * 2];
  int n = 1;
  while (ffd < 0)
    {
      char nb[14]; /* De quoi faire entrer les deux parenthèses, le point
		      et un int (max 10 chiffres).*/
      idx idx_lastdot = 0;
      for (idx i = 0; fname[i]; i++)
	{
	  if (fname[i] == '.')
	    {
	      idx_lastdot = i;
	    }
	}
      if (idx_lastdot > 0)
	{
	  sprintf(nb, "(%i).", n++);
	  fname[idx_lastdot] = 0;
	  strncpy(new_fname, fname, MAX_FNAME);
	  strncat(new_fname, nb, 14);
	  strncat(new_fname, fname + idx_lastdot + 1, MAX_FNAME);
	  fname[idx_lastdot] = '.';
	}
      else
	{
	  sprintf(nb, "(%i)", n++);
	  strncpy(new_fname, fname, MAX_FNAME);
	  strncat(new_fname, nb, 13);
	}
      ffd = open(new_fname, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU);
    }
  return ffd;
}

void send_file(int sock, string nom, WINDOW *win_chat, string *argv)
/* Procède à l'envoie d'un fichier. 
   L'envoie se fait dans un processus enfant afin que l'utilisateur puisse
   continuer à envoyer et recevoir des messsages pendant le transfert. */
{
  char fname[MAX_FNAME];
  char dest_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  Last(fname) = 0;
  sock_read_line(dest_usr, MAX_USERNAME, sock);
  Last(dest_usr) = 0;
  wprintw(win_chat, "Transfert du fichier %s à %s en cours\n", fname,
	  dest_usr);
  int t = fork();
  if (t == 0)
    {
      int fsock = dial(argv[1], argv[2]);
      write(fsock, "DATA\n", 5);
      write(fsock, nom, strlen(nom));
      write(fsock, "\n", 1);
      write(fsock, "FT\nSTART\n", 9);
      write(fsock, fname, strlen(fname));
      write(fsock, "\n", 1);
      write(fsock, dest_usr, strlen(dest_usr));
      write(fsock, "\n", 1);
      int ffd = open(fname, O_RDONLY);
      char ftbuff[MAX_BUFF];
      t = read(ffd, ftbuff, MAX_BUFF);
      while (t == MAX_BUFF)
	{
	  write(fsock, ftbuff, MAX_BUFF);
	  t = read(ffd, ftbuff, MAX_BUFF);
	} 
      if (t > 0)
	{
	  write(fsock, ftbuff, t);
	}
      close(fsock);
      close(ffd);
      exit(0);
    }
}

void cl_process_ftnacc(int sock, WINDOW *win_chat)
/* Traite la réception d'un refus de transfert de fichier. 
   Prévient l'utilisateur par un message. */
{
  char fname[MAX_FNAME];
  char dest_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  Last(fname) = 0;
  sock_read_line(dest_usr, MAX_USERNAME, sock);
  Last(dest_usr) = 0;
  wprintw(win_chat, "%s a refusé votre transfert de fichier (%s).\n",
	  dest_usr, fname);
}

void src_end_ft(int sock, WINDOW *win_chat)
/* Traite la réception d'un message signifiant qu'un envoi de fichier est
   terminé. 
   Prévient l'utilisateur avec un message. */
{
  char fname[MAX_FNAME];
  char dest_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  Last(fname) = 0;
  sock_read_line(dest_usr, MAX_USERNAME,sock);
  Last(dest_usr) = 0;
  wprintw(win_chat, "Transfert du fichier %s à %s terminé.\n", fname,
	  dest_usr);
  wait(NULL);
}

void dest_end_ft(int sock, WINDOW *win_chat)
/* Traite la réception d'un message signifiant que la réception d'un fichier
   est terminée.
   Prévient l'utilisateur avec un message. */
{
  char fname[MAX_FNAME];
  char src_usr[MAX_USERNAME];
  sock_read_line(fname, MAX_FNAME, sock);
  Last(fname) = 0;
  sock_read_line(src_usr, MAX_USERNAME,sock);
  Last(src_usr) = 0;
  wprintw(win_chat, "Transfert du fichier %s, envoyé par %s terminé.\n",
	  fname, src_usr);
  wait(NULL);
}
