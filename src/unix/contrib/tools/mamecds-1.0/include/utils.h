#ifndef __UTILS_H__
#define __UTILS_H__

#define	OK		0
#define ERROR	-1

#define MAXCAD		128
#define MAXJUEGOS	5000

#define MAME	1
#define NEOGEO	2
#define FULL	4

/****************************************/
/*										*/
/*	basedir								*/
/*		tmpdir							*/
/*			cddir						*/
/*				romsdir					*/
/*					neogeodir			*/
/*					mamedir				*/
/*					samplesdir			*/
/*					artworkdir			*/
/*				listadir				*/
/*					imagesdir			*/
/*					titlesdir			*/
/*										*/
/****************************************/
char srcbasedir[MAXCAD];	/* Directorio roms:				"/home/carlitos/roms"		*/

char srcromsdir[MAXCAD];	/* Directorio neogeo/mame):		"/home/carlitos/roms"		*/
char srclistdir[MAXCAD];	/* Directorio con la lista:		"/home/lista"				*/
char dstromsdir[MAXCAD];	/* Directorio completo roms:	"./tmp/mame-01/roms"		*/
char dstlistdir[MAXCAD];	/* Directorio completo lista:	"./tmp/mame-01/lista"		*/

char basedir[MAXCAD];		/* Directorio base:				"."							*/
char tmpdir[MAXCAD];		/* Directorio temporal:			"tmp"						*/
char cddir[MAXCAD];			/* Directorio del cd:			"neogeo-01"					*/
char romsdir[MAXCAD];		/* Directorio de roms general: 	"roms" 						*/
char neogeodir[MAXCAD];		/* Directorio de roms neogeo:	"neogeo"					*/
char mamedir[MAXCAD];		/* Directorio de roms mame:		"mame"						*/
char samplesdir[MAXCAD];	/* Directorio de samples:		"samples"					*/
char artworkdir[MAXCAD];	/* Directorio de artwork:		"artwork"					*/
char listdir[MAXCAD];		/* Directorio con la lista:		"lista"						*/
char imagesdir[MAXCAD];		/* Directorio de imagenes:		"images"					*/
char titlesdir[MAXCAD];		/* Directorio de imagenes:		"titulos"					*/

char versionmame[MAXCAD];	/* Version del mame */

#endif
