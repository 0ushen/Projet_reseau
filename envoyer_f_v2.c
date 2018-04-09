#include <stdio.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#define _WIN32_WINNT 0x501
#include <ws2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

#elif defined (linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

// Fonction qui intialise la DLL permettant de manipuler les sockets sous windows

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

// Fonction qui lib�re la DLL

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

int main(int argc, char *argv[])
{
    // V�rification de la syntaxe de la commande entr�e par l'utilisateur.

    if(argc < 4)
    {
		printf("Syntaxe : %s <fichier> <ip/hostname> <port>\n", argv[0]);
		return -1;
	}

	init();

    /* Je change les variables de ma structure hints , celle ci permettra
       d'aiguiller la fonction getaddrinfo sur le type de r�sultats � afficher.
       Ici je configure hints pour rechercher une structure de type IPv4/IPv6
       avec le protocole TCP. */

    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    /* La fonction getaddrinfo va rechercher des r�sultats en fonction de la
       structure hints et les stocker dans la liste chain�e result. La fonction
       renvoie 0 si il n'y a pas eu d'erreur. */

    struct addrinfo *result;
    int rc = getaddrinfo(argv[2], argv[3], &hints, &result);
    if (rc != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        exit(EXIT_FAILURE);
    }

     /* Cette boucle parcourt la liste chain�e result � la recherche d'un r�sultat
        sur lequel une connexion peut �tre initialis�e. */

    SOCKET sock;
    struct addrinfo *rp;
    for(rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sock == INVALID_SOCKET)
            continue;

        if(connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
            break;

        closesocket(sock);
    }
    if(rp == NULL)
    {
        fprintf(stderr, "Could not connect\n");
        end();
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);

    // Ouverture en mode binaire et lecture seule du fichier que l'on va envoy�.

    FILE* file = NULL;
    file = fopen(argv[1], "rb");
    if(file == NULL)
	{
		perror("Error fopen()");
		end();
		return -1 ;
	}

    /* Tant que fread renvoie une valeur diff�rente de 0 (donc tant qu'il
       reste des donn�es � envoyer) le block de donn�es lu par fread()
       (max 4000 octet) et stock� dans le buffer est envoy� via le socket
       cr�e pr�cedemment. */

    size_t buffer[4000];
    unsigned int size;
	int i = 1;
	while((size = fread(buffer, 1, 4000, file)))
	{
		if((send(sock, buffer, size, 0) < 0))
        {
            perror("send()");
            exit(errno);
        }
        printf("Block %d was sent || Size : %u\n", i, size);

        i++;
	}

    // Je lib�re le fichier , le socket et la DLL windows.

	fclose(file);
    closesocket(sock);
    end();

	return 0;

}
