#ifdef xgl

#include "xmame.h"
#include "glmame.h"

GLubyte *read_JPEG_file(char *);

GLuint cablist;
int numtex;
GLuint *cabtex=NULL;
GLubyte **cabimg=NULL;

#ifndef NDEBUG
static int wirecabinet = 1;
#else
static int wirecabinet = 0;
#endif

struct CameraPan *cpan=NULL;
int numpans;
int pannum;
int inpan=0;

static int inscreen=0;
static int scrvert;
static int inlist=0;
static int ingeom=0;

static int inbegin=0;

extern GLdouble vx_cscr_p1,vy_cscr_p1,vz_cscr_p1,vx_cscr_p2,vy_cscr_p2,vz_cscr_p2,
  vx_cscr_p3,vy_cscr_p3,vz_cscr_p3,vx_cscr_p4,vy_cscr_p4,vz_cscr_p4;


/* Skip until we hit whitespace */

char *SkipToSpace(char *buf)
{
  while(*buf&&!(isspace(*buf)||*buf==',')) buf++;

  return buf;
}

/* Skip whitespace and commas */

char *SkipSpace(char *buf)
{
  while(*buf&&(isspace(*buf)||*buf==',')) buf++;

  return buf;
}

/* Parse a string for a 4-component vector */

char *ParseVec4(char *buf,GLdouble *x,GLdouble *y,GLdouble *z,GLdouble *a)
{
  *x=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *y=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *z=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *a=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

/* Parse a string for a 3-component vector */

char *ParseVec3(char *buf,GLdouble *x,GLdouble *y,GLdouble *z)
{
  *x=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *y=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *z=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

/* Parse a string for a 2-component vector */

char  *ParseVec2(char *buf,GLdouble *x,GLdouble *y)
{
  *x=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *y=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

char  *ParseArg(char *buf,GLdouble *a)
{
  *a=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

/* Null-terminate a string after the text is done */

void MakeString(char *buf)
{
  while(*buf&&!isspace(*buf)) buf++;

  *buf='\0';
}

/* Parse a camera pan */

void ParsePan(char *buf,PanType type)
{
  if(pannum==numpans) {
	printf("GLError (cab): too many camera pans specified\n");
	return;
  }

  cpan[pannum].type=type;
  buf=ParseVec3(buf,&cpan[pannum].lx,&cpan[pannum].ly,&cpan[pannum].lz);
  buf=ParseVec3(buf,&cpan[pannum].px,&cpan[pannum].py,&cpan[pannum].pz);
  buf=ParseVec3(buf,&cpan[pannum].nx,&cpan[pannum].ny,&cpan[pannum].nz);

  if(type==pan_moveto) cpan[pannum].frames=atoi(buf);

  pannum++;
}

/* Parse a line of the .cab file */

void ParseLine(char *buf)
{
  GLdouble x,y,z,a;
  int texnum;
  int xdim,ydim;

  buf=SkipSpace(buf);

  if(!*buf||*buf=='#'||*buf=='\n') return;

  if(!strncasecmp(buf,"startgeom",9)) {
        CHECK_GL_BEGINEND();

	if(ingeom) printf("GLError (cab): second call to startgeom\n");
	ingeom=1;
  }
  else if(!strncasecmp(buf,"numtex",6)) {
        CHECK_GL_BEGINEND();

	if(ingeom)
	  printf("GLError (cab):numtex must be called before beginning model geometry\n");
	else {
	  numtex=atoi(buf+7);

	  if(numtex) {
		cabtex=(GLuint *)malloc(numtex*sizeof(GLuint));
		cabimg=(GLubyte **)malloc(numtex*sizeof(GLubyte *));
	  }
	}
  }
  else if(!strncasecmp(buf,"loadtex",7)) {
        CHECK_GL_BEGINEND();

	if(ingeom)
	  printf("GLError (cab):loadtex calls cannot come after beginning model geometry\n");
	else {
	  if(!cabtex)
		printf("GLError (cab): Number of textures must be declared before texture loading\n");
	  else {
		buf=SkipToSpace(buf);
		buf=SkipSpace(buf);
		
		texnum=atoi(buf);
		
		if(texnum>=numtex)
		  printf("GLError (cab): Hightest possible texture number is %d\n",numtex-1);
		else {
		  buf=SkipToSpace(buf);
		  buf=SkipSpace(buf);
		  
		  xdim=atoi(buf);
		  
		  buf=SkipToSpace(buf);
		  buf=SkipSpace(buf);
		  
		  ydim=atoi(buf);
		  
		  buf=SkipToSpace(buf);
		  buf=SkipSpace(buf);
		  
		  MakeString(buf);
		  
		  #ifndef NDEBUG
		    printf("GLINFO (cab): Loading texture %d (%dx%d) from %s\n",
		  	   texnum,xdim,ydim,buf);
		  #endif
		  
		  if(!wirecabinet)
		  {
			  disp__glGenTextures(1,&(cabtex[texnum]));
			  disp__glBindTexture(GL_TEXTURE_2D,cabtex[texnum]);
		  }
		  
		  cabimg[texnum]=read_JPEG_file(buf);
		  if(!cabimg[texnum])
			printf("GLError (cab): Unable to read %s\n",buf);
		  
		  if(!wirecabinet)
		  	disp__glTexImage2D(GL_TEXTURE_2D,0,3,xdim,ydim,0,
					   GL_RGB,GL_UNSIGNED_BYTE, cabimg[texnum]);
		  
		  disp__glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		  disp__glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		  
		  disp__glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		  disp__glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
		}
	  }
	}
  }
  else if(!strncasecmp(buf,"camerapan",9)) {
	numpans=atoi(buf+9);

	cpan=(struct CameraPan *)malloc(numpans*sizeof(struct CameraPan));

	pannum=0;
	inpan=1;
  }
  else if(!strncasecmp(buf,"goto",4)) {
	if(!inpan) printf("GLError (cab): pan command outside of camerapan\n");
	else ParsePan(buf+4,pan_goto);
  }
  else if(!strncasecmp(buf,"moveto",6)) {
	if(!inpan) printf("GLError (cab): pan command outside of camerapan\n");
	else ParsePan(buf+6,pan_moveto);
  }
  else if(!strncasecmp(buf,"end",3)) {
	if(inbegin==1)
	{
		GL_END();
		inbegin=0;
	} 
	else if(inpan==1)
		inpan=0;
	else if(inscreen==1)
		inscreen=0;
	else
	{
	          printf("GLError (cab): end command without begin, screen or camerapan\n");
	}
  } else {
	if(!ingeom) 
	{
		printf("GLError (cab): A startgeom call is needed before specifying any geometry\n");
        } else if(!strncasecmp(buf,"pointsize",9)) 
	{
	      if(inbegin) 
	          printf("GLError (cab): pointsize command within begin/end\n");
	      else 
	      {
		  ParseArg(buf+10, &a);
		  disp__glPointSize(a);
	      }
        } else if(!strncasecmp(buf,"begin",5)) 
	{
	  if(inbegin!=0)
	  {
		printf("GLError (cab): begin is called within begin/end !\n");
	  }
	  if(!strncasecmp(buf+6,"points",6)) {
		CHECK_GL_BEGINEND();
		GL_BEGIN(GL_POINTS); 
		inbegin=1;
	  }
	  else if(!strncasecmp(buf+6,"polygon",7)) {
	  	if(wirecabinet) {
			disp__glPolygonMode(GL_BACK , GL_LINE);
			disp__glPolygonMode(GL_FRONT, GL_LINE);
		}
		GL_BEGIN(GL_POLYGON);
		inbegin=1;
	  }
	  else if(!strncasecmp(buf+6,"quads",5)) {
		GL_BEGIN(GL_QUADS);
		inbegin=1;
	  }
	  else if(!strncasecmp(buf+6,"quad_strip",10)) {
		GL_BEGIN(GL_QUAD_STRIP);
		inbegin=1;
	  }
	  else if(!strncasecmp(buf+6,"screen",6)) {
		inscreen=1;
		scrvert=1;
	  }
	  else printf("GLError (cab): Invalid object type -- %s",buf+6);
	}
	else if(!strncasecmp(buf,"color3",6)) {
	  ParseVec3(buf+7,&x,&y,&z);
	  disp__glColor3d(x,y,z);
	}
	else if(!strncasecmp(buf,"color4",6)) {
	  ParseVec4(buf+7,&x,&y,&z,&a);
	  disp__glColor4d(x,y,z,a);
	}
	else if(!strncasecmp(buf,"vertex",6)) {
	  if(inscreen) {
		switch(scrvert) {
		case 4:
		  ParseVec3(buf+7,&vx_cscr_p1,&vy_cscr_p1,&vz_cscr_p1);
		  break;
		case 3:
		  ParseVec3(buf+7,&vx_cscr_p2,&vy_cscr_p2,&vz_cscr_p2);
		  break;
		case 2:
		  ParseVec3(buf+7,&vx_cscr_p3,&vy_cscr_p3,&vz_cscr_p3);
		  break;
		case 1:
		  ParseVec3(buf+7,&vx_cscr_p4,&vy_cscr_p4,&vz_cscr_p4);
		  break;
		default:
		  printf("GLError (cab): Error: Too many vertices in screen definition\n");
		  break;
		}
		
		scrvert++;
	  }
	  else {
		ParseVec3(buf+7,&x,&y,&z);
		disp__glVertex3d(x,y,z);
	  }
	}
	else if(!strncasecmp(buf,"shading",7)) {
	  if(!strncasecmp(buf+8,"flat",4))
		disp__glShadeModel(GL_FLAT);
	  else if(!strncasecmp(buf+8,"smooth",6))
		disp__glShadeModel(GL_SMOOTH);
	  else printf("GLError (cab): Invalid shading model -- %s",buf+8);
	}
	else if(!strncasecmp(buf,"enable",6)) {
	  if(!strncasecmp(buf+7,"texture",7))
		disp__glEnable(GL_TEXTURE_2D);
	  else printf("GLError (cab): Invalid feature to enable -- %s",buf+7);
	}
	else if(!strncasecmp(buf,"disable",7)) {
	  if(!strncasecmp(buf+8,"texture",7))
		disp__glDisable(GL_TEXTURE_2D);
	  else printf("GLError (cab): Invalid feature to disable -- %s",buf+7);
	}
	else if(!strncasecmp(buf,"settex",6)) {
	  texnum=atoi(buf+7);
	  
	  if(texnum>=numtex)
		printf("GLError (cab): Hightest possible texture number is %d\n",numtex-1);
	  else if(!wirecabinet)
		disp__glBindTexture(GL_TEXTURE_2D,cabtex[texnum]);
	}
	else if(!strncasecmp(buf,"texcoord",8)) {
	  ParseVec2(buf+9,&x,&y);
	  disp__glTexCoord2d(x,y);
	}
	else printf("GLError (cab): Invalid command -- %s",buf);
  }
}

void InitCabGlobals()
{
  int i;

  disp__glDeleteLists(cablist, 1);
  cablist=0;

  if(cabtex!=0) 
  {
          disp__glDeleteTextures(numtex, cabtex);
	  free(cabtex);
  }
  cabtex=0;

  for(i=0; cabimg!=0 && i<numtex; i++)
  {
	    if(cabimg[i]!=0)
		{
			free(cabimg[i]);
			cabimg[i]=0;
		}
  }

  if(cabimg!=0) 
	free(cabimg);
  cabimg=0;

  if(cpan!=0) 
	free(cpan);
  cpan=0;

  numtex=0;
  inlist=0;
  ingeom=0;
  numpans=0;
  pannum=0;
  inpan=0;
  inscreen=0;
  scrvert=0;
}

/* Load the cabinet */

int LoadCabinet(char *cabname)
{
  FILE *cfp;
  char buf[256];

  InitCabGlobals();

  sprintf(buf,"%s/cab/%s/%s.cab",XMAMEROOT,cabname,cabname);

  if(!(cfp=fopen(buf,"r")))
	return 0;

  #ifndef NDEBUG
    printf("GLINFO: Loading Cabinet from %s\n",buf);
  #endif

  cablist=disp__glGenLists(1);

  disp__glNewList(cablist,GL_COMPILE);
  inlist=1;

  if(!fgets(buf,256,cfp)) {
	printf("GLError (cab): File is empty\n");
	return 0;
  }

  if(strncasecmp(buf,"cabv1.0",7) &&
     strncasecmp(buf,"cabv1.1",7)
    ) 
  {
	printf("GLError (cab): File is not a v1.0, or v1.1 cabinet file -- cannot load\n");
	return 0;
  }

  while(fgets(buf,256,cfp)) {
	ParseLine(buf);
  }

  if(wirecabinet) {
	disp__glPolygonMode(GL_BACK , GL_FILL);
	disp__glPolygonMode(GL_FRONT, GL_FILL);
  }

  disp__glEndList();
  inlist=0;

  fclose(cfp);

  return(1);
}

#endif
