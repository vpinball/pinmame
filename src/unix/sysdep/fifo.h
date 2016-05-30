/* Generic fifo implemented through defines

   Copyright 2000 Hans de Goede
   
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
/* Changelog
Version 0.1, March 2000
-initial release (Hans de Goede)
*/
#ifndef __FIFO_H
#define __FIFO_H

#define FIFO(PREFIX,NAME,TYPE)						\
   FIFO_PROTOTYPES(PREFIX,NAME,TYPE)					\
   FIFO_STRUCT(PREFIX,NAME,TYPE)					\
   FIFO_CREATE(PREFIX,NAME,TYPE)					\
   FIFO_DESTROY(PREFIX,NAME,TYPE)					\
   FIFO_PUT(PREFIX,NAME,TYPE)						\
   FIFO_GET(PREFIX,NAME,TYPE)						\
   FIFO_PEEK(PREFIX,NAME,TYPE)						\
   FIFO_EMPTY(PREFIX,NAME,TYPE)						\
   FIFO_GET_FREESPACE(PREFIX,NAME,TYPE)					\
   FIFO_IN_USE(PREFIX,NAME,TYPE)

#define FIFO_PROTOTYPES(PREFIX,NAME,TYPE)				\
   struct NAME ## _fifo_struct;						\
   PREFIX struct NAME ## _fifo_struct * NAME ## _fifo_create(int size);	\
   PREFIX void NAME ## _fifo_destroy(struct NAME ## _fifo_struct	\
      *fifo);								\
   PREFIX int NAME ## _fifo_put(struct NAME ## _fifo_struct *fifo,	\
      TYPE data);							\
   PREFIX int NAME ## _fifo_get(struct NAME ## _fifo_struct *fifo,	\
      TYPE *data);							\
   PREFIX int NAME ## _fifo_peek(struct NAME ## _fifo_struct *fifo,	\
      TYPE *data);							\
   PREFIX void NAME ## _fifo_empty(struct NAME ## _fifo_struct *fifo);	\
   PREFIX int NAME ## _fifo_in_use(struct NAME ## _fifo_struct *fifo);	\
   PREFIX int NAME ## _fifo_get_freespace(struct NAME ## _fifo_struct 	\
      *fifo);

#define FIFO_STRUCT(PREFIX,NAME,TYPE)					\
   struct NAME ## _fifo_struct {					\
      int size;								\
      int head;								\
      int in_use;							\
      TYPE *buffer;							\
   };

#define FIFO_CREATE(PREFIX,NAME,TYPE)					\
   PREFIX struct NAME ## _fifo_struct * NAME ## _fifo_create(int size)	\
   {									\
      struct NAME ## _fifo_struct *fifo = NULL;				\
      									\
      /* allocate the fifo struct */					\
      if(!(fifo = calloc(1, sizeof(struct NAME ## _fifo_struct))))	\
      {									\
         fprintf(stderr,						\
            "error malloc failed for struct xxx_fifo_struct\n");	\
         return NULL;							\
      }									\
      									\
      /* allocate the buffer */						\
      if(!(fifo->buffer = calloc(size, sizeof(TYPE))))			\
      {									\
         fprintf(stderr,						\
            "error malloc failed for xxx_fifo buffer\n");		\
         NAME ## _fifo_destroy(fifo);					\
         return NULL;							\
      }									\
     									\
      fifo->size = size;						\
     									\
      return fifo;							\
   }

#define FIFO_DESTROY(PREFIX,NAME,TYPE)					\
   PREFIX void NAME ## _fifo_destroy(struct NAME ## _fifo_struct *fifo)	\
   {									\
      if(fifo->buffer)							\
         free(fifo->buffer);						\
      									\
      free(fifo);							\
   }
   
#define FIFO_PUT(PREFIX,NAME,TYPE)					\
   PREFIX int NAME ## _fifo_put(struct NAME ## _fifo_struct *fifo,	\
      TYPE data)							\
   {									\
      int tail;								\
      									\
      if(fifo->in_use == fifo->size)					\
         return -1;							\
      									\
      tail = (fifo->head + fifo->in_use) % fifo->size;			\
      fifo->buffer[tail] = data;					\
      fifo->in_use++;							\
      									\
      return 0;								\
   }

#define FIFO_GET(PREFIX,NAME,TYPE)					\
   PREFIX int NAME ## _fifo_get(struct NAME ## _fifo_struct *fifo,	\
      TYPE *data)							\
   {									\
      if(!fifo->in_use)							\
         return -1;							\
      									\
      *data = fifo->buffer[fifo->head];					\
      fifo->head = (fifo->head + 1) % fifo->size;			\
      fifo->in_use--;							\
      									\
      return 0;								\
   }

#define FIFO_PEEK(PREFIX,NAME,TYPE)					\
   PREFIX int NAME ## _fifo_peek(struct NAME ## _fifo_struct *fifo,	\
      TYPE *data)							\
   {									\
      if(!fifo->in_use)							\
         return -1;							\
      									\
      *data = fifo->buffer[fifo->head];					\
      									\
      return 0;								\
   }

#define FIFO_EMPTY(PREFIX,NAME,TYPE)					\
   PREFIX void NAME ## _fifo_empty(struct NAME ## _fifo_struct *fifo)	\
   {									\
      fifo->head = fifo->in_use = 0;					\
   }

#define FIFO_IN_USE(PREFIX,NAME,TYPE)					\
   PREFIX int NAME ## _fifo_in_use(struct NAME ## _fifo_struct *fifo)	\
   {									\
      return fifo->in_use;						\
   }

#define FIFO_GET_FREESPACE(PREFIX,NAME,TYPE)				\
   PREFIX int NAME ## _fifo_get_freespace(struct NAME ## _fifo_struct	\
      *fifo)								\
   {									\
      return fifo->size - fifo->in_use;					\
   }

#endif /* ifndef __FIFO_H */
