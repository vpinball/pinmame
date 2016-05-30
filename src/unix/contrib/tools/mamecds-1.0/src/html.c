#include <stdio.h>
#include "utils.h"
#include "html.h"
#include "gamelist.h"
#include "mamecds.h"

char version_linux[]="Linux version 2.4.3";

/************************************************************/
/* 															*/
/* Generacion de la lista de juegos del CD en cuestion		*/
/* en html...												*/
/* 															*/
/************************************************************/
int escribe_individual(struct mamegame *juego, int tipo, int ncd)
{   
	FILE *fd;
	char individual[MAXCAD];
	char directorio[MAXCAD];
	float kbytes;

	kbytes=(float)juego->tamanyorom/1024.0;

	if (tipo==NEOGEO)
	{
		sprintf(directorio,"../%s/%s",romsdir,neogeodir);
		sprintf(individual,"%s/neogeo-%02i/%s/%s.html",tmpdir,ncd,listdir,juego->base);
	}
	if (tipo==MAME)
	{
		sprintf(directorio,"../%s/%s",romsdir,mamedir);
		sprintf(individual,"%s/mame-%02i/%s/%s.html",tmpdir,ncd,listdir,juego->base);
	}

	fd=fopen(individual,"w");

	fprintf(fd,"<HTML>\n");
	fprintf(fd,"<HEAD>\n");
	fprintf(fd,"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n");
	fprintf(fd,"<META NAME=\"Author\" CONTENT=\"Monigot\">   <META NAME=\"GENERATOR\" CONTENT=\"carlitos (%s)\">\n",version_linux);
	if (tipo==NEOGEO)
		fprintf(fd,"<TITLE>Lista de juegos (%s) NeogeoCD %02i</TITLE>\n", versionmame, ncd);
	if (tipo==MAME)
		fprintf(fd,"<TITLE>Lista de juegos (%s) MameCD %02i</TITLE>\n", versionmame, ncd);
	fprintf(fd,"</HEAD>\n");
	fprintf(fd,"<BODY TEXT=\"#FFFFFF\" BGCOLOR=\"#000000\" LINK=\"#33CCFF\" VLINK=\"#CC66CC\" ALINK=\"#FF6666\">\n");
	fprintf(fd,"<P>\n");
	fprintf(fd,"<CENTER>\n");
	if (tipo==MAME)
	{
		/*fprintf(fd,"<IMG SRC=\"%s/mame.jpg\" HEIGHT=297 WIDTH=600></CENTER>\n",imagesdir);*/
		fprintf(fd,"<IMG SRC=\"%s/mame.jpg\" HEIGHT=148 WIDTH=300></CENTER>\n",imagesdir);
	}
	if (tipo==NEOGEO)
	{
		/*fprintf(fd,"<IMG SRC=\"%s/neogeo.jpg\" HEIGHT=224 WIDTH=304></CENTER>\n", imagesdir);*/
		fprintf(fd,"<IMG SRC=\"%s/neogeo.jpg\" HEIGHT=112 WIDTH=152></CENTER>\n", imagesdir);
	}
	fprintf(fd,"<P>\n");

	fprintf(fd,"<CENTER><P>\n");
#ifdef ANCHO_IMAGEN
	fprintf(fd,"<TABLE border=\"5\" width=\"%i\">\n",ANCHO_IMAGEN);
#else
	fprintf(fd,"<TABLE border=\"5\">\n");
#endif
	fprintf(fd,"<COLGROUP><COL ALIGN=\"center\">\n");

	fprintf(fd,"<TR><TH rowspan=\"2\"><TH colspan=\"2\"><CENTER><B>%s</B><P><FONT COLOR=\"#00ff00\"><A HREF=\"%s/%s.zip\">%s</A></FONT>",juego->nombre, directorio, juego->base, juego->base);
	fprintf(fd,"&nbsp &nbsp<FONT COLOR=\"#ff0000\">%.2f Kbytes</FONT></CENTER>\n",kbytes);

#ifdef ANCHO_IMAGEN
	if (juego->hastitle) fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",titlesdir, juego->base, ANCHO_IMAGEN);
		else fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/notitle.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",titlesdir, ANCHO_IMAGEN);
	if (juego->hasimage) fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",imagesdir, juego->base, ANCHO_IMAGEN);
		else fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/noimage.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",imagesdir, ANCHO_IMAGEN);
#else
	if (juego->hastitle) fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\"> </CENTER>\n",titlesdir, juego->base);
		else fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/notitle.gif\" ALIGN=\"center\"> </CENTER>\n",titlesdir);
	if (juego->hasimage) fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\"> </CENTER>\n",imagesdir, juego->base);
		else fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/noimage.gif\" ALIGN=\"center\"> </CENTER>\n",imagesdir);
#endif

	fprintf(fd,"</TABLE><P>\n");

	fprintf(fd,"</CENTER><P>\n");
	fprintf(fd,"</HTML><P>\n");

	fclose(fd);

	return(0);
}

int MakeAll (struct mamegame *lista, int tipo, int ncd)
{
	FILE *fdall;
	struct mamegame *juego;
	
	char allfilename[MAXCAD];
	char pathlista[MAXCAD];
	char pathroms[MAXCAD];
	char pathsamples[MAXCAD];
	char pathartwork[MAXCAD];

	float kbytes;
	char color1[]="#000070";
	char color2[]="#0000A0";
	char color[]="#000000";
	int flag=0;

	if (!generatelist) return(OK);

	if (tipo==NEOGEO)
	{
		sprintf(pathlista,"%s/neogeo-%02i/%s", tmpdir, ncd, listdir);
		sprintf(pathroms,"../%s/%s",romsdir,neogeodir);
		sprintf(pathsamples,".");
		sprintf(pathartwork,".");
	}

	if (tipo==MAME)
	{
		sprintf(pathlista,"%s/mame-%02i/%s", tmpdir, ncd, listdir);
		sprintf(pathroms,"../%s/%s",romsdir,mamedir);
		sprintf(pathsamples,"../%s/%s",romsdir,samplesdir);
		sprintf(pathartwork,"../%s/%s",romsdir,artworkdir);
	}

	sprintf(allfilename,"%s/all.html",pathlista);
	fdall=fopen(allfilename,"w");

	fprintf(fdall,"<HTML>\n");
	fprintf(fdall,"<HEAD>\n");
	fprintf(fdall,"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n");
	fprintf(fdall,"<META NAME=\"Author\" CONTENT=\"Monigot\">   <META NAME=\"GENERATOR\" CONTENT=\"carlitos (Linux 2.4.3)\">\n");
	if (tipo==NEOGEO)
		fprintf(fdall,"<TITLE>Lista de juegos (%s) NeogeoCD %02i (all CD games)</TITLE>\n", versionmame, ncd);
	if (tipo==MAME)
		fprintf(fdall,"<TITLE>Lista de juegos (%s) MameCD %02i (all CD games)</TITLE>\n", versionmame, ncd);
	fprintf(fdall,"</HEAD>\n");
	fprintf(fdall,"<BODY TEXT=\"#FFFFFF\" BGCOLOR=\"#000000\" LINK=\"#33CCFF\" VLINK=\"#CC66CC\" ALINK=\"#FF6666\">\n");
	fprintf(fdall,"<P>\n");
	fprintf(fdall,"<CENTER>\n");
	if (tipo==MAME)
		fprintf(fdall,"<IMG SRC=\"%s/mame.jpg\" HEIGHT=297 WIDTH=600></CENTER>\n",imagesdir);
	if (tipo==NEOGEO)
		fprintf(fdall,"<IMG SRC=\"%s/neogeo.jpg\" HEIGHT=224 WIDTH=304></CENTER>\n", imagesdir);
	fprintf(fdall,"<P>\n");
	fprintf(fdall,"<CENTER>\n");
	fprintf(fdall,"<TABLE>\n");
	fprintf(fdall,"<COLGROUP>\n");
	fprintf(fdall,"<COL><COL><COL ALIGN=\"right\">\n");
	fprintf(fdall,"<THEAD>\n");
	fprintf(fdall,"<TR><TH ALIGN=\"left\">Titles<TH>Download<TH>Size\n");
	fprintf(fdall,"</THEAD>\n");
	fprintf(fdall,"<TBODY>\n");

	juego=lista;
	while(juego!=NULL)
	{
		if ((juego->numerocd==ncd)&&(juego->tipo==tipo))
		{
			if (flag==0)
			{
				strcpy(color,color1);
				flag=1;
			}
			else
			{
				strcpy(color,color2);
				flag=0;
			}
			kbytes=(float)juego->tamanyorom/1024.0;
			fprintf(fdall,"<TR><TD bgcolor=\"%s\">",color);
			fprintf(fdall,"<A HREF=\"%s.html\"><IMG SRC=\"%s/limage.gif\" BORDER=0></A> ",juego->base, imagesdir);
			fprintf(fdall,"%s",juego->nombre);
			fprintf(fdall,"<TD bgcolor=\"%s\">",color);
			fprintf(fdall,"<A HREF=\"%s/%s.zip\">%s.zip</A></FONT>",pathroms,juego->base, juego->base);
			fprintf(fdall,"<TD ALIGN=\"right\" bgcolor=\"%s\">%.2f Kb\n",color,kbytes);

			escribe_individual(juego, tipo, ncd);
		}
		juego=juego->next;
	}

	fprintf(fdall,"</CENTER>\n");
	fprintf(fdall,"</TABLE>\n");
	fprintf(fdall,"</CENTER>\n");
	fprintf(fdall,"</HTML>\n");
	fclose(fdall);

	return(OK);
}

int escribe_index (struct mamegame *lista, int tipo, int ncd)
{
	FILE *fdindex;
	char indexfilename[MAXCAD];
	char pathlista[MAXCAD];
	int i;

	if (tipo==NEOGEO)
	{
		sprintf(pathlista,"%s/neogeo-%02i/%s", tmpdir, ncd, listdir);
	}

	if (tipo==MAME)
	{
		sprintf(pathlista,"%s/mame-%02i/%s", tmpdir, ncd, listdir);
	}

	if (tipo==FULL)
	{
		sprintf(pathlista,"%s", srclistdir);
	}

	sprintf(indexfilename,"%s/index.html",pathlista);
	fdindex=fopen(indexfilename,"w");

#ifdef DEBUG_HTML
	printf("Escribiendo %s\n",indexfilename);
#endif

	i=0;
	fprintf(fdindex,"<HTML>\n");
	fprintf(fdindex,"<HEAD>\n");
	fprintf(fdindex,"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n");
	fprintf(fdindex,"<META NAME=\"Author\" CONTENT=\"Monigot\">   <META NAME=\"GENERATOR\" CONTENT=\"carlitos (Linux 2.2.14)\">\n");
	if (tipo==NEOGEO)
		fprintf(fdindex,"<TITLE>Lista de juegos (%s) NeogeoCD %02i</TITLE>\n", versionmame, ncd);
	if ((tipo==MAME)||(tipo==FULL))
		fprintf(fdindex,"<TITLE>Lista de juegos (%s) MameCD %02i</TITLE>\n", versionmame, ncd);
	fprintf(fdindex,"</HEAD>\n");
	fprintf(fdindex,"<BODY TEXT=\"#FFFFFF\" BGCOLOR=\"#000000\" LINK=\"#33CCFF\" VLINK=\"#CC66CC\" ALINK=\"#FF6666\">\n");
	fprintf(fdindex,"<P>\n");
	fprintf(fdindex,"<CENTER>\n");
	if (tipo==NEOGEO)
		fprintf(fdindex,"<IMG SRC=\"%s/neogeo.jpg\" HEIGHT=224 WIDTH=304></CENTER>\n", imagesdir);
	if ((tipo==MAME)||(tipo==FULL))
		fprintf(fdindex,"<IMG SRC=\"%s/mame.jpg\" HEIGHT=297 WIDTH=600></CENTER>\n",imagesdir);
	fprintf(fdindex,"<P>\n");
	fprintf(fdindex,"<CENTER>\n");
	fprintf(fdindex,"<TABLE>\n");
	fprintf(fdindex,"<TABLE border=\"5\">\n");
	fprintf(fdindex,"<COLGROUP><COL ALIGN=\"center\">\n");
	fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER><FONT COLOR=\"#00ff00\" SIZE=+4>Indice de juegos </FONT></CENTER></FONT><TD><FONT SIZE=+2><CENTER><FONT COLOR=\"#00ff00\" SIZE=+4>Nº de juegos</FONT></CENTER></FONT><P>\n");
	if (njuegos_letra[GAME_NEOGEO]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"neogeo_1.html\"><FONT SIZE=+2><CENTER> NeoGeo</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_NEOGEO]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> NeoGeo</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_NEOGEO]);

	if (njuegos_letra[GAME_1]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"1_1.html\"><FONT SIZE=+2><CENTER> 0..9</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_1]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> 0..9</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_1]);

	if (njuegos_letra[GAME_A]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"a_1.html\"><FONT SIZE=+2><CENTER> A</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_A]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> A</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_A]);

	if (njuegos_letra[GAME_B]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"b_1.html\"><FONT SIZE=+2><CENTER> B</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_B]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> B</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_B]);

	if (njuegos_letra[GAME_C]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"c_1.html\"><FONT SIZE=+2><CENTER> C</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_C]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> C</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_C]);

	if (njuegos_letra[GAME_D]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"d_1.html\"><FONT SIZE=+2><CENTER> D</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_D]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> D</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_D]);

	if (njuegos_letra[GAME_E]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"e_1.html\"><FONT SIZE=+2><CENTER> E</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_E]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> E</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_E]);

	if (njuegos_letra[GAME_F]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"f_1.html\"><FONT SIZE=+2><CENTER> F</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_F]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> F</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_F]);

	if (njuegos_letra[GAME_G]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"g_1.html\"><FONT SIZE=+2><CENTER> G</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_G]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> G</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_G]);

	if (njuegos_letra[GAME_H]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"h_1.html\"><FONT SIZE=+2><CENTER> H</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_H]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> H</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_H]);

	if (njuegos_letra[GAME_I]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"i_1.html\"><FONT SIZE=+2><CENTER> I</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_I]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> I</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_I]);

	if (njuegos_letra[GAME_J]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"j_1.html\"><FONT SIZE=+2><CENTER> J</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_J]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> J</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_J]);

	if (njuegos_letra[GAME_K]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"k_1.html\"><FONT SIZE=+2><CENTER> K</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_K]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> K</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_K]);

	if (njuegos_letra[GAME_L]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"l_1.html\"><FONT SIZE=+2><CENTER> L</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_L]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> L</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_L]);

	if (njuegos_letra[GAME_M]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"m_1.html\"><FONT SIZE=+2><CENTER> M</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_M]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> M</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_M]);

	if (njuegos_letra[GAME_N]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"n_1.html\"><FONT SIZE=+2><CENTER> N</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_N]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> N</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_N]);

	if (njuegos_letra[GAME_O]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"o_1.html\"><FONT SIZE=+2><CENTER> O</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_O]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> O</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_O]);

	if (njuegos_letra[GAME_P]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"p_1.html\"><FONT SIZE=+2><CENTER> P</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_P]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> P</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_P]);

	if (njuegos_letra[GAME_Q]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"q_1.html\"><FONT SIZE=+2><CENTER> Q</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_Q]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> Q</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_Q]);

	if (njuegos_letra[GAME_R]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"r_1.html\"><FONT SIZE=+2><CENTER> R</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_R]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> R</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_R]);

	if (njuegos_letra[GAME_S]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"s_1.html\"><FONT SIZE=+2><CENTER> S</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_S]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> S</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_S]);

	if (njuegos_letra[GAME_T]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"t_1.html\"><FONT SIZE=+2><CENTER> T</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_T]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> T</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_T]);

	if (njuegos_letra[GAME_U]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"u_1.html\"><FONT SIZE=+2><CENTER> U</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_U]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> U</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_U]);

	if (njuegos_letra[GAME_V]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"v_1.html\"><FONT SIZE=+2><CENTER> V</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_V]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> V</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_V]);

	if (njuegos_letra[GAME_W]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"w_1.html\"><FONT SIZE=+2><CENTER> W</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_W]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> W</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_W]);

	if (njuegos_letra[GAME_X]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"x_1.html\"><FONT SIZE=+2><CENTER> X</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_X]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> X</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_X]);

	if (njuegos_letra[GAME_Y]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"y_1.html\"><FONT SIZE=+2><CENTER> Y</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_Y]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> Y</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_Y]);

	if (njuegos_letra[GAME_Z]!=0)
		fprintf(fdindex,"<TR><TD><A HREF=\"z_1.html\"><FONT SIZE=+2><CENTER> Z</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_Z]);
	else
		fprintf(fdindex,"<TR><TD><FONT SIZE=+2><CENTER> Z</CENTER></FONT><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_letra[GAME_Z]);


	/*fprintf(fdindex,"<TR><TD><A HREF=\"all.html\"><FONT SIZE=+2><CENTER> All</CENTER></FONT></A><P>\n<TD><FONT SIZE=+2><CENTER> %i</CENTER></FONT>\n",njuegos_neogeo + njuegos_mame);*/
	fprintf(fdindex,"<TR><TD><CENTER><FONT SIZE=+3 COLOR=\"ff0000\"> Total</FONT><CENTER>\n<TD><CENTER><FONT SIZE=+3 COLOR=\"ff0000\"> %i</FONT><CENTER>\n", njuegos_neogeo + njuegos_mame);
	fprintf(fdindex,"</TABLE>\n");
	fprintf(fdindex,"</CENTER>\n");
	fprintf(fdindex,"<P>\n");

	fclose(fdindex);
	return(0);
}

int escribe_cabecera(FILE *fd, int tipo)
{
	fprintf(fd,"<HTML>\n");
	fprintf(fd,"<HEAD>\n");
	fprintf(fd,"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n");
	fprintf(fd,"<META NAME=\"Author\" CONTENT=\"Monigot\">   <META NAME=\"GENERATOR\" CONTENT=\"carlitos (Linux %s)\">\n",version_linux);
	fprintf(fd,"<TITLE>Lista de juegos (Mame %s)</TITLE>\n",versionmame);
	fprintf(fd,"</HEAD>\n");
	fprintf(fd,"<BODY TEXT=\"#FFFFFF\" BGCOLOR=\"#000000\" LINK=\"#33CCFF\" VLINK=\"#CC66CC\" ALINK=\"#FF6666\">\n");
	fprintf(fd,"<P>\n");
	fprintf(fd,"<CENTER>\n");
	if (tipo==MAME)
		fprintf(fd,"<IMG SRC=\"%s/mame.jpg\" HEIGHT=148 WIDTH=300></CENTER>\n",imagesdir);
	else if (tipo==NEOGEO)
		fprintf(fd,"<IMG SRC=\"%s/neogeo.jpg\" HEIGHT=224 WIDTH=304></CENTER>\n",imagesdir);

	fprintf(fd,"<P>\n");

	return(0);
}

int escribe_flechas(FILE *fd, int letra, int pagina)
{
	int paginamas, paginamenos;
	float otra;
	char ltr[256];

	if (letra==GAME_NEOGEO) sprintf(ltr,"neogeo");
	else if (letra==GAME_1) sprintf(ltr,"1");
	else sprintf(ltr,"%c",letra+0x60);

	paginamas=pagina+1;
	paginamenos=pagina-1;

	fprintf(fd,"<CENTER>\n");

	/* ANTERIOR */
	if (pagina > 1)
	{
		fprintf(fd,"<A HREF=\"%s_%i.html\">",ltr, paginamenos);
		fprintf(fd,"<IMG SRC=\"%s/anterior.jpg\" BORDER=0> </A>",imagesdir);
	}
	else
	{
		/*fprintf(fd,"<IMG SRC=\"%s/anterior.jpg\"> ",imagesdir);*/
	}

	/* CENTRO */
	fprintf(fd,"<IMG SRC=\"%s/centro.jpg\" BORDER=0>",imagesdir);

	/* SIGUIENTE */
	if (letra==GAME_NEOGEO)
		otra=(float)njuegos_neogeo/(float)pagina;
	else
		otra=(float)njuegos_letra[letra]/(float)pagina;

	if (otra > JUEGOS_PAGINA)
	{
		fprintf(fd,"<A HREF=\"%s_%i.html\">",ltr, paginamas);
		fprintf(fd,"<IMG SRC=\"%s/siguiente.jpg\" BORDER=0></A>\n",imagesdir);
	}
	else
	{
		/*fprintf(fd,"<IMG SRC=\"%s/siguiente.jpg\" BORDER=0>\n",imagesdir);*/
	}

	fprintf(fd,"</CENTER>\n");
	return(0);
}

int escribe_barra(FILE *fd, int letra)
{

	fprintf(fd,"<CENTER>\n");
	fprintf(fd,"<B><H3>\n");
	if ((letra == GAME_NEOGEO ) || (njuegos_letra[GAME_NEOGEO]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">NeoGeo</FONT></B>\n"); } else { fprintf(fd,"<A HREF=\"neogeo_1.html\">NeoGeo</A>\n"); }
	if ((letra == GAME_1 ) || (njuegos_letra[GAME_1]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">0..9</FONT></B>\n"); }   else { fprintf(fd,"<A HREF=\"1_1.html\">0..9</A>\n"); }
	if ((letra == GAME_A ) || (njuegos_letra[GAME_A]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">A</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"a_1.html\">A</A>\n");}
	if ((letra == GAME_B ) || (njuegos_letra[GAME_B]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">B</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"b_1.html\">B</A>\n");}
	if ((letra == GAME_C ) || (njuegos_letra[GAME_C]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">C</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"c_1.html\">C</A>\n");}
	if ((letra == GAME_D ) || (njuegos_letra[GAME_D]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">D</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"d_1.html\">D</A>\n");}
	if ((letra == GAME_E ) || (njuegos_letra[GAME_E]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">E</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"e_1.html\">E</A>\n");}
	if ((letra == GAME_F ) || (njuegos_letra[GAME_F]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">F</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"f_1.html\">F</A>\n");}
	if ((letra == GAME_G ) || (njuegos_letra[GAME_G]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">G</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"g_1.html\">G</A>\n");}
	if ((letra == GAME_H ) || (njuegos_letra[GAME_H]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">H</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"h_1.html\">H</A>\n");}
	if ((letra == GAME_I ) || (njuegos_letra[GAME_I]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">I</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"i_1.html\">I</A>\n");}
	if ((letra == GAME_J ) || (njuegos_letra[GAME_J]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">J</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"j_1.html\">J</A>\n");}
	if ((letra == GAME_K ) || (njuegos_letra[GAME_K]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">K</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"k_1.html\">K</A>\n");}
	if ((letra == GAME_L ) || (njuegos_letra[GAME_L]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">L</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"l_1.html\">L</A>\n");}
	if ((letra == GAME_M ) || (njuegos_letra[GAME_M]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">M</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"m_1.html\">M</A>\n");}
	if ((letra == GAME_N ) || (njuegos_letra[GAME_N]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">N</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"n_1.html\">N</A>\n");}
	if ((letra == GAME_O ) || (njuegos_letra[GAME_O]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">O</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"o_1.html\">O</A>\n");}
	if ((letra == GAME_P ) || (njuegos_letra[GAME_P]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">P</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"p_1.html\">P</A>\n");}
	if ((letra == GAME_Q ) || (njuegos_letra[GAME_Q]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">Q</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"q_1.html\">Q</A>\n");}
	if ((letra == GAME_R ) || (njuegos_letra[GAME_R]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">R</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"r_1.html\">R</A>\n");}
	if ((letra == GAME_S ) || (njuegos_letra[GAME_S]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">S</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"s_1.html\">S</A>\n");}
	if ((letra == GAME_T ) || (njuegos_letra[GAME_T]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">T</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"t_1.html\">T</A>\n");}
	if ((letra == GAME_U ) || (njuegos_letra[GAME_U]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">U</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"u_1.html\">U</A>\n");}
	if ((letra == GAME_V ) || (njuegos_letra[GAME_V]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">V</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"v_1.html\">V</A>\n");}
	if ((letra == GAME_W ) || (njuegos_letra[GAME_W]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">W</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"w_1.html\">W</A>\n");}
	if ((letra == GAME_X ) || (njuegos_letra[GAME_X]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">X</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"x_1.html\">X</A>\n");}
	if ((letra == GAME_Y ) || (njuegos_letra[GAME_Y]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">Y</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"y_1.html\">Y</A>\n");}
	if ((letra == GAME_Z ) || (njuegos_letra[GAME_Z]==0)) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">Z</FONT></B>\n"); }  else {fprintf(fd,"<A HREF=\"z_1.html\">Z</A>\n");}
	/*if (letra == ALL ) { fprintf(fd,"<B><FONT COLOR=\"#FF0000\">All</FONT></B>\n"); } else {fprintf(fd,"<A HREF=\"all.html\">All</A>\n");}*/
	fprintf(fd,"</H3></B>\n");
	fprintf(fd,"</CENTER>\n");
	fprintf(fd,"<P>\n");

	return(0);
}

int escribe_juego (FILE *fd, int tipo, struct mamegame *juego)
{   
	char directorio[MAXCAD];
	char directoriosamples[MAXCAD];
	char directorioartwork[MAXCAD];
	float kbytes;
	float kbytessamples;
	float kbytesartwork;

	kbytes=(float)juego->tamanyorom/1024.0;
	if (juego->hassamples) kbytessamples=(float)juego->tamanyosamples/1024.0;
	if (juego->hasartwork) kbytesartwork=(float)juego->tamanyoartwork/1024.0;

#ifdef ANCHO_IMAGEN
	fprintf(fd,"<TABLE border=\"5\" width=\"%i\">\n",ANCHO_IMAGEN);
#else
	fprintf(fd,"<TABLE border=\"5\">\n");
#endif
	fprintf(fd,"<COLGROUP><COL ALIGN=\"center\">\n");
	if (tipo==GAME_NEOGEO)
		sprintf(directorio,"../%s/%s",romsdir,neogeodir);
	else
	{
		sprintf(directorio,"../%s/%s",romsdir,mamedir);
		sprintf(directoriosamples,"../%s/%s",romsdir,samplesdir);
		sprintf(directorioartwork,"../%s/%s",romsdir,artworkdir);
	}

	fprintf(fd,"<TR><TH rowspan=\"2\"><TH colspan=\"2\"><CENTER><B>%s</B><P><FONT COLOR=\"#00ff00\"><A HREF=\"%s/%s.zip\">%s.zip</A></FONT>",juego->nombre, directorio, juego->base, juego->base);
	fprintf(fd,"&nbsp &nbsp<FONT COLOR=\"#ff0000\">%5.2f Kbytes</FONT></CENTER>\n",kbytes);
	if (juego->hassamples) 
	{
		fprintf(fd,"<FONT COLOR=\"#00ff00\">samples: <A HREF=\"%s/%s.zip\">%s.zip</A></FONT>", directoriosamples, juego->base, juego->base);
		fprintf(fd,"&nbsp &nbsp<FONT COLOR=\"#ff0000\">%5.2f Kbytes</FONT></CENTER>\n",kbytessamples);
	}
	if (juego->hasartwork) 
	{
		fprintf(fd,"<FONT COLOR=\"#00ff00\">artwork: <A HREF=\"%s/%s.zip\">%s.zip</A></FONT>", directorioartwork, juego->base, juego->base);
		fprintf(fd,"&nbsp &nbsp<FONT COLOR=\"#ff0000\">%5.2f Kbytes</FONT></CENTER>\n",kbytesartwork);
	}

#ifdef ANCHO_IMAGEN
	if (juego->hastitle) fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",titlesdir, juego->base, ANCHO_IMAGEN);
		else fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/notitle.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",titlesdir, ANCHO_IMAGEN);
	if (juego->hasimage) fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",imagesdir, juego->base, ANCHO_IMAGEN);
		else fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/noimage.gif\" ALIGN=\"center\" WIDTH=\"%i\"> </CENTER>\n",imagesdir, ANCHO_IMAGEN);
#else
	if (juego->hastitle) fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\"> </CENTER>\n",titlesdir, juego->base);
		else fprintf(fd,"<TR><TH><CENTER> <IMG SRC=\"%s/notitle.gif\" ALIGN=\"center\"> </CENTER>\n",titlesdir);
	if (juego->hasimage) fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/%s.gif\" ALIGN=\"center\"> </CENTER>\n",imagesdir, juego->base);
		else fprintf(fd,"    <TH><CENTER> <IMG SRC=\"%s/noimage.gif\" ALIGN=\"center\"> </CENTER>\n",imagesdir);
#endif
		
	fprintf(fd,"</TABLE><P>\n");

	return(0);
}   

int escribe_mame(struct mamegame *lista, int tipo, int ncd)
{
	FILE *fd;
	int pagina, contador;
	char nombre[256];
	char ltr;
	int letra_ok;
	int letra;
	struct mamegame *juego;

	for (letra=GAME_1;letra<=GAME_Z;letra++)
	{
		if (letra==GAME_1) ltr='1';
		else ltr=letra+0x60;

		pagina=1;
		juego=lista;
		/*printf("\tWriting \"%c\" \t(%i)\n",ltr,njuegos_letra[letra]);*/
		while(juego!=NULL)
		{
			/* Abrimos el fichero */
			if (tipo==FULL) sprintf(nombre,"%s/%c_%i.html", srclistdir, (char)ltr, pagina);
			else sprintf(nombre,"%s/mame-%02i/%s/%c_%i.html", tmpdir, ncd, listdir, (char)ltr, pagina);
			fd=fopen(nombre,"w");

			escribe_cabecera(fd, MAME);
			escribe_barra(fd, letra);
			escribe_flechas(fd, letra, pagina);
			fprintf(fd,"<CENTER>\n");

			contador=0;
			while ((contador<JUEGOS_PAGINA)&&(juego!=NULL))
			{
				if (((juego->tipo==MAME) || (juego->tipo==FULL)) && (juego->numerocd==ncd))
				{
					letra_ok=0;
					/* Empieza por la letra adecuada? */
					if (letra==GAME_1)
					{
						if ((juego->nombre[0]=='0')|| (juego->nombre[0]=='1')|| (juego->nombre[0]=='2')|| (juego->nombre[0]=='3')||
								(juego->nombre[0]=='4')|| (juego->nombre[0]=='5')|| (juego->nombre[0]=='6')|| (juego->nombre[0]=='7')||
								(juego->nombre[0]=='8')|| (juego->nombre[0]=='9')|| (juego->nombre[0]=='\''))
							letra_ok=1;
					}
					if ((letra==GAME_A)&&(juego->nombre[0]=='A')) letra_ok=1;
					if ((letra==GAME_B)&&(juego->nombre[0]=='B')) letra_ok=1;
					if ((letra==GAME_C)&&(juego->nombre[0]=='C')) letra_ok=1;
					if ((letra==GAME_D)&&(juego->nombre[0]=='D')) letra_ok=1;
					if ((letra==GAME_E)&&(juego->nombre[0]=='E')) letra_ok=1;
					if ((letra==GAME_F)&&(juego->nombre[0]=='F')) letra_ok=1;
					if ((letra==GAME_G)&&(juego->nombre[0]=='G')) letra_ok=1;
					if ((letra==GAME_H)&&(juego->nombre[0]=='H')) letra_ok=1;
					if ((letra==GAME_I)&&(juego->nombre[0]=='I')) letra_ok=1;
					if ((letra==GAME_J)&&(juego->nombre[0]=='J')) letra_ok=1;
					if ((letra==GAME_K)&&(juego->nombre[0]=='K')) letra_ok=1;
					if ((letra==GAME_L)&&(juego->nombre[0]=='L')) letra_ok=1;
					if ((letra==GAME_M)&&(juego->nombre[0]=='M')) letra_ok=1;
					if ((letra==GAME_N)&&(juego->nombre[0]=='N')) letra_ok=1;
					if ((letra==GAME_O)&&(juego->nombre[0]=='O')) letra_ok=1;
					if ((letra==GAME_P)&&(juego->nombre[0]=='P')) letra_ok=1;
					if ((letra==GAME_Q)&&(juego->nombre[0]=='Q')) letra_ok=1;
					if ((letra==GAME_R)&&(juego->nombre[0]=='R')) letra_ok=1;
					if ((letra==GAME_S)&&(juego->nombre[0]=='S')) letra_ok=1;
					if ((letra==GAME_T)&&(juego->nombre[0]=='T')) letra_ok=1;
					if ((letra==GAME_U)&&(juego->nombre[0]=='U')) letra_ok=1;
					if ((letra==GAME_V)&&(juego->nombre[0]=='V')) letra_ok=1;
					if ((letra==GAME_W)&&(juego->nombre[0]=='W')) letra_ok=1;
					if ((letra==GAME_X)&&(juego->nombre[0]=='X')) letra_ok=1;
					if ((letra==GAME_Y)&&(juego->nombre[0]=='Y')) letra_ok=1;
					if ((letra==GAME_Z)&&(juego->nombre[0]=='Z')) letra_ok=1;

					if(letra_ok==1)
					{
						escribe_juego(fd, letra, juego);
						contador++;
					}
				}
				juego=juego->next;
			}

			escribe_flechas(fd, letra, pagina);
			escribe_barra(fd, letra);
			fclose(fd);

			pagina++;
		}
	}

	return(0);
}


int escribe_neogeo (struct mamegame *lista, int tipo, int ncd)
{
	FILE *fd;
	int pagina, contador;
	char nombre[MAXCAD];
	struct mamegame *juego;

	/*printf("\tWriting \"neogeo\" (%i)\n", njuegos_neogeo);*/

	pagina=1;
	juego=lista;
	while (juego!=NULL)
	{
		/* Abrimos el fichero */
		if (tipo==FULL) sprintf(nombre,"%s/neogeo_%i.html", srclistdir, pagina);
		else sprintf(nombre,"%s/neogeo-%02i/%s/neogeo_%i.html", tmpdir, ncd, listdir, pagina);
		fd=fopen(nombre,"w");

		escribe_cabecera(fd, NEOGEO);
		escribe_barra(fd, GAME_NEOGEO);
		escribe_flechas(fd, GAME_NEOGEO, pagina);

		fprintf(fd,"<CENTER>\n");

		contador=0;
		while ((contador<JUEGOS_PAGINA)&&(juego!=NULL))
		{
			if (((juego->tipo==NEOGEO) || (juego->tipo==FULL)) && (juego->numerocd==ncd))
			{
				escribe_juego(fd, GAME_NEOGEO, juego);
				contador++;
			}
			juego=juego->next;
		}
		/*printf("(%i)\n",contador);*/

		escribe_flechas(fd, GAME_NEOGEO, pagina);
		escribe_barra(fd,GAME_NEOGEO);
		fclose(fd);

		pagina++;
	}
	
	return(0);
}


int MakeHTML (struct mamegame *lista, int tipo, int ncd)
{
	
	if (generatelist)
	{
		if (tipo==FULL) escribe_index(lista,FULL,0);
		else escribe_index(lista,tipo,ncd);
		/*MakeAll(lista,tipo,ncd);*/

		if (tipo==NEOGEO)
		{
			escribe_neogeo(lista,tipo,ncd);
		}

		if (tipo==MAME)
		{
			escribe_mame(lista,tipo,ncd);
		}

		if (tipo==FULL)
		{
			escribe_neogeo(lista,FULL,0);
			escribe_mame(lista,FULL,0);
		}
	}

	return(0);
}

