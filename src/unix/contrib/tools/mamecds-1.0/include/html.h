#ifndef __HTML_H__
#define __HTML_H__

#include "utils.h"
#include "gamelist.h"

#define JUEGOS_PAGINA	25
/*#define ANCHO_IMAGEN	304*/

int MakeHTML (struct mamegame *lista, int tipo, int ncd);

#endif
