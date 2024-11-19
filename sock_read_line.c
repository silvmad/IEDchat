#include "commod.h"
#include <unistd.h>

int sock_read_line(string ligne, int max_ligne, int fd)
/* Lit dans le socket représenté par fd octet par octet et écrit ces octets
   dans ligne. La lecture s'arrête après un retour à la ligne. 
   Renvoie 1 s'il n'y avait rien à lire (EOF) et 0 sinon. */
{
  idx i = 0;
  char ch;
  int t;
  t = read(fd, &ch, 1);
  if (t == 0)
    {
      return -1;
    }
  while (ch != '\n')
    {
      ligne[i++] = ch;
      if (i == max_ligne - 1)
	{
	  ligne[i] = 0;
	  return 0;
	}
      read(fd, &ch, 1);
    }
  ligne[i] = ch;
  ligne[i + 1] = 0;
  return 0;
}
