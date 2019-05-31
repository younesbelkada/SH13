/**
 * \file sh13.c
 * \brief client permettant de se connecter au serveur
 * \author Arthur & Younes
 * \version 1.1
 * \date 10 mai 2019
 *
 * Le client sh13 permet de se connecter au serveur proposant un autre protocol que HTTP, pour
 * comprendre comment ce dernier fonctionne.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
pthread_t thread_serveur_tcp_id; /*!< Stock l'id du thread  correspondant au thread du serveur*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /*!< Le mutex permet de proteger l'accès au serveur */
char gbuffer[256]; /*!< Le buffer permet de stocker le message à transmettre au serveur */
char gServerIpAddress[256];/*!< L'adresse IP du srveur entrée lorsque l'on lance sh13 est stockée sous la forme de charctère afin de faciliter le transfert */
int gServerPort;/*!< Le port à utiliser pour le serveur est donné en entrée */
char gClientIpAddress[256]; /*!< L'IP du client envoyant un message doit être transmise au serveur afin de pouvoir ensuite recevoir des messages' */
int gClientPort;/*!< De même le serveur utilise le port du client pour lui transmettre le message à travers le protocole TCP */
char gName[256];/*!< Username ou  surnom entrée lors de l'inscription sur le serveur */
char gNames[4][256]; /*!<Tableaux stockant les nomx des autres joueurs ayant aussi rejoint la partie */
int gId;/*!< ID du client donnée par le serveur selon l'ordre d'arrivée*/
int joueurSel;/*!< Joueur séléctionné pour une demande */
int objetSel;/*!< Onjet séléctionné pour une demande */
int guiltSel;/*!< Coupable séléctionné pour une demande */
int guiltGuess[13];/*!< Stock le nom des coupable pour permettre un affichage où l'on peut completer les donnée en éléminant ceux qu'on pense innocents */
int tableCartes[4][8];/*!< Stock les données obtenue par les demande ainsi que les objets présents sur les cartes distribuées par le serveur */
int b[3];/*!< Stock les 3 cartes ou nom des cartes distribuées par le mélange du serveur */
int goEnabled; /*!< Un booléen indiquant si le client possède la main, si c'est à lui de jouer le bouton GO s'affichera et il pourra effectuer une demande */
int connectEnabled;/*!< Si le joueur est connecté, il n'a plus besoin du bouton connect qui doit donc disparaitre */
int chatEnable = -1;/*!< Prototype permettant de gérer le chat */
SDL_Color couleurNoire = {0, 0, 0};


char *nbobjets[]={"5","5","5","5","4","3","3","3"};/*!< Liste le nombre total de chaque objet par indices par exemple objet[0] = 5 */
char *nbnoms[]={"Sebastian Moran", "irene Adler", "inspector Lestrade",
"inspector Gregson", "inspector Baynes", "inspector Bradstreet",
"inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
"Mrs. Hudson", "Mary Morstan", "James Moriarty"};
char texte_courant[1000];/*<!Stock les 8 derniers messages envoyés dans le chat */
int i = 0;

volatile int synchro; /*!< La syncro permet d'avoir des thread syncronisé, cependant il faut passer au dela du cache */
// Passage par dessus le cache, direction la memoire pour les problemes de plusieurs cache chacun une copie de syncro
/**
 * \fn void   *fn_serveur_tcp (void *arg)
 * \brief Fonction passée en argument aux thread qui l'executera
 * \param void *arg sont les arguments de la fonction
 */
void *fn_serveur_tcp(void *arg)
{
  int sockfd, newsockfd, portno; //Déclaration des socket et ports utilisés plus tard*/
  socklen_t clilen; /*Nombre maximum de clients acceptés sur le serveur */
  struct sockaddr_in serv_addr, cli_addr; /* Adresse IP pour la copie des sockets */
  int n;

  sockfd = socket(AF_INET, SOCK_STREAM, 0); /*On récupère le socket qui est une copie, une traduction de la connexion sous la forme de fichier  */
  if (sockfd<0) /* permet de gérer les erreurs rare de récupération de socket */
  {
    printf("sockfd error\n");
    exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr)); /* Efface le contenu présent à l'adresse de serv_adresse */
  portno = gClientPort;
  serv_addr.sin_family = AF_INET;         /* On initialise le type de protocole utilisé par notre socket, ici les adresse IPV4 */
  serv_addr.sin_addr.s_addr = INADDR_ANY; /* Permet d'autoriser nimporte quelle adresse à être lié au socket lors du BIND (more details to come)  */
  serv_addr.sin_port = htons(portno);     /*Le port utilisé par le socket pour communiquer */
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) /*!< \if permettant de gérer les erreurs de bind, qui permet d'allouer le port à la socket */
  {
    printf("bind error\n");
    exit(1);
  }
  listen(sockfd,5); /* la socket est prête à recevoir des connection, et 5 indique le nombre maximum de personnes autorisé en attente pour se connecter  */
  clilen = sizeof(cli_addr); /* Taille de l'adresse du client*/
  while (1) /* LA boucle permettant au serveur de tourner sans jamais s'arreter et accepter des connections */
  {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); /*On accepte une socket donc une connexion  */
    if (newsockfd < 0)
    {
      printf("accept error\n");
      exit(1);
    }

    bzero(gbuffer,256);
    n = read(newsockfd,gbuffer,255);
    if (n < 0)
    {
      printf("read error\n");
      exit(1);
    }
    pthread_mutex_lock( &mutex );
    synchro=1;
    pthread_mutex_unlock( &mutex );

    while (synchro);

  }
}

/**
* \fn void   sendMessageToServer(char *ipAddress, int portno, char *mess)
* \param char *ipAddress est l'adresse iP du serveur auquel envoyer le messages
* \param int portno est le port à utiliser pour le socket que l'on va créer
* \param char *mess est le message à envoyer
* La fonction permet d'envoyer un message, en suivant le protocol mis en place
*/

void sendMessageToServer(char *ipAddress, int portno, char *mess)
{
  int sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char sendbuffer[256];

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  server = gethostbyname(ipAddress);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
  (char *)&serv_addr.sin_addr.s_addr,
  server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
  {
    printf("ERROR connecting\n");
    exit(1);
  }

  sprintf(sendbuffer,"%s\n",mess);
  n = write(sockfd,sendbuffer,strlen(sendbuffer));

  close(sockfd);
}



/**
 * \fn int main(int argc, char *argv[])
 * \brief Fonction principale
 * \param char *argv[] prend le port du serveur, l'adresse IP du serveur, le port client à utiliser, l'adresse IP du client
 * \warning Executer comme suis: ./sh13  <ip serveur>  <n°port serveur>  <ip client> <n°port client> <Nom du client>
 * \bug Quand les clients quittent le jeu en pleine partie, les noms dans les chats sont écrasés
**/

int main(int argc, char ** argv)
{
  int ret;
  int i,j;
  int Max_text_size = 10000;
  int iiii = 1;
  int quit = 0;
  SDL_Event event;
  int mx,my;
  char sendBuffer[256];
  char lname[256];
  int id;

  if (argc<6)
  {
    printf("<app> <Main server ip address> <Main server port> <Client ip address> <Client port> <player name>\n");
    exit(1);
  }

  strcpy(gServerIpAddress,argv[1]);
  gServerPort=atoi(argv[2]);
  strcpy(gClientIpAddress,argv[3]);
  gClientPort=atoi(argv[4]);
  strcpy(gName,argv[5]);

  SDL_Init(SDL_INIT_VIDEO);
  //SDL_StartTextInput(); /*!< Pas d'erreur ici chez moi */
  TTF_Init();
  SDL_Window * window = SDL_CreateWindow("SDL2 SH13",
  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0);

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Rect rect5 = {612, 480, 400, 300};
  char text[256] = "";
  char tab_text[8][256];


  strcat(text,"");
  printf("%s\n",text );


  SDL_Surface *deck[13],*objet[8],*gobutton,*connectbutton, *chatbutton,*quitbutton;
  char path[256] ;

  for (int i = 0; i < 13; i++) {
    sprintf(path,"./Image/SH13_%d.png",i);
    deck[i] = IMG_Load(path);
  }

  objet[0] = IMG_Load("./Image/SH13_pipe_120x120.png");
  objet[1] = IMG_Load("./Image/SH13_ampoule_120x120.png");
  objet[2] = IMG_Load("./Image/SH13_poing_120x120.png");
  objet[3] = IMG_Load("./Image/SH13_couronne_120x120.png");
  objet[4] = IMG_Load("./Image/SH13_carnet_120x120.png");
  objet[5] = IMG_Load("./Image/SH13_collier_120x120.png");
  objet[6] = IMG_Load("./Image/SH13_oeil_120x120.png");
  objet[7] = IMG_Load("./Image/SH13_crane_120x120.png");
  gobutton = IMG_Load("./Image/Go.png");
  quitbutton = IMG_Load("./Image/Quit.png");
  chatbutton = IMG_Load("./Image/chat.png");
  connectbutton = IMG_Load("./Image/ConnectButton.jpg");
  for (int i = 0; i < 4; i++) {
    strcpy(gNames[i],"-");
  }

  joueurSel=-1;
  objetSel=-1;
  guiltSel=-1;

  b[0]=-1;
  b[1]=-1;
  b[2]=-1;

  for (i=0;i<13;i++)
  guiltGuess[i]=0;

  for (i=0;i<4;i++)
  for (j=0;j<8;j++)
  tableCartes[i][j]=-1;

  goEnabled=0;
  connectEnabled=1;

  SDL_Texture *texture_deck[13],*texture_gobutton,*texture_connectbutton,*texture_objet[8],*texture_chatbutton,*texture_quitbutton;

  for (i=0;i<13;i++)
  texture_deck[i] = SDL_CreateTextureFromSurface(renderer, deck[i]);
  for (i=0;i<8;i++)
  texture_objet[i] = SDL_CreateTextureFromSurface(renderer, objet[i]);

  texture_gobutton = SDL_CreateTextureFromSurface(renderer, gobutton);
  texture_connectbutton = SDL_CreateTextureFromSurface(renderer, connectbutton);
  texture_chatbutton = SDL_CreateTextureFromSurface(renderer, chatbutton);
  texture_quitbutton = SDL_CreateTextureFromSurface(renderer, quitbutton);
  TTF_Font* Sans = TTF_OpenFont("./sans.ttf", 15);
  TTF_Font* Sans2 = TTF_OpenFont("./sans.ttf", 15);
  printf("Sans=%p\n",Sans);

  /* Creation du thread serveur tcp. */
  printf ("Creation du thread serveur tcp !\n");
  synchro=0;
  ret = pthread_create ( & thread_serveur_tcp_id, NULL, fn_serveur_tcp, NULL);
  int nbrmessages = 0;
  while (!quit)
  {
    if (SDL_PollEvent(&event))
    {
      //printf("un event\n");
      switch (event.type)
      {
        case SDL_KEYDOWN:
        if (chatEnable == 1) {
          if (event.key.keysym.sym  == SDLK_RETURN){
            sprintf(sendBuffer,"Z %d %s",gId,text);
            printf("%s\n",text );
            strcpy(text,"");
            iiii = 1;
            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
          }
          else if (event.key.keysym.sym  == SDLK_BACKSPACE) {
            text[strlen(text)-1] = '\0';
          }
        }

        if (event.key.keysym.sym  == SDLK_ESCAPE) {
          sprintf(sendBuffer,"Q %d",gId);
          sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
          quit =1;
        }

        break;
        case SDL_TEXTINPUT:
                    /* Add new text onto the end of our text */
                    strcat(text, event.text.text);
        break;

        case SDL_QUIT:
          sprintf(sendBuffer,"Q %d",gId);
          sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
          quit = 1;
        break;
        case  SDL_MOUSEBUTTONDOWN:
        SDL_GetMouseState( &mx, &my );
        //printf("mx=%d my=%d\n",mx,my);
        if ((mx<200) && (my<50) && (connectEnabled==1))
        {
          sprintf(sendBuffer,"C %s %d %s",gClientIpAddress,gClientPort,gName);
          sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
          connectEnabled=0;
        }
        else if ((mx>=0) && (mx<200) && (my>=90) && (my<330))
        {
          joueurSel=(my-90)/60;
          guiltSel=-1;
        }
        else if ((mx>=200) && (mx<680) && (my>=0) && (my<90))
        {
          objetSel=(mx-200)/60;
          guiltSel=-1;
        }
        else if ((mx>=100) && (mx<250) && (my>=350) && (my<740))
        {
          joueurSel=-1;
          objetSel=-1;
          guiltSel=(my-350)/30;

        }
        else if ((mx>=250) && (mx<300) && (my>=350) && (my<740))
        {
          int ind=(my-350)/30;
          guiltGuess[ind]=1-guiltGuess[ind];
        }
        else if ((mx>=500) && (mx<550) && (my>=400) && (my<450) && (goEnabled==1))
        {
          printf("go! joueur=%d objet=%d guilt=%d\n",joueurSel, objetSel, guiltSel);
          if (guiltSel!=-1)
          {
            sprintf(sendBuffer,"G %d %d",gId, guiltSel);
            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
          }
          else if ((objetSel!=-1) && (joueurSel==-1))
          {
            sprintf(sendBuffer,"O %d %d",gId, objetSel);
            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);

          }
          else if ((objetSel!=-1) && (joueurSel!=-1))
          {
            sprintf(sendBuffer,"S %d %d %d",gId, joueurSel,objetSel);
            sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);

          }
        }else if ((mx <= 550) && (my <= 650) && (mx >= 500) && (my >= 600) ) {
          // Exécution du chat
          chatEnable *= -1;
          if (chatEnable == 1) {
              SDL_StartTextInput();
              SDL_SetTextInputRect(&rect5);
          }
          else{
            iiii = 1;
            SDL_StopTextInput();
          }
          // peut être qu'il faut le faire dans le rendererSDL_StartTextInput();
          // Il faut que l'input se fasse sur le rectangle du chat où sera render les messages
          // Le ser
        }else if ((mx < 550) && (my <= 550) && (mx >= 500) && (my > 500) ){
          sprintf(sendBuffer,"Q %d",gId);
          sendMessageToServer(gServerIpAddress, gServerPort, sendBuffer);
          quit=1;
        }

        else
        {
          joueurSel=-1;
          objetSel=-1;
          guiltSel=-1;
        }
        break;
        case  SDL_MOUSEMOTION:
        SDL_GetMouseState( &mx, &my );
        break;
      }
    }

    if (synchro==1)
    {
      pthread_mutex_lock( &mutex );
      printf("consomme |%s|\n",gbuffer);
      int ob;
      char c1[5];
      switch (gbuffer[0])
      {
        case 'Q':
        sscanf(gbuffer,"Q %d",&joueurSel);
        // reinitialiser b[], reinitialiser  table carte, actualiser table nom avec L, changement d'id
        for (int i = 0; i < 3; i++) {
          b[i] = -1; //Ou 0?
        }
        for (int i = 0; i < 8; i++) {
          for (int j = 0; j < 4; j++) {
            tableCartes[j][i] = -1; //ou 0?
          }
        }
        // Message 'I' : le joueur recoit son Id
        case 'I':
        sscanf(gbuffer,"I %d",&gId);
        break;
        // Message 'L' : le joueur recoit la liste des joueurs
        case 'L':
        sscanf(gbuffer,"L %s %s %s %s",gNames[0],gNames[1],gNames[2],gNames[3]);
        break;
        // Message 'D' : le joueur recoit ses trois cartes
        case 'D':
        sscanf(gbuffer,"D %d %d %d",&b[0],&b[1], &b[2]);
        //printf("%s\n", nbnoms[b[0]]);
        break;
        // Message 'M' : le joueur recoit le n° du joueur courant
        // Cela permet d'affecter goEnabled pour autoriser l'affichage du bouton g
        case 'M':
        sscanf(gbuffer,"M %d",&id);
        if (id == gId) {
          goEnabled = 1;
        }
        else{
          goEnabled = 0;
        }
        break;
        // Message 'V' : le joueur recoit une valeur de tableCartes
        case 'V':
        sscanf(gbuffer,"V %d %d %d %d %d %d %d %d ",&tableCartes[gId][0],&tableCartes[gId][1], &tableCartes[gId][2], &tableCartes[gId][3], &tableCartes[gId][4], &tableCartes[gId][5], &tableCartes[gId][6], &tableCartes[gId][7]);
        break;
        case 'O':
        sscanf(gbuffer,"O %d %c %c %c %c",&ob, &c1[0], &c1[1], &c1[2], &c1[3]);
        for (int i = 0; i < 4; i++) {
          if (i!= gId) {

              if (c1[i] == '0') {
                tableCartes[i][ob] = 0;
              }
              else if (tableCartes[i][ob] == -1)
              {tableCartes[i][ob] = 100;}



          }
        }
        break;
        case 'S':
        sscanf(gbuffer,"S %d %d %d",&joueurSel, &objetSel, &ob);
        tableCartes[joueurSel][objetSel] = ob;
        break;
        case 'W': exit(1);
        case 'Y': exit(1);
        case 'Z':
        sscanf(gbuffer,"Z %[^Z]s", texte_courant);
        char texte_temporaire[100];
        int compteur = 0;
        for(int r = 0; r<8; r++){
	        while(texte_courant[compteur] != '@'){
	        	compteur++;
	        	if (compteur > strlen(texte_courant))
	        	{
	        		break;
	        	}
	        }
	        if (texte_courant[compteur] == '@')
	        {
	        	char nom_temporaire[40];
	        	int p = compteur+1;
	        	int q = 0;
	        	strcpy(texte_temporaire, texte_courant);
	        	while(texte_temporaire[p] != ' '){
	        		nom_temporaire[q] = texte_temporaire[p];
	        		q++;
	        		p++;
	        	}
	        	if (strcmp(gName, nom_temporaire) != 0)
	        	{
	        		while(texte_courant[p] != '\n'){
	        			texte_courant[p] = 42;
	        			p++;
	        		}
	        	}
	        	compteur = p;
	        }
	    }
	    compteur = 0;
        printf("texte courant recu  %s\n",texte_courant);
        break;
      }
      synchro=0;
      pthread_mutex_unlock( &mutex );
    }

    SDL_Rect dstrect_grille = { 512-250, 10, 500, 350 };
    SDL_Rect dstrect_image = { 0, 0, 500, 330 };
    SDL_Rect dstrect_image1 = { 0, 340, 250, 330/2 };

    SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
    SDL_Rect rect = {0, 0, 1024, 768};
    SDL_RenderFillRect(renderer, &rect);



    if (chatEnable ==1 )
    {
      int nb_b = 1;
      int o =0;
      while(texte_courant[o]!= '\0'){
        if (texte_courant[o]=='\n') {
          nb_b++;
        }
        o++;
      }

      SDL_SetRenderDrawColor(renderer, 0,0,0, 255); // couleur du rectangle
      SDL_RenderFillRect(renderer, &rect5);

      SDL_SetRenderDrawColor(renderer, 255,255,255, 255);
      SDL_RenderDrawLine(renderer, 612,715,1050,715);
      SDL_Color col5 = {255, 255, 255 };
      //MIN(Max_text_size,strlen(texte_courant)*10) y = 50*nb_b
      SDL_Rect rect7 = {612, 480,400,250};
      SDL_Surface* surfaceMessage2 = TTF_RenderText_Blended_Wrapped(Sans, texte_courant, col5, 390);
      SDL_Texture* Message2 = SDL_CreateTextureFromSurface(renderer, surfaceMessage2);
      SDL_RenderCopy(renderer, Message2, NULL, &rect7);
      SDL_DestroyTexture(Message2);
      SDL_FreeSurface(surfaceMessage2);

      int a = strlen(text);
      if (a*10 > 400*iiii) {   // Ici faire une tableauuuuuu des messsage à afficher si la taille dépasse
        if (iiii == 1) {
          Max_text_size =400;
        }
        iiii++;

      }
      SDL_Color col9 = {255, 165, 0 };
        //MIN(a*10,M)
        SDL_Rect rect6 = {612, 700, MIN(Max_text_size,a*10), 20*iiii*i};

        //SDL_Surface* surfaceMessage1 = TTF_RenderText_Solid(Sans2,text, col5);
        SDL_Surface* surfaceMessage1 = TTF_RenderText_Blended_Wrapped(Sans, text, col9, 50*i);
        SDL_Texture* Message1 = SDL_CreateTextureFromSurface(renderer, surfaceMessage1);
        SDL_RenderCopy(renderer, Message1, NULL, &rect6);
        SDL_DestroyTexture(Message1);
        SDL_FreeSurface(surfaceMessage1);

      }


    if (joueurSel!=-1)
    {
      SDL_SetRenderDrawColor(renderer, 255, 180, 180, 255);
      SDL_Rect rect1 = {0, 90+joueurSel*60, 200 , 60};
      SDL_RenderFillRect(renderer, &rect1);
    }

    if (objetSel!=-1)
    {
      SDL_SetRenderDrawColor(renderer, 180, 255, 180, 255);
      SDL_Rect rect1 = {200+objetSel*60, 0, 60 , 90};
      SDL_RenderFillRect(renderer, &rect1);
    }

    if (guiltSel!=-1)
    {
      SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255);
      SDL_Rect rect1 = {100, 350+guiltSel*30, 150 , 30};
      SDL_RenderFillRect(renderer, &rect1);
    }

    {
      SDL_Rect dstrect_pipe = { 210, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
      SDL_Rect dstrect_ampoule = { 270, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
      SDL_Rect dstrect_poing = { 330, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
      SDL_Rect dstrect_couronne = { 390, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
      SDL_Rect dstrect_carnet = { 450, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
      SDL_Rect dstrect_collier = { 510, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
      SDL_Rect dstrect_oeil = { 570, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
      SDL_Rect dstrect_crane = { 630, 10, 40, 40 };
      SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
    }




    SDL_Color col1 = {0, 0, 0 };
    for (i=0;i<8;i++)
    {
      SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, nbobjets[i], col1);
      SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
      SDL_Rect Message_rect; //create a rect
      Message_rect.x = 230+i*60;  //controls the rect's x coordinate
      Message_rect.y = 50; // controls the rect's y coordinte
      Message_rect.w = surfaceMessage->w; // controls the width of the rect
      Message_rect.h = surfaceMessage->h; // controls the height of the rect

      SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
      SDL_DestroyTexture(Message);
      SDL_FreeSurface(surfaceMessage);
    }

    for (i=0;i<13;i++)
    {
      SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, nbnoms[i], col1);
      SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

      SDL_Rect Message_rect;
      Message_rect.x = 105;
      Message_rect.y = 350+i*30;
      Message_rect.w = surfaceMessage->w;
      Message_rect.h = surfaceMessage->h;

      SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
      SDL_DestroyTexture(Message);
      SDL_FreeSurface(surfaceMessage);
    }

    for (i=0;i<4;i++)
    for (j=0;j<8;j++)
    {
      if (tableCartes[i][j]!=-1)
      {
        char mess[10];
        if (tableCartes[i][j]==100)
        sprintf(mess,"*");
        else
        sprintf(mess,"%d",tableCartes[i][j]);
        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, mess, col1);
        SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

        SDL_Rect Message_rect;
        Message_rect.x = 230+j*60;
        Message_rect.y = 110+i*60;
        Message_rect.w = surfaceMessage->w;
        Message_rect.h = surfaceMessage->h;

        SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
        SDL_DestroyTexture(Message);
        SDL_FreeSurface(surfaceMessage);
      }
    }


    // Sebastian Moran
    {
      SDL_Rect dstrect_crane = { 0, 350, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
    }
    {
      SDL_Rect dstrect_poing = { 30, 350, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
    }
    // Irene Adler
    {
      SDL_Rect dstrect_crane = { 0, 380, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
    }
    {
      SDL_Rect dstrect_ampoule = { 30, 380, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
    }
    {
      SDL_Rect dstrect_collier = { 60, 380, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
    }
    // Inspector Lestrade
    {
      SDL_Rect dstrect_couronne = { 0, 410, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
    }
    {
      SDL_Rect dstrect_oeil = { 30, 410, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
    }
    {
      SDL_Rect dstrect_carnet = { 60, 410, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
    }
    // Inspector Gregson
    {
      SDL_Rect dstrect_couronne = { 0, 440, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
    }
    {
      SDL_Rect dstrect_poing = { 30, 440, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
    }
    {
      SDL_Rect dstrect_carnet = { 60, 440, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
    }
    // Inspector Baynes
    {
      SDL_Rect dstrect_couronne = { 0, 470, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
    }
    {
      SDL_Rect dstrect_ampoule = { 30, 470, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
    }
    // Inspector Bradstreet
    {
      SDL_Rect dstrect_couronne = { 0, 500, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
    }
    {
      SDL_Rect dstrect_poing = { 30, 500, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
    }
    // Inspector Hopkins
    {
      SDL_Rect dstrect_couronne = { 0, 530, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[3], NULL, &dstrect_couronne);
    }
    {
      SDL_Rect dstrect_pipe = { 30, 530, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
    }
    {
      SDL_Rect dstrect_oeil = { 60, 530, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
    }
    // Sherlock Holmes
    {
      SDL_Rect dstrect_pipe = { 0, 560, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
    }
    {
      SDL_Rect dstrect_ampoule = { 30, 560, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
    }
    {
      SDL_Rect dstrect_poing = { 60, 560, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
    }
    // John Watson
    {
      SDL_Rect dstrect_pipe = { 0, 590, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
    }
    {
      SDL_Rect dstrect_oeil = { 30, 590, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[6], NULL, &dstrect_oeil);
    }
    {
      SDL_Rect dstrect_poing = { 60, 590, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[2], NULL, &dstrect_poing);
    }
    // Mycroft Holmes
    {
      SDL_Rect dstrect_pipe = { 0, 620, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
    }
    {
      SDL_Rect dstrect_ampoule = { 30, 620, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
    }
    {
      SDL_Rect dstrect_carnet = { 60, 620, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
    }
    // Mrs. Hudson
    {
      SDL_Rect dstrect_pipe = { 0, 650, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[0], NULL, &dstrect_pipe);
    }
    {
      SDL_Rect dstrect_collier = { 30, 650, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
    }
    // Mary Morstan
    {
      SDL_Rect dstrect_carnet = { 0, 680, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[4], NULL, &dstrect_carnet);
    }
    {
      SDL_Rect dstrect_collier = { 30, 680, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[5], NULL, &dstrect_collier);
    }
    // James Moriarty
    {
      SDL_Rect dstrect_crane = { 0, 710, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[7], NULL, &dstrect_crane);
    }
    {
      SDL_Rect dstrect_ampoule = { 30, 710, 30, 30 };
      SDL_RenderCopy(renderer, texture_objet[1], NULL, &dstrect_ampoule);
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    // Afficher les suppositions
    for (i=0;i<13;i++)
    if (guiltGuess[i])
    {
      SDL_RenderDrawLine(renderer, 250,350+i*30,300,380+i*30);
      SDL_RenderDrawLine(renderer, 250,380+i*30,300,350+i*30);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 1; i < 6; i++) {
      SDL_RenderDrawLine(renderer, 0,30+i*60,680,30+i*60);
    }

    for (int i = 0; i < 9; i++) {
      SDL_RenderDrawLine(renderer, 200+i*60,0,200+i*60,330);
    }

    for (i=0;i<14;i++)
    SDL_RenderDrawLine(renderer, 0,350+i*30,300,350+i*30);
    SDL_RenderDrawLine(renderer, 100,350,100,740);
    SDL_RenderDrawLine(renderer, 250,350,250,740);
    SDL_RenderDrawLine(renderer, 300,350,300,740);

    //SDL_RenderCopy(renderer, texture_grille, NULL, &dstrect_grille);
    if (b[0]!=-1)
    {
      SDL_Rect dstrect = { 750, 0, 1000/4, 660/4 };
      SDL_RenderCopy(renderer, texture_deck[b[0]], NULL, &dstrect);
    }
    if (b[1]!=-1)
    {
      SDL_Rect dstrect = { 750, 660/4, 1000/4, 660/4 };
      SDL_RenderCopy(renderer, texture_deck[b[1]], NULL, &dstrect);
    }
    if (b[2]!=-1)
    {
      SDL_Rect dstrect = { 750, 660/2, 1000/4, 660/4 };
      SDL_RenderCopy(renderer, texture_deck[b[2]], NULL, &dstrect);
    }

    if (goEnabled)
    {
      SDL_Rect dstrect = { 500, 400, 50, 50 };
      SDL_RenderCopy(renderer, texture_gobutton, NULL, &dstrect);
    }
    // Le bouton connect
    if (connectEnabled==1)
    {
      SDL_Rect dstrect = { 0, 0, 200, 50 };
      SDL_RenderCopy(renderer, texture_connectbutton, NULL, &dstrect);
    }

    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    //SDL_RenderDrawLine(renderer, 0, 0, 200, 200);


    SDL_Rect dstrect = { 500, 500, 50, 50 };
    SDL_RenderCopy(renderer, texture_quitbutton, NULL, &dstrect);
    SDL_Rect dstrect1 = { 500, 600, 50, 50 };
    SDL_RenderCopy(renderer, texture_chatbutton, NULL, &dstrect1);


    SDL_Color col = {0, 0, 0};
    for (i=0;i<4;i++)
    if (strlen(gNames[i])>0)
    {
      SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, gNames[i], col);
      SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

      SDL_Rect Message_rect; //create a rect
      Message_rect.x = 10;  //controls the rect's x coordinate
      Message_rect.y = 110+i*60; // controls the rect's y coordinte
      Message_rect.w = surfaceMessage->w; // controls the width of the rect
      Message_rect.h = surfaceMessage->h; // controls the height of the rect

      SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
      SDL_DestroyTexture(Message);
      SDL_FreeSurface(surfaceMessage);
    }

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture_deck[0]);
  SDL_DestroyTexture(texture_deck[1]);
  SDL_FreeSurface(deck[0]);
  SDL_FreeSurface(deck[1]);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}
