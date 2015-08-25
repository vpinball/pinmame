#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "utils.h"
#include "gamelist.h"
#include "mamecds.h"
#include "html.h"

int MakeIso (void)
{
	char command[MAXCAD];
	char srcdir[MAXCAD];
	char image[MAXCAD];

	if (!generatecds) return(OK);

	sprintf(srcdir,"%s/%s/%s/",basedir,tmpdir,cddir);
	sprintf(image,"%s/%s/%s.iso",basedir,tmpdir,cddir);

	sprintf(command,"mkisofs -r -R -J -o %s %s",image,srcdir);
	printf("Generating %s\n", image);

	system(command);

	return(OK);
}


int CreateLinks (struct mamegame *juego, int tipo, int ncd)
{
	char src[MAXCAD];
	char dst[MAXCAD];

	sprintf(dstromsdir,"%s/%s/%s/%s", basedir, tmpdir, cddir, romsdir);
	sprintf(dstlistdir,"%s/%s/%s/%s", basedir, tmpdir, cddir, listdir);

	/* Link de la rom */
	if (tipo==NEOGEO)
	{
		sprintf(src, "%s/%s/%s.zip", srcbasedir, neogeodir, juego->base);
		sprintf(dst, "%s/%s/%s.zip", dstromsdir, neogeodir, juego->base);
		if (link(src,dst)==-1)
		{
			printf("Can't link %s %s\n", src, dst);	
		}
	}
	else if (tipo==MAME)
	{
		if (juego->hasgame)
		{
			sprintf(src, "%s/%s/%s.zip", srcbasedir, mamedir, juego->base);
			sprintf(dst, "%s/%s/%s.zip", dstromsdir, mamedir, juego->base);
			if (link(src,dst)==-1)
			{
				printf("Can't link %s %s\n", src, dst);	
			}
		}

		/* Link de los samples */
		if (juego->hassamples)
		{
			sprintf(src,"%s/%s/%s.zip", srcbasedir, samplesdir, juego->base);
			sprintf(dst,"%s/%s/%s.zip", dstromsdir, samplesdir, juego->base);
			if (link(src,dst)==-1)
			{
				printf("Can't link %s %s\n", src, dst);	
			}
		}

		/* Link del artwork */
		if (juego->hasartwork)
		{
			sprintf(src,"%s/%s/%s.zip", srcbasedir, artworkdir, juego->base);
			sprintf(dst,"%s/%s/%s.zip", dstromsdir, artworkdir, juego->base);
			if (link(src,dst)==-1)
			{
				printf("Can't link %s %s\n", src, dst);	
			}
		}

	}

	if (generatelist)
	{
		/* Link del titulo */
		if (juego->hastitle)
		{
			sprintf(src,"%s/%s/%s.gif", srclistdir, titlesdir, juego->base);
			sprintf(dst,"%s/%s/%s.gif", dstlistdir, titlesdir, juego->base);
			if (link(src,dst)==-1)
			{
				printf("Can't link %s %s\n", src, dst);	
			}
		}

		/* Link de la imagen */
		if (juego->hasimage)
		{
			sprintf(src,"%s/%s/%s.gif", srclistdir, imagesdir, juego->base);
			sprintf(dst,"%s/%s/%s.gif", dstlistdir, imagesdir, juego->base);
			if (link(src,dst)==-1)
			{
				printf("Can't link %s %s\n", src, dst);	
			}
		}
	}

	return(OK);
}

int InsertGame (struct mamegame *lista, struct mamegame *juego, int tipo, int ncd, int tamanyoparcial)
{
	int tamanyo, i;
	struct mamegame *clon;

#ifdef DEBUG_CD
	printf("\tInserting %8s in CD %02i (%i %i)\n",juego->base, ncd, juego->tamanyo, tamanyoparcial);
#endif

	/* ¿cabe el juego y los clones? (imagenes, html, artwork y samples estan incluidos en el tamaño) */
	tamanyo=juego->tamanyo;
	juego->numerocd=ncd;	

	i=0;
	if (juego->hasclones)
	{
		while (strcmp(juego->clones[i],"")!=0)
		{
			clon=lista;
			while (clon!=NULL)
			{
				if (strcmp(clon->base, juego->clones[i])==0)
				{
#ifdef DEBUG_CD
					printf("\t    Clone %8s in CD %02i (%i %i)\n",clon->base, ncd, clon->tamanyo, tamanyoparcial);
#endif
					/* Clon encontrado */
					tamanyo+=clon->tamanyo;
					clon->numerocd=ncd;	
					i++;
				}
				else
				{
					clon=clon->next;
				}
			}
		}
	}

	if ((tamanyo+tamanyoparcial) > tamanyocd) 
	{
#ifdef DEBUG_CD
		printf("\tRemoving  %8s in CD %02i (%i %i)\n",juego->base, ncd, juego->tamanyo, tamanyoparcial);
#endif
		juego->numerocd=0;	

		i=0;
		if (juego->hasclones)
		{
			while (strcmp(juego->clones[i],"")!=0)
			{
				clon=lista;
				while (clon!=NULL)
				{
					if (strcmp(clon->base, juego->clones[i])==0)
					{
						/* Clon encontrado */
						clon->numerocd=0;	
						i++;
					}
					else
					{
						clon=clon->next;
					}
				}
			}
		}
		return ERROR;
	}
	else
	{
		juego->numerocd=ncd;

		/* Creamos todos los links correspondientes */

		CreateLinks(juego,tipo,ncd);

		i=0;
		if (juego->hasclones)
		{
			while (strcmp(juego->clones[i],"")!=0)
			{
				clon=lista;
				while (clon!=NULL)
				{
					if (strcmp(clon->base, juego->clones[i])==0)
					{
						/* Clon encontrado */
						CreateLinks(clon,tipo,ncd);
						i++;
					}
					else
					{
						clon=clon->next;
					}
				}
			}
		}

		return(tamanyo);
	}
}

/* Retorna numoero de cd + 1 o LASTCD si ya no quedan mas juegos */ 
int MakeCD (struct mamegame *lista, int tipo, int ncd)
{
	char aux[MAXCAD];
	char src[MAXCAD];
	char basecd[MAXCAD];		/* Directorio base del CD 			*/
	char baseroms[MAXCAD];		/* Directorio base de las roms 		*/
	char basesamples[MAXCAD];	/* Directorio base de los samples 	*/
	char baseartwork[MAXCAD];	/* Directorio base del artwork	 	*/
	char baselista[MAXCAD];		/* Directorio base de la lista 	 	*/
	char basetitles[MAXCAD];	/* Directorio de titulos 			*/
	char baseimages[MAXCAD];	/* Directorio de imagenes 		 	*/

	char srcroms[MAXCAD];		/* Directorio con las roms 			*/
	char neogeozip[MAXCAD];		/* Path completo de la bios de neogeo */
	char cvszip[MAXCAD];		/* Path completo de la bios de cvs 	*/
	char playchzip[MAXCAD];		/* Path completo de la bios de playchoice */
	char decocasszip[MAXCAD];	/* Path completo de la bios de decocass */
	char srcimages[MAXCAD];		/* Directorio con las imagenes		*/
	char srctitles[MAXCAD];		/* Directorio con los titulos		*/
	char srcsamples[MAXCAD];	/* Directorio con los samples		*/
	char srcartwork[MAXCAD];	/* Directorio con el artwork		*/

	int tamanyo, tamanyototal;
	struct mamegame *actual;
	struct stat buff;

	/* Creamos los directorios */
	sprintf(basecd,"%s/%s/%s", basedir, tmpdir, cddir);
	mkdir(basecd,S_IRWXU|S_IRWXG|S_IRWXO);									/* ./tmp/cddir-ncd 						*/

	sprintf(baseroms,"%s/%s",basecd,romsdir);
	mkdir(baseroms,S_IRWXU|S_IRWXG|S_IRWXO);								/* ./tmp/cddir-ncd/roms					*/

	if (tipo==NEOGEO) 	
	{
		sprintf(baseroms,"%s/%s/%s",basecd,romsdir,neogeodir);
		mkdir(baseroms,S_IRWXU|S_IRWXG|S_IRWXO);							/* ./tmp/cddir-ncd/roms/neogeo			*/

		sprintf(srcroms,"%s/%s", srcbasedir, neogeodir);					/* /home/carlitos/roms/neogeo			*/
		sprintf(neogeozip,"%s/neogeo.zip", srcroms);
	}
	if (tipo==MAME) 	
	{
		sprintf(baseroms,"%s/%s/%s",basecd,romsdir,mamedir);				/* ./tmp/cddir-ncd/roms/mame			*/
		mkdir(baseroms,S_IRWXU|S_IRWXG|S_IRWXO);

		sprintf(srcroms,"%s/%s", srcbasedir, mamedir);						/* /home/carlitos/roms/mame				*/
		sprintf(cvszip,"%s/cvs.zip", srcroms);
		sprintf(playchzip,"%s/playch10.zip", srcroms);
		sprintf(decocasszip,"%s/decocass.zip", srcroms);

		/* Los juegos de MAME pueden tener samples y artwork */
		sprintf(basesamples,"%s/%s/%s",basecd,romsdir,samplesdir);
		mkdir(basesamples,S_IRWXU|S_IRWXG|S_IRWXO);							/* ./tmp/cddir-ncd/roms/samples			*/

		sprintf(baseartwork,"%s/%s/%s",basecd,romsdir,artworkdir);
		mkdir(baseartwork,S_IRWXU|S_IRWXG|S_IRWXO);							/* ./tmp/cddir-ncd/roms/artwork			*/

		sprintf(srcroms,"%s/%s", srcbasedir, mamedir);						/* /home/carlitos/roms/mame				*/
		sprintf(srcsamples,"%s/%s/%s", srcbasedir, romsdir, samplesdir);	/* /home/carlitos/roms/samples			*/
		sprintf(srcartwork,"%s/%s/%s", srcbasedir, romsdir, artworkdir);	/* /home/carlitos/roms/artwork			*/
	}

	if (generatelist)
	{
		sprintf(baselista,"%s/%s",basecd,listdir);
		mkdir(baselista,S_IRWXU|S_IRWXG|S_IRWXO);								/* ./tmp/cddir.ncd/lista				*/

		sprintf(basetitles,"%s/%s",baselista,titlesdir);
		mkdir(basetitles,S_IRWXU|S_IRWXG|S_IRWXO);								/* ./tmp/cddir.ncd/lista/titulos		*/

		sprintf(baseimages,"%s/%s",baselista,imagesdir);
		mkdir(baseimages,S_IRWXU|S_IRWXG|S_IRWXO);;								/* ./tmp/cddir.ncd/lista/images			*/


		sprintf(srcimages,"%s/%s", srclistdir, imagesdir);						/* /home/lista/images					*/
		sprintf(srctitles,"%s/%s", srclistdir, titlesdir);						/* /home/lista/titulos					*/
	}

	/* Reservamos 7 Mb para los ficheros html */
	tamanyototal=7*1024*1024;

	if (tipo==NEOGEO) 	
	{
		/* Si son de neogeo necesitamos la BIOS */	
		/* Creamos el link a neogeo.zip */
		sprintf(aux,"%s/neogeo.zip",baseroms);
		link(neogeozip,aux);
		stat(neogeozip,&buff);
		tamanyototal+=buff.st_size;

		if (generatelist)
		{
			sprintf(src,"%s/neogeo.jpg",srcimages);
			sprintf(aux,"%s/neogeo.jpg",baseimages);
			link(src,aux);
			stat(aux,&buff);
			tamanyototal+=buff.st_size;
		}
	}
	if (tipo==MAME) 	
	{
		/* Creamos el link a cvs.zip */
		sprintf(aux,"%s/cvs.zip",baseroms);
		link(cvszip,aux);
		stat(cvszip,&buff);
		tamanyototal+=buff.st_size;

		/* Creamos el link a playch10.zip */
		sprintf(aux,"%s/playch10.zip",baseroms);
		link(playchzip,aux);
		stat(playchzip,&buff);
		tamanyototal+=buff.st_size;

		/* Creamos el link a decocass.zip */
		sprintf(aux,"%s/decocass.zip",baseroms);
		link(decocasszip,aux);
		stat(decocasszip,&buff);
		tamanyototal+=buff.st_size;

		if (generatelist)
		{
			sprintf(src,"%s/mame.jpg",srcimages);
			sprintf(aux,"%s/mame.jpg",baseimages);
			link(src,aux);
			stat(aux,&buff);
			tamanyototal+=buff.st_size;
		}
	}

	if (generatelist)
	{
		sprintf(src,"%s/limage.gif",srcimages);
		sprintf(aux,"%s/limage.gif",baseimages);
		link(src,aux);
		stat(aux,&buff);
		tamanyototal+=buff.st_size;

		sprintf(src,"%s/anterior.jpg",srcimages);
		sprintf(aux,"%s/anterior.jpg",baseimages);
		link(src,aux);
		stat(aux,&buff);
		tamanyototal+=buff.st_size;

		sprintf(src,"%s/centro.jpg",srcimages);
		sprintf(aux,"%s/centro.jpg",baseimages);
		link(src,aux);
		stat(aux,&buff);
		tamanyototal+=buff.st_size;

		sprintf(src,"%s/siguiente.jpg",srcimages);
		sprintf(aux,"%s/siguiente.jpg",baseimages);
		link(src,aux);
		stat(aux,&buff);
		tamanyototal+=buff.st_size;

		sprintf(src,"%s/noimage.gif",srcimages);
		sprintf(aux,"%s/noimage.gif",baseimages);
		link(src,aux);
		stat(aux,&buff);
		tamanyototal+=buff.st_size;

		sprintf(src,"%s/notitle.gif",srctitles);
		sprintf(aux,"%s/notitle.gif",basetitles);
		link(src,aux);
		stat(aux,&buff);
		tamanyototal+=buff.st_size;
	}

	actual=lista;
	while (1)
	{
		/* ¿El juego pertenece a algun CD? */
		if (actual->numerocd==0)
		{
			/* ¿Es del tipo que le hemos dicho? */
			if (actual->tipo==tipo)
			{
				/* Si es un clonico se insertara con su original */
				if (!actual->isclone)
				{
					tamanyo=InsertGame(lista, actual, tipo, ncd, tamanyototal);
					if (tamanyo==ERROR)
					{
#ifdef DEBUG_CD
						printf("Size of CD %02i %i\n", ncd, tamanyototal);
#endif
						/* El juego no ha cabido en el CD */
						return ncd+1;
					}
					else
					{
						tamanyototal+=tamanyo;
					}
				}
			}
			else	
			{
#ifdef DEBUG_CD_WARNINGS
				printf("%8s (%i) is not of type %i\n",actual->base, actual->tipo, tipo);
#endif
			}
		}
		else
		{
#ifdef DEBUG_CD_WARNINGS
			if (!actual->isclone) printf("%8s is in CD %02i\n",actual->base, actual->numerocd);
#endif
		}

		if (actual->next==NULL) 
		{
#ifdef DEBUG_CD
			printf("Size of CD %02i %i\n", ncd, tamanyototal);
#endif
			return LASTCD;
		}
		actual=actual->next;
	}

	return ncd+1;
}

int CreateCDS (struct mamegame *lista, int tipo)
{
	int ncd;	/* CD number */
	int ret;

	/* Creamos el direcrotio temporal */
	mkdir(tmpdir,S_IRWXU|S_IRWXG|S_IRWXO);

	ncd=1;	
	ret=MORECDS;
	if (tipo==MAME)
	{
		while(ret!=LASTCD)
		{
			sprintf(cddir,"mame-%02i",ncd);
			ret=MakeCD(lista,tipo,ncd);
			printf("Making MAME\tCD %02i (%4i games)\n",ncd, cuenta_juegos(lista,tipo,ncd));
			MakeHTML(lista,tipo,ncd);
			MakeIso();
			ncd++;
		}
	}

	ncd=1;	
	ret=MORECDS;
	if (tipo==NEOGEO)
	{
		while(ret!=LASTCD)
		{
			sprintf(cddir,"neogeo-%02i",ncd);
			ret=MakeCD(lista,tipo,ncd);
			printf("Making NEOGEO\tCD %02i (%4i games)\n",ncd, cuenta_juegos(lista,tipo,ncd));
			MakeHTML(lista,tipo,ncd);
			MakeIso();
			ncd++;
		}
	}

	return 0;
}
