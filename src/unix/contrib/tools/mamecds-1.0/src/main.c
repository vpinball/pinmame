#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <ctype.h>
#include "gamelist.h"
#include "mamecds.h"
#include "html.h"

/************************************************************/
/*															*/
/* Imprime la ayuda del programa							*/
/*															*/
/************************************************************/
int Ayuda(char *progname)
{
	printf("\nUsage: %s [-n | --neogeo] [-m | --mame]\n",progname);
	printf("\t\t[-v | --version]\n");
	printf("\t\t[-s size | --size=size]\n");
	printf("\t\t[--nocd | -c]\n");
	printf("\t\t[--nolist | -l]\n");
	printf("\t\t[--fulllist | -f]\n");
	printf("\t\t[--help | -h]\n\n");

	printf("\t-h --help\tThis help screen\n");
	/*printf("\t-f --file\tUse file instead of ~/.mamecdsrc\n");*/
	printf("\t-v --version\tShow version information\n");
	printf("\t-n --neogeo\tMake neogeo CD's\n");
	printf("\t-m --mame\tMake mame CD's\n");
	printf("\t-c --nocd\tDon't generate the ISOs\n");
	printf("\t-l --nolist\tDon't generate the game list\n");
	printf("\t-f --fulllist\tCreates a full list in the 'srclistdir'\n");
	printf("\t-s --size\tSize of the ISO CD's (default 640Mb)\n");
	printf("\t\t\tYou can append B for bytes, K for kbytes,\n");
	printf("\t\t\tM for megabytes and m for minutes (default in Mb).\n");

	printf("\n");
	return(ERROR);
}

/************************************************************/
/*															*/
/* Saca una linea del fichero								*/
/*															*/
/************************************************************/
int ReadLine(FILE *fd, char *line)
{
	char c;
	int count,i;

	i=0;
	count=fread(&c,1,1,fd);
	while ((count==1) && (c!='\n'))
	{
		line[i++]=c;
		count=fread(&c,1,1,fd);
	}
	line[i]='\0';

	if (count==0) return(EOF);	
	return(OK);
}

/************************************************************/
/*															*/
/* Parsea los argumentos del fichero rc						*/
/*															*/
/************************************************************/
int Parserc(FILE *fd)
{
	char aux[MAXCAD];
	char iden[MAXCAD], value[MAXCAD];

	while (ReadLine(fd, aux)!=EOF)
	{
		/* comment */
		if (aux[0]=='#') continue;
		/* void line */
		if (aux[0]=='\0') continue;
		sscanf(aux,"%s %s",iden,value);

		/* CAMBIAR!!! */
		if (strcmp(iden,"mamebin")		== 0) strcpy(mamebin,value);

		if (strcmp(iden,"srcromsdir")	== 0) strcpy(srcbasedir,value);
		if (strcmp(iden,"srclistdir")	== 0) strcpy(srclistdir,value);

		if (strcmp(iden,"tmpdir")		== 0) strcpy(tmpdir,value);
		if (strcmp(iden,"romsdir")		== 0) strcpy(romsdir,value);
		if (strcmp(iden,"mamedir")		== 0) strcpy(mamedir,value);
		if (strcmp(iden,"neogeodir")	== 0) strcpy(neogeodir,value);
		if (strcmp(iden,"samplesdir")	== 0) strcpy(samplesdir,value);
		if (strcmp(iden,"artworkdir")	== 0) strcpy(artworkdir,value);
		if (strcmp(iden,"listdir")		== 0) strcpy(listdir,value);
		if (strcmp(iden,"imagesdir")	== 0) strcpy(imagesdir,value);
		if (strcmp(iden,"titlesdir")	== 0) strcpy(titlesdir,value);
	}

	
	return(OK);
}

/************************************************************/
/*															*/
/* Parsea los argumentos de la linea de comandos			*/
/*															*/
/************************************************************/
int ParseArgs(int argc, char **argv)
{
	int i, j;
	unsigned long multiplier=1024*1024;
	FILE *mamecdsrc;
	struct passwd *user;
	char aux[MAXCAD];

	/* CAMBIAR!!!!! */
	sprintf(mamebin,	"xmame");

	sprintf(srcbasedir, "/home/carlitos/roms");
	sprintf(srclistdir, "/home/carlitos/lista");	

	sprintf(basedir,	".");
	sprintf(cddir,		".");
	sprintf(tmpdir,		"tmp");
	sprintf(romsdir,	"roms");
	sprintf(mamedir,	"mame");
	sprintf(neogeodir,	"neogeo");
	sprintf(samplesdir,	"samples");
	sprintf(artworkdir,	"artwork");
	sprintf(listdir,	"lista");
	sprintf(imagesdir,	"images");
	sprintf(titlesdir,	"titulos");

	generatecds=1;
	generatelist=1;
	generatefulllist=0;

	/* Primero miramos si existe ~/.mamecdsrc */
	user=getpwuid(getuid());
	sprintf(aux,"%s/.mamecdsrc",user->pw_dir);

	mamecdsrc=fopen(aux,"r");
	if (mamecdsrc!=NULL) 
	{
		if (Parserc(mamecdsrc)==ERROR) 
		{
			fclose(mamecdsrc);
			return ERROR;
		}
		fclose(mamecdsrc);
	}
	else
	{
		printf("Generating %s\n",aux);
		/* Generamos el fichero con los valores por defecto */
		mamecdsrc=fopen(aux,"w");
		
		fprintf(mamecdsrc,"# Xmame binary\n");
		fprintf(mamecdsrc,"mamebin\txmame\n");
		fprintf(mamecdsrc,"# Base directory where the roms are\n");
		fprintf(mamecdsrc,"srcromsdir\t%s/roms\n",user->pw_dir);
		fprintf(mamecdsrc,"# Base directory where the images and titles images are\n");
		fprintf(mamecdsrc,"srclistdir\t%s/list\n",user->pw_dir);
		fprintf(mamecdsrc,"# Temporary directory\n");
		fprintf(mamecdsrc,"tmpdir\ttmp\n");
		fprintf(mamecdsrc,"# Subdirectory where the roms are (and there be)\n");
		fprintf(mamecdsrc,"romsdir\troms\n");
		fprintf(mamecdsrc,"# Subdirectory where the mame-only roms are (and there be)\n");
		fprintf(mamecdsrc,"mamedir\tmame\n");
		fprintf(mamecdsrc,"# Subdirectory where the neogeo-only roms are (and there be)\n");
		fprintf(mamecdsrc,"neogeodir\tneogeo\n");
		fprintf(mamecdsrc,"# Subdirectory where the samples roms are (and there be)\n");
		fprintf(mamecdsrc,"samplesdir\tsamples\n");
		fprintf(mamecdsrc,"# Subdirectory where the artwork roms are (and there be)\n");
		fprintf(mamecdsrc,"artworkdir\tartwork\n");
		fprintf(mamecdsrc,"# Subdirectory where the list there be\n");
		fprintf(mamecdsrc,"listdir\tlist\n");
		fprintf(mamecdsrc,"# Subdirectory where the game images are (and there be)\n");
		fprintf(mamecdsrc,"imegesdir\timages\n");
		fprintf(mamecdsrc,"# Subdirectory where the title images roms are (and there be)\n");
		fprintf(mamecdsrc,"titlesdir\ttitles\n");
		
		fclose(mamecdsrc);
	}

	if (argc>1)
	{
		for (i=1; i<argc;i++)
		{
			if ( (strcmp(argv[i],"--help")==0) || (strcmp(argv[i],"-h")==0) )
			{
				return(Ayuda(argv[0]));
			}

			if ( (strcmp(argv[i],"--version")==0) || (strcmp(argv[i],"-v")==0) )
			{
				printf("%s version 1.0\n", argv[0]);
				return(ERROR);
			}

			if ( (strcmp(argv[i],"--neogeo")==0) || (strcmp(argv[i],"-n")==0) )
			{
				neogeocd=1;
			}

			if ( (strcmp(argv[i],"--mame")==0) || (strcmp(argv[i],"-m")==0) )
			{
				mamecd=1;
			}

			if ( (strcmp(argv[i],"--nocd")==0) || (strcmp(argv[i],"-c")==0) )
			{
				generatecds=0;
			}

			if ( (strcmp(argv[i],"--nolist")==0) || (strcmp(argv[i],"-l")==0) )
			{
				generatelist=0;
			}

			if ( (strcmp(argv[i],"--fulllist")==0) || (strcmp(argv[i],"-f")==0) )
			{
				generatefulllist=1;
			}

			if ( (strcmp(argv[i],"--size")==0) || (strcmp(argv[i],"-s")==0) )
			{
				i++;
				if (i==argc)
				{
					printf("Unreconigzed size: 0\n");
					return(ERROR);
				}
				
				if (isdigit(argv[i][0]))
				{
					tamanyocd=atol(argv[i]);
				}
				else
					{
						printf("Unreconigzed size: %s\n",argv[i]);
						return(ERROR);
					}

				j=0;
				while (argv[i][j]!='\0')
				{
					if (isalpha(argv[i][j]))
					{
						if (argv[i][j]=='B') multiplier=MULTIPLIERB;
						if (argv[i][j]=='K') multiplier=MULTIPLIERK;
						if (argv[i][j]=='M') multiplier=MULTIPLIERM;
						if (argv[i][j]=='m') multiplier=MULTIPLIERm;
						break;
					}
					else
						{
							j++;
						}
					
				}

				tamanyocd*=multiplier;
			}
		}
	}
 	return(OK);
}

/************************************************************/
/*															*/
/* Programa principal										*/
/*															*/
/************************************************************/
int main(int argc, char **argv)
{
	struct mamegame *lista;

	tamanyocd=74*MULTIPLIERm;
	/*tamanyocd=10*MULTIPLIERM;*/
	neogeocd=0;
	mamecd=0;

	lista=NULL;

	/* Parseamos los argumentos de la linea de comandos */
	if (ParseArgs(argc, argv)==ERROR) return(ERROR);

	/* Si no decimos nada hay que hacer todos los CDs */
	if (!neogeocd && !mamecd)
	{
		neogeocd=1;
		mamecd=1;
	}

	/* Imprimimos un poco de informacion */	
	printf("\n");
	printf("Make NEOGEO CD's ");
	if (neogeocd) printf("yes\n");
		else printf("no\n");
	
	printf("Make MAME CD's   ");
	if (mamecd) printf("yes\n");
		else printf("no\n");

	if (!generatecds) printf("Don't generate ISO CDs\n");
	if (!generatelist) printf("Don't generate game list\n");
	if (generatefulllist) printf("Create full game list\n");

	printf("Size of CD's     %lu bytes (%lu Kb, %lu Mb, %lu')\n\n", tamanyocd/MULTIPLIERB, tamanyocd/MULTIPLIERK, tamanyocd/MULTIPLIERM, tamanyocd/MULTIPLIERm);

	/* Creamos la lista de juegos */
	lista=MakeGameList(mamebin);
	printf("\n");

	/* Creamos los CD's que toquen */
	if (mamecd)
		CreateCDS(lista, MAME);

	if (neogeocd)
		CreateCDS(lista, NEOGEO);

	if (generatefulllist) 
	{
		printf("Making full list      (%4i games)\n", cuenta_juegos(lista,FULL,0));
		MakeHTML(lista,FULL,0);
	}

	printf("\n");

	return(0);
}
