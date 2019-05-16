// linked list functions
// for preloading mp3 files in mpfserver
// using struct

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "linked_list.h"


/* Given a reference (pointer to pointer) to the head
  of a list and  a filename together with the Music pointer
  push a new node on the front
  of the list. */
void push_sound_element(struct Node_sound_element** head_ref, char *new_filename_with_path, Mix_Music *new_music)
{
    /* allocate node */
    struct Node_sound_element* new_node =
            (struct Node_sound_element*) malloc(sizeof(struct Node_sound_element));
 
    /* put in the filename  */
    strcpy (new_node->filename_with_path, new_filename_with_path);

    /* put in the cwmusic pointerfilename  */
    new_node->music = new_music;
 
    /* link the old list off the new node */
    new_node->next = (*head_ref);
 
    /* move the head to point to the new node */
    (*head_ref)    = new_node;
}
 
// Checks whether the filename is present in linked list
// give back pointer to preloaded music or NULL if not found
Mix_Music* search_sound_element(struct Node_sound_element* head, char *filename_with_path)
{
    struct Node_sound_element* current = head;  // Initialize current
    while (current != NULL)
    {
        if ( strcmp( current->filename_with_path, filename_with_path) == 0)
            return current->music;
        current = current->next;
    }
    return NULL;
}
