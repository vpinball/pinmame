#ifndef __MAMELIST_H__
#define __MAMELIST_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define GAME_1      0
#define GAME_A      1
#define GAME_B      2
#define GAME_C      3
#define GAME_D      4
#define GAME_E      5
#define GAME_F      6
#define GAME_G      7
#define GAME_H      8
#define GAME_I      9
#define GAME_J      10
#define GAME_K      11
#define GAME_L      12
#define GAME_M      13
#define GAME_N      14
#define GAME_O      15
#define GAME_P      16
#define GAME_Q      17
#define GAME_R      18
#define GAME_S      19
#define GAME_T      20
#define GAME_U      21
#define GAME_V      22
#define GAME_W      23
#define GAME_X      24
#define GAME_Y      25
#define GAME_Z      26
#define GAME_NEOGEO 27

typedef struct mamegame
{
	char base[8+1];				/* Base del juego (para .zip, .jpg)		*/
	char nombre[MAXCAD];		/* Nombre real del juego				*/
	char anyo[5+1];				/* Año del juego						*/
	char fabricante[MAXCAD];	/* Fabricante del juego					*/

	char cloneof[8+1];			/* "base" es clonico de "clone"			*/
	char clones[50][8+1];		/* clonicos de "base"					*/
	char samplesrom[MAXCAD];	/* Fichero con los samples del juego	*/
	char artwork[MAXCAD];		/* Fichero con el artwork del juego		*/
	char titleimage[MAXCAD];	/* Fichero con la imagen del titulo		*/
	char gameimage[MAXCAD];		/* Fichero con la imagen del juego		*/

	int hasgame;				/* ¿Existe el fichero de roms?			*/
	int isclone;				/* ¿Es un clonico?						*/
	int hasclones;				/* ¿Tiene juegos clonicos?				*/
	int hastitle;				/* ¿Tiene imagen del titulo?			*/
	int hasimage;				/* ¿Tiene imagen del juego?				*/
	int hassamples;				/* ¿Tiene fichero de samples?			*/
	int hasartwork;				/* ¿Tiene fichero de artwork?			*/

	int tipo;					/* MAME, NEOGEO, ...					*/
	int tamanyo;				/* Tamaño en bytes rom+samples+artwork+html+imagen+titulo */
	int tamanyorom;				/* Tamaño en bytes rom */
	int tamanyosamples;			/* Tamaño en bytes samples */
	int tamanyoartwork;			/* Tamaño en bytes artwork */
	int numerocd;				/* Numero de CD en el que esta incluido	*/

	struct mamegame *next;
} MAMEGAME;

int njuegos_letra[GAME_NEOGEO+1]; /* GAME_NEOGEO=27 */
int njuegos_mame;
int njuegos_neogeo;

struct mamegame *MakeGameList(char *mame);
int PrintGameList(struct mamegame *lista);
int cuenta_juegos(struct mamegame *lista, int tipo, int ncd);

#endif
