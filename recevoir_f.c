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
typedef struct sockaddr_in6 SOCKADDR_IN6;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

// Fonction qui intialise la DLL permettant de manipuler les sockets sous windows.

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

// Fonction qui libère la DLL.

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

int main(int argc, char *argv[])
{
	// Vérification de la syntaxe de la commande entrée par l'utilisateur.

    if(argc < 3)
    {
		printf("Syntax : %s <file> <port>\n", argv[0]);
		return -1;
	}

	init();

    // Création du socket sur lequel le serveur écoutera.

	SOCKET sock = socket(AF_INET6, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET)
	{
        perror("socket()");
        end();
        exit(errno);
    }

    /* Le support dual stack est possible sur windows à partir de vista.
       Pour cela il faut mettre à 0 la valeur d'IPV6_V6ONLY via la fonction
       setsockopt(). Ceci n'est pas nécéssaire sur linux. */

    #ifdef WIN32
    int optValue = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optValue, sizeof(optValue)) < 0)
    {
        WSAGetLastError();
    }
    #endif

    /* J'utilise une structure de type sockaddr_in6 avec les membres intialisés
       à des valeurs permettant de recevoir n'importe quel adresse ipv6, ainsi
       que le port entré en argument qui sera redéfini selon le network byte order.
       Cette façon de faire permet d'accepter les connexions ipv6 mais aussi ipv4. */

    SOCKADDR_IN6 sin = {0};
    sin.sin6_addr = in6addr_any;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(atoi(argv[2]));
    if(bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        end();
        exit(errno);
    }

    /* Le serveur écoute et attends une connexion ipv4 ou ipv6 sur le port spécifié.
       La longueur maxime de la file des connexions est fixée à 5. */

    if(listen(sock, 5) == SOCKET_ERROR)
    {
        perror("listen()");
        end();
        exit(errno);
    }
    printf("Ready for client connect().\n");

    /* Un autre socket est crée afin de pouvoir gérer la connexion entrante acceptée.
       Le type de protocole accepté par le socket n'est pas précisé car il doit pouvoir géré
       l'ipv4 et l'ipv6. */

    SOCKET csock;
    csock = accept(sock, NULL, NULL);
    if(csock == INVALID_SOCKET)
    {
        perror("accept()");
        end();
        exit(errno);
    }

    // Ouverture du fichier.

    FILE* file = NULL;
    file = fopen(argv[1], "wb");
    if(file == NULL)
	{
		perror("Error fopen()");
		end();
		exit(errno);
	}

    /* Cette boucle permet la réception du fichier envoyé par tranche de 4000 octet.
       Je sors de la boucle quand la fonction recv() renvoie un code d'erreur (-1) ou
       quand la taille des données lues est égale à 0. */

	size_t buffer[4000];
	unsigned int size = 4000;
	int i = 0;
	while(size != 0)
	{
        if((size = recv(csock, buffer, sizeof buffer, 0)) >= 0)
        {
            printf("Block %d received | Size : %u\n", i, size);
            fwrite(buffer, 1, size, file);
            i++;
        }
        else
        {
            perror("Error recv()");
            break;
        }
	}

    // Je ferme le fichier , les 2 sockets et la dll windows.

	fclose(file);
    closesocket(sock);
    closesocket(csock);
    end();

    return 0;
}
