/**
 * \file server.c
 * \brief server soutenant le jeu Sherlock Holmes. On exécute le server une fois et 4 clients afin de commencer un jeu. Ce jeu nous permet d'aborder les notions fondamentales de la programmation réseau.
 * \author Arthur & Younes
 * \version 1.1
 * \date 13 mai 2019
 *
 * Ce serveur propose un autre protocole que HTTP, pour
 * comprendre comment ce dernier fonctionne.
 */

// Ajouter: Sauvegarde des données rentrées
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


/**
 * \struct _client
 * \brief Objet client se connectant au serveur
 *
 *_client permet de stocker l'adresse iP, le port et le nom du client se connectant au serveur
 */

struct _client
{
  char ipAddress[40];
  int port;
  char name[40];
} tcpClients[4]; /*!< On a un tableau de 4 clients avec leur adrese IP, numéro de port ainsi que leur nom */
int nbClients; /*!< Le nombre de Clients, comme son nom l'indique */
int fsmServer; /*!< Tant que tout les clients ne sont pas connectés fsmServer = 0 */
int idDemande; /*!< Il s'agit de l'id du client qui réalise sa demande */
int guiltSel; /*!< Permet de stocker le coupable qu'accuse le client */
int joueurSel; /*!< Permet de stocker le joueur dont le client souhaite connaitre le nombre d'objet */
int objetSel; /*!< Permet de stocker le objet qui interèsse le joueur effectuant une demande */
int deck[13]={0,1,2,3,4,5,6,7,8,9,10,11,12};  /*!< La variable deck correspond à l'ensemble des indices utilisés afin de parcourir les cartes initialisés dans la variable nom_cartes */
int tableCartes[4][8]; /*!< Permet de faire correspondre le deck aux noms */
int banned[4] = {0,0,0,0};
char *nomcartes[]=
{"Sebastian Moran", "irene Adler", "inspector Lestrade",
"inspector Gregson", "inspector Baynes", "inspector Bradstreet",
"inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
"Mrs. Hudson", "Mary Morstan", "James Moriarty"}; /*!< Liste des cartes avec leur nom, on associe à chaque entier de deck, sa carte correspondate dans cette liste */

int joueurCourant; /*!< Indice du joueur ayant la main */
char chatserver[256]; /*!< Texte contenant le chat */
char tab_texte[8][256] = {"","","","","","","",""}; /*!< Tableau contenant les messages */
int i = 0;
/**
 * \fn void   error (const char *msg)
 * \brief Fonction de gestion d'erreur
 * \param const char *msg est le message à renvoyer
 * \return Cette fonction ne retourne rien.
 */

/**
 * \fn void   melangerDeck ()
 * \brief Fonction de mélange du deck de cartes afin de les distribuer
 * \warning à ne pas confondre deck et nom_cartes, comme on ne peut pas mélanger une liste de chaines de caractères, on mélange leurs indices!
 * \return Cette fonction ne retourne rien, elle exerce sa modification directement sur la variable globale.
 */

/**
 * \fn void   createTable ()
 * \brief Fonction de création de la table de carte qui est une variable globale.
 * \return Cette fonction ne retourne rien.
 */

/**
 * \fn void   printDeck ()
 * \brief Fonction d'affichage du deck'
 * \return Cette fonction ne retourne rien.
 */

/**
 * \fn void   printClients ()
 * \brief affiche le numéro, adresse IP et numéro de port de chaque client
 * \return Cette fonction ne retourne rien.
 */

/**
 * \fn int   findClientByName (char* name)
 * \brief Fonction d'affichage du deck'
 * \param char* name est le nom du client qu'on souhaite chercher
 * \warning Faire attention à bien écrire le nom (ne pas se tromper sur les majuscules etc.)
 * \returns Retourne le id du client, renvoie -1 sinon.
 */

/**
 * \fn void   sendMessageToClient(char *clientip,int clientport,char *mess)
 * \brief Fonction d'affichage du deck'
 * \warning En cas d'erreur (faute sur le nom par exemple), le message ne sera pas envoyé
 * \param char* name est le nom du client qu'on souhaite chercher
 * \return Cette fonction ne retourne rien, elle envoie le message directement au client.
 */

/**
 * \fn void   broadcastMessage (char* mess)
 * \brief Permet d'envoyer un message à l'ensemble des clients connectés sur le server
 * \param char* mess est le message à envoyer
 * \return Cette fonction ne retourne rien.
 */

/**
 * \fn int   main(int argc, char *argv[])
 * \brief Fonction principale
 *
 */

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

void melangerDeck()
{
  int i;
  int index1,index2,tmp;

  for (i=0;i<1000;i++)
  {
    index1=rand()%13;
    index2=rand()%13;

    tmp=deck[index1];
    deck[index1]=deck[index2];
    deck[index2]=tmp;
  }
}

void createTable()
{
  // Le joueur 0 possede les cartes d'indice 0,1,2
  // Le joueur 1 possede les cartes d'indice 3,4,5
  // Le joueur 2 possede les cartes d'indice 6,7,8
  // Le joueur 3 possede les cartes d'indice 9,10,11
  // Le coupable est la carte d'indice 12
  int i,j,c;

  for (i=0;i<4;i++)
  for (j=0;j<8;j++)
  tableCartes[i][j]=0;

  for (i=0;i<4;i++)
  {
    for (j=0;j<3;j++)
    {
      c=deck[i*3+j];
      switch (c)
      {
        case 0: // Sebastian Moran
        tableCartes[i][7]++;
        tableCartes[i][2]++;
        break;
        case 1: // Irene Adler
        tableCartes[i][7]++;
        tableCartes[i][1]++;
        tableCartes[i][5]++;
        break;
        case 2: // Inspector Lestrade
        tableCartes[i][3]++;
        tableCartes[i][6]++;
        tableCartes[i][4]++;
        break;
        case 3: // Inspector Gregson
        tableCartes[i][3]++;
        tableCartes[i][2]++;
        tableCartes[i][4]++;
        break;
        case 4: // Inspector Baynes
        tableCartes[i][3]++;
        tableCartes[i][1]++;
        break;
        case 5: // Inspector Bradstreet
        tableCartes[i][3]++;
        tableCartes[i][2]++;
        break;
        case 6: // Inspector Hopkins
        tableCartes[i][3]++;
        tableCartes[i][0]++;
        tableCartes[i][6]++;
        break;
        case 7: // Sherlock Holmes
        tableCartes[i][0]++;
        tableCartes[i][1]++;
        tableCartes[i][2]++;
        break;
        case 8: // John Watson
        tableCartes[i][0]++;
        tableCartes[i][6]++;
        tableCartes[i][2]++;
        break;
        case 9: // Mycroft Holmes
        tableCartes[i][0]++;
        tableCartes[i][1]++;
        tableCartes[i][4]++;
        break;
        case 10: // Mrs. Hudson
        tableCartes[i][0]++;
        tableCartes[i][5]++;
        break;
        case 11: // Mary Morstan
        tableCartes[i][4]++;
        tableCartes[i][5]++;
        break;
        case 12: // James Moriarty
        tableCartes[i][7]++;
        tableCartes[i][1]++;
        break;
      }
    }
  }
}

void printDeck()
{
  int i,j;

  for (i=0;i<13;i++)
  printf("%d %s\n",deck[i],nomcartes[deck[i]]);

  for (i=0;i<4;i++)
  {
    for (j=0;j<8;j++)
    printf("%2.2d ",tableCartes[i][j]);
    puts("");
  }
}

void printClients()
{
  int i;

  for (i=0;i<nbClients;i++)
  printf("%d: %s %5.5d %s\n",i,tcpClients[i].ipAddress,
  tcpClients[i].port,
  tcpClients[i].name);
}

int findClientByName(char *name)
{
  int i;

  for (i=0;i<nbClients;i++)
  if (strcmp(tcpClients[i].name,name)==0)
  return i;
  return -1;
}

void sendMessageToClient(char *clientip,int clientport,char *mess)
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[10000];
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server = gethostbyname(clientip);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
  (char *)&serv_addr.sin_addr.s_addr,
  server->h_length);
  serv_addr.sin_port = htons(clientport);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
  {
    printf("ERROR connecting\n");
    exit(1);
  }

  sprintf(buffer,"%s\n",mess);
  n = write(sockfd,buffer,strlen(buffer));
  close(sockfd);
}

void broadcastMessage(char *mess){
  int i;
  for (i=0;i<nbClients;i++)
  sendMessageToClient(tcpClients[i].ipAddress,
    tcpClients[i].port,
    mess);
}

void initialiser(struct _client Clients[4]){
	for (int i = 0; i < 4; ++i)
	{
		strcpy(tcpClients[i].ipAddress, "\0");
        strcpy(tcpClients[i].name, "-");
		Clients[i].port = 0;
	}
}

int main(int argc, char *argv[]){
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    int i=0;
    //int idDemande,guiltSel,joueurSel,objetSel;
    char com;
    char clientIpAddress[256], clientName[256];
    int clientPort;
    int id;
    char reply[256];


    if (argc < 2) {
      fprintf(stderr,"ERROR, no port provided\n");
      exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
    sizeof(serv_addr)) < 0)
    error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    printDeck();
    melangerDeck();
    createTable();
    printDeck();
    joueurCourant=0;

    for (i=0;i<4;i++){
      strcpy(tcpClients[i].ipAddress,"localhost");
      tcpClients[i].port=-1;
      strcpy(tcpClients[i].name,"-");
    }
    int nombre_messages = 0;
    while (1)
    {
      newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
      if (newsockfd < 0)
      error("ERROR on accept");
      bzero(buffer,256);
      n = read(newsockfd,buffer,255);
      if (n < 0)
      error("ERROR reading from socket");
      printf("Received packet from %s:%d\nData: [%s]\n\n",
      inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);
      if (fsmServer==0)
      {
      	struct _client temp[4];
        switch (buffer[0])
        {
            case 'Q':
                sscanf(buffer,"Q %d",&idDemande);
                nbClients = nbClients-1;
                //tcpClients[idDemande] = NULL;
                if (nbClients == 0)
                {
                  strcpy(tcpClients[0].ipAddress, "\0");
                  strcpy(tcpClients[0].name, "-");



              break;
                }else{
                  int z = 0;
                  for (int i = 0; i < nbClients+1; ++i)
                  {
                    if (i != idDemande)
                    {
                      //printf("AJJJ\n");
                      strcpy(temp[z].ipAddress, tcpClients[i].ipAddress);
                  	  strcpy(temp[z].name, tcpClients[i].name);
                  	  temp[z].port = tcpClients[i].port;
                      z++;
                    }
                    //printf("%s\n", temp[z-1].name);
                  }
                  initialiser(tcpClients);
                  printClients();
                  for (int i = 0; i < z; ++i)
                  {
                  	//printf("AHAHAHAH\n");
                  	printf("%s\n", temp[i].ipAddress);
                  	printf("%s\n", temp[i].name);
                  	strcpy(tcpClients[i].ipAddress, temp[i].ipAddress);
                  	strcpy(tcpClients[i].name, temp[i].name);
                  	tcpClients[i].port = temp[i].port;
                  }
                  //printf("%s\n", temp[z-1].name);
                  //initialiser(temp);
                  printClients();
            	  sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
            	  printf("reply : %s\n", reply);
            	  broadcastMessage(reply);
            	  ///printf("%s\n", );
                }
                break;
            // On recrée la liste en enlevant le joueur.
            // On remet l'odre de la liste
            case 'Z':
              //printf("COCOU\n");
              sscanf(buffer,"Z %d %[^\n]s", &idDemande, chatserver);
              printf("chatserver1 : %s\n", chatserver);
              sprintf(tab_texte[nombre_messages], "%s : %s", tcpClients[idDemande].name, chatserver);
              printf("tabtexte de i : %s\n", tab_texte[nombre_messages]);
              nombre_messages++;
              if (nombre_messages > 7) {
                nombre_messages = 0;
              }
              char tempreply[1000000] = {"Z"};
              printf("temprepmly = %s\n", tempreply);
              for (int j = 0; j < 8; j++) {
                sprintf(reply, " %s\n", tab_texte[j]);
                strcat(tempreply,reply);
              }
              strcat(tempreply,"Z");
              broadcastMessage(tempreply);
              break;
            case 'C':
                sscanf(buffer,"%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
                printf("COM=%c ipAddress=%s port=%d name=%s\n",com, clientIpAddress, clientPort, clientName);

                // Afficher message "il reste tant de clients avant que la partie commence"
                sprintf(tab_texte[nombre_messages], "!Server!:|%s| joined the game",clientName);
                nombre_messages++;
                if (nombre_messages > 7) {
                  nombre_messages = 0;
                }
                sprintf(tab_texte[nombre_messages], "!Server!:|%d| missing players before the beggining",3-nbClients);
                nombre_messages++;
                if (nombre_messages > 7) {
                  nombre_messages = 0;
                }
                char reply2[100000];
                char tempreply4[100000] = {"Z"};
                printf("temprepmly = %s\n", tempreply4);
                for (int j = 0; j < 8; j++) {
                  sprintf(reply2, " %s\n", tab_texte[j]);
                  strcat(tempreply4,reply2);
                }
                strcat(tempreply4,"Z");

                printf("broadcastMessage pas bien\n");
                broadcastMessage(tempreply4);

                printf("Erreur pas ou je syncronisé\n");
                // fsmServer==0 alors j'attends les connexions de tous les joueurs
                strcpy(tcpClients[nbClients].ipAddress,clientIpAddress);
                tcpClients[nbClients].port=clientPort;
                strcpy(tcpClients[nbClients].name,clientName);
                nbClients++;

                printClients();

                // rechercher l'id du joueur qui vient de se connecter

                id=findClientByName(clientName);
                printf("id=%d\n",id);

                // lui envoyer un message personnel pour lui communiquer son id

                sprintf(reply,"I %d",id);
                sendMessageToClient(tcpClients[id].ipAddress,tcpClients[id].port,reply);

                  // Envoyer un message broadcast pour communiquer a tout le monde la liste des joueurs actuellement
                  // connectes

                sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
                broadcastMessage(reply);

                  // Si le nombre de joueurs atteint 4, alors on peut lancer le jeu
                if (nbClients==4){
                    // On envoie ses cartes au joueur 0, ainsi que la ligne qui lui correspond dans tableCartes
                    int k = 0;
                    sprintf(tab_texte[nombre_messages], "!! Serveur !! : Le salon est complet la partie peut commencer!");
                    nombre_messages++;
                    if (nombre_messages > 7) {
                      nombre_messages = 0;
                    }
                    char reply1[100000];
                    char tempreply3[100000] = {"Z"};
                    printf("temprepmly = %s\n", tempreply3);
                    for (int j = 0; j < 8; j++) {
                      sprintf(reply1, " %s\n", tab_texte[j]);
                      strcat(tempreply3,reply1);
                    }
                    strcat(tempreply3,"Z");
                    broadcastMessage(tempreply3);
                    // Dire que la partie va commencer


                    for ( i = 0; i < 4; i++) {
                      sprintf(reply,"D %d %d %d",deck[i+k],deck[i+k+1],deck[i+k+2]);
                      k+=2;
                      sendMessageToClient(tcpClients[i].ipAddress,
                        tcpClients[i].port,
                        reply);
                        sprintf(reply,"V %d %d %d %d %d %d %d %d ", tableCartes[i][0] ,tableCartes[i][1],tableCartes[i][2],tableCartes[i][3],tableCartes[i][4],tableCartes[i][5],tableCartes[i][6],tableCartes[i][7]);
                        sendMessageToClient(tcpClients[i].ipAddress,
                          tcpClients[i].port,
                          reply);
                        }
                        sprintf(reply,"M %d", joueurCourant);
                        broadcastMessage(reply);
                        fsmServer=1;
                      }
                      break;
                    }

                }
      else if (fsmServer==1){
      	 		struct _client temp[4];
                switch (buffer[0])
                {
                  case 'Z':
                  sscanf(buffer,"Z %d %[^\n]s", &idDemande, chatserver);
                  printf("chatserver1 : %s\n", chatserver);
                  sprintf(tab_texte[nombre_messages], "%s : %s", tcpClients[idDemande].name, chatserver);
                  printf("tabtexte de i : %s\n", tab_texte[nombre_messages]);
                  nombre_messages++;
                  if (nombre_messages > 7) {
                    nombre_messages = 0;
                  }
                  char tempreply[1000] = {"Z"};
                  printf("temprepmly = %s\n", tempreply);
                  for (int j = 0; j < 8; j++) {
                    sprintf(reply, " %s\n", tab_texte[j]);
                    strcat(tempreply,reply);
                  }
                  strcat(tempreply,"Z");
                  broadcastMessage(tempreply);
                  break;
                  case 'Q':
                    sscanf(buffer,"Q %d",&idDemande);
                    nbClients = nbClients-1;
                    //tcpClients[idDemande] = NULL;
                    {
                      //initialiser(temp);
                      int z = 0;
                      for (int i = 0; i < nbClients+1; ++i)
                      {
                        if (i != idDemande)
                        {
                          //printf("AJJJ\n");
                          strcpy(temp[z].ipAddress, tcpClients[i].ipAddress);
                      	  strcpy(temp[z].name, tcpClients[i].name);
                      	  temp[z].port = tcpClients[i].port;
                          z++;
                        }
                        //printf("%s\n", temp[z-1].name);
                      }
                      printClients();
                      initialiser(tcpClients);
                      printClients();
                      for (int i = 0; i < z; ++i)
                      {
                      	//printf("AHAHAHAH\n");
                      	printf("%s\n", temp[i].ipAddress);
                      	printf("%s\n", temp[i].name);
                      	strcpy(tcpClients[i].ipAddress, temp[i].ipAddress);
                      	strcpy(tcpClients[i].name, temp[i].name);
                      	tcpClients[i].port = temp[i].port;
                      }
                      //printf("%s\n", temp[z-1].name);
                      //initialiser(temp);
                      printf("Apres le quit\n" );
                      printClients();
                  	  sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
                  	  printf("reply : %s\n", reply);
                  	  broadcastMessage(reply);
                      sprintf(reply,"Q %d",z);
                  	  printf("reply : %s\n", reply);
                  	  broadcastMessage(reply);
                  	  ///printf("%s\n", );
                        }
                      fsmServer=0;
                    break;
                  case 'G':
                  sscanf(buffer,"G %d %d",&idDemande,&guiltSel);
                  if (guiltSel == deck[12] ) {
                    sprintf(reply,"Winner : %d", idDemande);
                    broadcastMessage(reply);
                    exit(1); // Reset le serveur

                  }
                  else{
                    sprintf(reply,"Fail! He is not guilty %d you can not play anymore", idDemande);
                    broadcastMessage(reply);
                    banned[idDemande] = 1;
                  }
                  break;
                  char c1[5];
                  case 'O':
                    sscanf(buffer,"O %d %d",&idDemande,&objetSel);
                    for (int i = 0; i < 4; i++) {
                      if (tableCartes[i][objetSel] != 0) {
                        c1[i] = '*';
                      }
                      else{c1[i] = '0';}
                    }
                    sprintf(reply,"O %d %c %c %c %c", objetSel,c1[0],c1[1],c1[2],c1[3]);
                    broadcastMessage(reply);
                    break;
                  case 'S':
                    sscanf(buffer,"S %d %d %d",&idDemande,&joueurSel,&objetSel);
                    printf("%d %d %d",idDemande,joueurSel,objetSel );
                    sprintf(reply,"S %d %d %d",joueurSel, objetSel,tableCartes[joueurSel][objetSel]);
                    broadcastMessage(reply);
                    break;
                  default:
                    break;
                  }
                  joueurCourant = (++joueurCourant)%4;
                  int z = 0;
                  while(banned[joueurCourant] == 1 ) {
                    z++;
                    joueurCourant = (++joueurCourant)%4;
                    if (z==3) {
                      sprintf(reply,"You all lost!");
                      broadcastMessage(reply);
                      exit(1); // Reset le serveur
                    }
                  }
                  sprintf(reply,"M %d", joueurCourant);
                  broadcastMessage(reply);
                }
                close(newsockfd);
              }
              close(sockfd);
              return 0;
            }
