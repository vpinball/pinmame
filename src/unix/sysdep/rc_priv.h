/* A simple rcfile and commandline parsing mechanism

   Copyright 1999,2000 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __RC_PRIV_H
#define __RC_PRIV_H

#include "rc.h"
#include "begin_code.h"

struct rc_struct
{
   struct rc_option *option;
   int option_size;
   char **arg;
   int arg_size;
   int args_registered;
};

#include "end_code.h"
#endif /* ifndef __RC_PRIV_H */
