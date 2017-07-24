#include <stdio.h>

#include "utils.h"
#include "mamecds.h"
#include "gamelist.h"

int cuenta_juegos (struct mamegame *lista, int tipo, int ncd)
{   
	int i;
	struct mamegame *juego;

	njuegos_mame=0;
	njuegos_neogeo=0;

	for(i=GAME_1; i<=GAME_Z; i++) 
	{
		njuegos_letra[i]=0;
	}
	njuegos_letra[GAME_NEOGEO]=0;

	if ((tipo==MAME)||(tipo==FULL))
	{
		juego=lista;
		while(juego!=NULL)
		{
			if (tipo==FULL) juego->numerocd=ncd;

			if (((juego->tipo==MAME) || (juego->tipo==FULL)) && (juego->numerocd==ncd))
			{
				if  ( (juego->nombre[0]=='0') || (juego->nombre[0]=='1') || (juego->nombre[0]=='2')
						||(juego->nombre[0]=='3') || (juego->nombre[0]=='4') || (juego->nombre[0]=='5')
						||(juego->nombre[0]=='6') || (juego->nombre[0]=='7') || (juego->nombre[0]=='8')
						||(juego->nombre[0]=='9') || (juego->nombre[0]=='\''))
					njuegos_letra[GAME_1]++;

				if (juego->nombre[0]=='A') njuegos_letra[GAME_A]++;
				if (juego->nombre[0]=='B') njuegos_letra[GAME_B]++;
				if (juego->nombre[0]=='C') njuegos_letra[GAME_C]++;
				if (juego->nombre[0]=='D') njuegos_letra[GAME_D]++;
				if (juego->nombre[0]=='E') njuegos_letra[GAME_E]++;
				if (juego->nombre[0]=='F') njuegos_letra[GAME_F]++;
				if (juego->nombre[0]=='G') njuegos_letra[GAME_G]++;
				if (juego->nombre[0]=='H') njuegos_letra[GAME_H]++;
				if (juego->nombre[0]=='I') njuegos_letra[GAME_I]++;
				if (juego->nombre[0]=='J') njuegos_letra[GAME_J]++;
				if (juego->nombre[0]=='K') njuegos_letra[GAME_K]++;
				if (juego->nombre[0]=='L') njuegos_letra[GAME_L]++;
				if (juego->nombre[0]=='M') njuegos_letra[GAME_M]++;
				if (juego->nombre[0]=='N') njuegos_letra[GAME_N]++;
				if (juego->nombre[0]=='O') njuegos_letra[GAME_O]++;
				if (juego->nombre[0]=='P') njuegos_letra[GAME_P]++;
				if (juego->nombre[0]=='Q') njuegos_letra[GAME_Q]++;
				if (juego->nombre[0]=='R') njuegos_letra[GAME_R]++;
				if (juego->nombre[0]=='S') njuegos_letra[GAME_S]++;
				if (juego->nombre[0]=='T') njuegos_letra[GAME_T]++;
				if (juego->nombre[0]=='U') njuegos_letra[GAME_U]++;
				if (juego->nombre[0]=='V') njuegos_letra[GAME_V]++;
				if (juego->nombre[0]=='W') njuegos_letra[GAME_W]++;
				if (juego->nombre[0]=='X') njuegos_letra[GAME_X]++;
				if (juego->nombre[0]=='Y') njuegos_letra[GAME_Y]++;
				if (juego->nombre[0]=='Z') njuegos_letra[GAME_Z]++;

				njuegos_mame++;
			}
			juego=juego->next;
		}
#ifdef DEBUG_COUNT
		printf("Mame %i\n",njuegos_mame);
#endif
	}

	if ((tipo==NEOGEO)||(tipo==FULL))
	{
		juego=lista;
		while(juego!=NULL)
		{
			if (tipo==FULL) juego->numerocd=ncd;

			if (((juego->tipo==NEOGEO) || (juego->tipo==FULL)) && (juego->numerocd==ncd))
			{
				njuegos_letra[GAME_NEOGEO]++;
				njuegos_neogeo++;
			}
			juego=juego->next;
		}
#ifdef DEBUG_COUNT
		printf("Neogeo %i\n",njuegos_neogeo);
#endif
	}

	if (tipo==MAME)
	{	
#ifdef DEBUG_COUNT
		for(i=GAME_1; i<=GAME_Z; i++) 
		{
			if (i==GAME_1) printf("Numero de juegos 1-9 \t%i\n",njuegos_letra[i]);
			else printf("Numero de juegos %c \t%i\n",i+0x60,njuegos_letra[i]);
		}
#endif
		return(njuegos_mame);
	}
	if (tipo==NEOGEO)
	{	
#ifdef DEBUG_COUNT
		printf("Numero de juegos neogeo \t%i\n",njuegos_letra[GAME_NEOGEO]);
#endif
		return(njuegos_neogeo);
	}

	if (tipo==FULL)
	{	
#ifdef DEBUG_COUNT
		for(i=GAME_1; i<=GAME_Z; i++) 
		{
			if (i==GAME_1) printf("Numero de juegos 1-9 \t%i\n",njuegos_letra[i]);
			else printf("Numero de juegos %c \t%i\n",i+0x60,njuegos_letra[i]);
		}
		printf("Numero de juegos neogeo \t%i\n",njuegos_letra[GAME_NEOGEO]);
#endif
		return(njuegos_neogeo+njuegos_mame);
	}

	return(OK);
}

/************************************************************/
/* 															*/
/* Funcion de ordenacion para el qsort						*/
/* 															*/
/************************************************************/
int ComparaJuegos(struct mamegame *game1, struct mamegame *game2)
{
	return(strcmp(game1->nombre, game2->nombre));
}

/************************************************************/
/* 															*/
/* Generamos la lista de juegos a partir del ejecutable		*/
/* Retorno:													*/
/* 			OK -----> La lista se genera correctamente		*/
/* 			ERROR --> Error, el ejecuatable no es valido	*/
/* 					  o error inesperado					*/
/* 															*/
/************************************************************/
struct mamegame *MakeGameList(char *mame)
{
	int retclone, i, k;
	int njuegos=0;
	int clonecount;
	struct mamegame *lista, *anterior, *actual, *todos;
	char comando[MAXCAD];
	char basura[MAXCAD];
	char linea[MAXCAD];
	FILE *pmame, *pclones;
	struct stat buff;

	char base[MAXCAD];
	char filename[MAXCAD];
	char aux1[MAXCAD], aux2[MAXCAD], aux3[MAXCAD], aux4[MAXCAD];

	int rline=0;
	int nozip=0;
	int notitle=0;


	todos=(struct mamegame *)calloc(MAXJUEGOS,sizeof(struct mamegame));

	/* Ejecutamos el mame para conseguir la version */
	sprintf(comando,"%s --version 2>/dev/null", mame);
	pmame=popen(comando,"r");

	fscanf(pmame,"%s %s %s %s %s %s\n", aux1, basura, basura, aux2, aux3, aux4); /* xmame (SDL) version 0.37 BETA 13 (Mar 31 2001) */
	sprintf(versionmame,"%s %s %s %s",aux1, aux2, aux3, aux4);
	printf("%s\n",versionmame);

	lista=NULL;
	/* Ejecutamos el mame para conseguir la lista de juegos */
	sprintf(comando,"%s --listfull 2>/dev/null", mame);
	pmame=popen(comando,"r");

	fscanf(pmame,"%s %s\n", basura, basura); /* name      description */
	fscanf(pmame,"%s %s\n", basura, basura); /* --------  -----------*/
	
	i=0;
	/*fprintf(stderr,"Scanning games: ");*/
	while (1)
	{
		fgets(linea,MAXCAD,pmame);

		/* La ultima linea esta en blanco */
		if (linea[0]=='\n') break;

		sscanf(linea, "%s", todos[i].base);
		fprintf(stderr,"Scanning game: (%5i) %8s ", i+1, todos[i].base);
		rline=0;
		nozip=0;
		notitle=0;

		linea[strlen(linea)-2]='\0';
		strcpy(todos[i].nombre, &linea[11]);

		/*********************************/		
		/*********************************/		
		/*********************************/		
		/*if (i==99) break;*/
		/*********************************/		
		/*********************************/		
		/*********************************/		

		if (strcmp(todos[i].base,"Total")==0) break;

		/* ¿Existe el fichero .zip? ¿Es de NEOGEO o de MAME? */
		sprintf(filename,"%s/%s/%s.zip",srcbasedir,neogeodir,todos[i].base);
		if (stat(filename,&buff)==-1)
		{
			sprintf(filename,"%s/%s/%s.zip",srcbasedir,mamedir,todos[i].base);
			if (stat(filename,&buff)==-1)
			{
				fprintf(stderr, "\t!roms");
				todos[i].tipo=MAME;
				todos[i].tamanyo=0;
				todos[i].tamanyorom=0;
				todos[i].tamanyosamples=0;
				todos[i].tamanyoartwork=0;
				todos[i].hasgame=0;
				rline=1;
				nozip=1;
			}
			else
				{
					/* Es de mame */
					todos[i].tipo=MAME;
					todos[i].tamanyo=buff.st_size;
					todos[i].tamanyorom=buff.st_size;
					todos[i].hasgame=1;
				}
		}
		else
			{
				/* Es de neogeo */
				todos[i].tipo=NEOGEO;
				todos[i].tamanyo=buff.st_size;
				todos[i].tamanyorom=buff.st_size;
				todos[i].hasgame=1;
			}

		/* ¿Existen las imagenes del titulo y del juego? */
		sprintf(todos[i].titleimage,"%s/%s/%s.gif",srclistdir,titlesdir,todos[i].base);
		if (stat(todos[i].titleimage,&buff)==-1)
		{
			if (!nozip) 
			{
				fprintf(stderr,"\t");
				nozip=1;
			}
			fprintf(stderr, "\t!title");
			todos[i].hastitle=0;
			rline=1;
			notitle=1;
		}
		else
		{
			todos[i].tamanyo+=buff.st_size;
			todos[i].hastitle=1;
		}
		sprintf(todos[i].gameimage,"%s/%s/%s.gif",srclistdir,imagesdir,todos[i].base);
		if (stat(todos[i].gameimage,&buff)==-1)
		{
			if (!nozip) fprintf(stderr,"\t");
			if (!notitle) fprintf(stderr,"\t");

			fprintf(stderr, "\t!image");
			todos[i].hasimage=0;
			rline=1;
		}
		else
		{
			todos[i].tamanyo+=buff.st_size;
			todos[i].hasimage=1;
		}

		/* ¿Existe el fichero de samples? */
		sprintf(todos[i].samplesrom,"%s/%s/%s.zip",srcbasedir,samplesdir,todos[i].base);
		if (stat(todos[i].samplesrom,&buff)==-1)
		{
			/*fprintf(stderr, "\nERROR: %s/%s/%s.zip no existe\n",srcromsdir,samplesdir,todos[i].base);*/
			todos[i].hassamples=0;
		}
		else
			{
				todos[i].tamanyo+=buff.st_size;
				todos[i].tamanyosamples=buff.st_size;
				todos[i].hassamples=1;
			}
		/* ¿Existe el fichero de artwork? */
		sprintf(todos[i].artwork,"%s/%s/%s.zip",srcbasedir,artworkdir,todos[i].base);
		if (stat(todos[i].artwork,&buff)==-1)
		{
			/*fprintf(stderr, "\nERROR: %s/%s/%s.zip no existe\n",srcromsdir,artworkdir,todos[i].base);*/
			todos[i].hasartwork=0;
		}
		else
			{
				todos[i].tamanyo+=buff.st_size;
				todos[i].tamanyoartwork=buff.st_size;
				todos[i].hasartwork=1;
			}
	
		todos[i].numerocd=0;	/* No esta en ningun CD de momento */
		/* ¿Es un clonico? */
		sprintf(comando,"%s --listclones %s 2>/dev/null", mame, todos[i].base);
		pclones=popen(comando,"r");
		if (fscanf(pclones, "%s %s %s\n", basura, basura, basura)!= EOF)	/* Name: 	Clone of: 	*/
		{
			retclone=0;
			clonecount=0;
			k=0;
			while (retclone!=EOF)
			{
				basura[0]='\0';
				base[0]='\0';
				retclone=fscanf(pclones, "%s %s\n", base, basura);			/* ........ ........	*/ 
				if (strcmp(base,todos[i].base)==0)
				{
					/* Si esta el primero es que es clonico de basura */
					sprintf(todos[i].cloneof,"%s",basura);
					todos[i].isclone=1;
					todos[i].hasclones=0;
				}
				else
					{
						/* Si esta el segundo es que tiene clonicos */
						if (strcmp(basura,todos[i].base)==0)
						{
							strcpy(todos[i].clones[k],base);
							k++;
							todos[i].isclone=0;
							todos[i].hasclones=1;
						}
					}
			}
		}
		else
			{
				todos[i].isclone=0;
			}

		pclose(pclones);

		/* Un poquito de verbose */		
		/*
		if (todos[i].tipo==NEOGEO) 	printf(" neogeo");
		if (todos[i].tipo==MAME) 	printf("   mame");
		printf(" %10i  %02i  %i%i%i%i%i%i %5s %50s %8s %8s %s\n",	todos[i].tamanyorom,
																		todos[i].numerocd, 
																		todos[i].hasgame,
																		todos[i].isclone,
																		todos[i].hastitle,
																		todos[i].hasimage,
																		todos[i].hassamples,
																		todos[i].hasartwork,
																		todos[i].anyo,
																		todos[i].fabricante,
																		todos[i].base, 
																		todos[i].cloneof, 
																		todos[i].nombre);

		k=0;
		while (todos[i].clones[k][0]!='\0')
		{
			printf("                                                                                                 %8s\n",todos[i].clones[k++]);
		}
		*/

		i++;
		njuegos++;

		if (rline) fprintf(stderr,"\n");
			else fprintf(stderr,"\r");
	}
	fprintf(stderr,"\n");

	/* Cerramos el fichero con la lista de juegos */
	pclose(pmame);	

	/* Ordenamos la lista */
	qsort(todos, njuegos, sizeof(struct mamegame), (const void *)ComparaJuegos);

	/* rellenamos ls lista enlazada */
	for (i=0; i<njuegos;i++)
	{
		actual=(struct mamegame *)calloc(1, sizeof(struct mamegame));
		actual->next=NULL;

		strcpy(actual->base, todos[i].base);
		strcpy(actual->nombre, todos[i].nombre);
		strcpy(actual->anyo, todos[i].anyo);
		strcpy(actual->fabricante, todos[i].fabricante);
		strcpy(actual->cloneof, todos[i].cloneof);
		strcpy(actual->samplesrom, todos[i].samplesrom);
		strcpy(actual->artwork, todos[i].artwork);
		strcpy(actual->titleimage, todos[i].titleimage);
		strcpy(actual->gameimage, todos[i].gameimage);
		actual->tipo=todos[i].tipo;
		actual->tamanyo=todos[i].tamanyo;
		actual->tamanyorom=todos[i].tamanyorom;
		actual->tamanyosamples=todos[i].tamanyosamples;
		actual->tamanyoartwork=todos[i].tamanyoartwork;
		actual->numerocd=todos[i].numerocd;
		actual->hasgame=todos[i].hasgame;
		actual->isclone=todos[i].isclone;
		actual->hasclones=todos[i].hasclones;
		actual->hastitle=todos[i].hastitle;
		actual->hasimage=todos[i].hasimage;
		actual->hassamples=todos[i].hassamples;
		actual->hasartwork=todos[i].hasartwork;
		k=0;
		while (todos[i].clones[k][0]!='\0')
		{
			strcpy(actual->clones[k],todos[i].clones[k]);
			k++;
		}

		if (i==0) lista=actual;
			else anterior->next=actual;

		anterior=actual;
	}

	free(todos);
	return(lista);
}

/************************************************************/
/* 															*/
/* Imprime la lista de juegos (para debug)					*/
/* 															*/
/************************************************************/
int PrintGameList(struct mamegame *lista)
{
	int i;
	struct mamegame *actual;

	actual=lista;

	printf("\n");
	printf("Type          Size  CD  RCCTISA Base     Clone of year  Game\n");
	printf("------- ----------  --  ------- -------- -------- ----- -------------------------------------------------------\n");

	while(actual!=NULL)
	{
		if (actual->tipo==NEOGEO) 	printf(" Neogeo");
		if (actual->tipo==MAME) 	printf("   Mame");
		printf(" %10i  %02i  %i%i%i%i%i%i%i %8s %8s %5s %s %s\n",	actual->tamanyorom,
																	actual->numerocd, 
																	actual->hasgame,
																	actual->isclone,
																	actual->hasclones,
																	actual->hastitle,
																	actual->hasimage,
																	actual->hassamples,
																	actual->hasartwork,
																	actual->base, 
																	actual->cloneof, 
																	actual->anyo,
																	actual->nombre,
																	actual->fabricante);

		i=0;
		while (actual->clones[i][0]!='\0')
		{
			printf("                                         %8s\n",actual->clones[i++]);
		}
		
		actual=actual->next;
	}
	printf("\n");
	return(OK);
}
