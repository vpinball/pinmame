#include "xmame.h"
#include "devices.h"

struct rc_option joy_SDL_opts[] = {
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};


#ifdef SDL

#include <SDL.h>

void joy_SDL_init(void);
void joy_SDL_poll(void);

SDL_Joystick 	*joystick;

void joy_SDL_init (void)
{
   int i,j;
   int joy_n=0;    

   if(SDL_Init(SDL_INIT_JOYSTICK ) < 0) printf ("SDL: JoyStick init Error!! ");
   else printf("SDL: joystick interface initialization...\n");

   joy_n= SDL_NumJoysticks();
   printf("SDL: %d joysticks founds.\n", joy_n );

   for (i = 0; i < joy_n; i++)
   {
      printf("SDL: The names of the joysticks :  %s\n", SDL_JoystickName(i));
      joystick=SDL_JoystickOpen(i);      
      if ( joystick == NULL)   printf("SDL:  the joystick init FAIL!!\n");

      joy_data[i].num_buttons = SDL_JoystickNumButtons(joystick);
      joy_data[i].num_axis    = SDL_JoystickNumAxes(joystick);

      if (joy_data[i].num_buttons > JOY_BUTTONS)
         joy_data[i].num_buttons = JOY_BUTTONS;
      if (joy_data[i].num_axis > JOY_AXIS)
         joy_data[i].num_axis = JOY_AXIS;

      for (j=0; j<joy_data[i].num_axis; j++)
      {
         joy_data[i].axis[j].min = -32768;
         joy_data[i].axis[j].max =  32768;
      }
      joy_poll_func = joy_SDL_poll;
    }

    for (; i < JOY ; i++)
	joy_data[i].fd = -1;

}


void joy_SDL_poll (void)
{
    /* this function is merged to SDL.c */
}

#endif
