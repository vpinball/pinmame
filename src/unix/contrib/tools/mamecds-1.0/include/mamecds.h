#ifndef __MAMECDS_H__
#define __MAMECDS_H__

#include "utils.h"
#include "gamelist.h"

/* 60 minutos, 75 bloques por segundo, 2048 bytes por bloque */
#define MULTIPLIERB     1
#define MULTIPLIERK     1024
#define MULTIPLIERM     (1024*1024)
#define MULTIPLIERm     (60*75*2048)

/* Para el retorno de MakeCD */
#define LASTCD			0
#define MORECDS			1


unsigned long	tamanyocd;		/* Tamanyo de la imagen ISO a generar 	*/
int	neogeocd;		/* Flag de creacion de CD's de neogeo	*/
int	mamecd;			/* Flag de creacion de CD's de mame		*/
int generatecds;	/* Flag para generar las ISOS 1 si 0 no	*/
int generatelist;	/* Flag para generar la lista de juegos	*/
int generatefulllist;	/* Flag para generar la lista completa de juegos	*/

char mamebin[MAXCAD];
char listgamesarg[MAXCAD];
char listclonesarg[MAXCAD];

int CreateCDS (struct mamegame *lista, int tipo);

#endif
