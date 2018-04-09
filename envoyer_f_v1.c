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

    // Cr�ation du socket.

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    /* Cr�ation de la structure sin de type sockaddr_in contenant les
       informations de connexions (adresse ip, port, protocole ipv4).
       inet_pton() permet de transformer l'adresse ip pass�e sous forme de
       chaine de caract�res en argument en une forme binaire.
       htons() s'assure que le port pass� en argument soir conforme au network
       byte order. */

    SOCKADDR_IN sin = { 0 };
    if(inet_pton(AF_INET, argv[2], &sin.sin_addr.s_addr) == 0)
    {
        perror("Error inet_pton()");
        end();
        return -1;
    }
    sin.sin_port = htons(atoi(argv[3]));
    sin.sin_family = AF_INET;

    // Connexion � l'h�te.

    if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        perror("connect()");
        exit(errno);
    }

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
