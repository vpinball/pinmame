#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

/* Link list node */
struct Node_sound_element
{
    char filename_with_path[512];;
    Mix_Music *music; //our pointer to preloaded music
    struct Node_sound_element* next;
};

void push_sound_element(struct Node_sound_element** head_ref, char *new_filename_with_path, Mix_Music *new_music;);
Mix_Music* search_sound_element(struct Node_sound_element* head, char *filename_with_path);

#endif  // _LINKED_LIST_H






