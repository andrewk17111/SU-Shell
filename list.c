/**
 * @file: list.c
 * @author: Michael Permyashkin
 * 
 * @brief: Implementations of linked list utilities functions for operations to a list.
 * 
 * Functions defined provide the ability to add nodes, remove nodes, get the list size, 
 * check is the list is empty and join lists.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

/**
 * Adds the new item at to the list, at the front
 * 
 * @param new: the node we are adding
 * @param head: the head node of the list we are prepending @new to
 **/ 
void list_add(struct list_head *new, struct list_head *head) {
    new->next = head->next; // new points to the previous first node                             (new -> previous_first)
    new->prev = head;       // new points back to head since it is the first node now            (head <- new)
    head->next->prev = new; // the previous first node now points back to new since it is second (new <-> previous_first)
    head->next = new;       // head now points to new since it is second                         (head <-> new)
}


/**
 * Adds the new item at to the list, at the end
 * 
 * @param new: the node we are adding
 * @param head: the head node of the list we are appending @new to
 **/ 
void list_add_tail(struct list_head *new, struct list_head *head) {
    new->next = head;       // since new is going to the end of the circular list, it points back to head (new -> head)
    head->prev->next = new; // the previous tail node now points to the new tail node we added            (old_tail -> new)
    new->prev = head->prev; // new points back to the previous tail node                                  (old_tail <-> new)
    head->prev = new;       // head points back to the new tail node                                      (new <-> head)
}


/** Removes the item from the list.  
 *  Since the entry that is removed is no longer part of the original list, 
 *  update its next and prev pointers to itself (like LIST_INIT).
 *
 * @entry: the node we want to remove from the list
 **/  
void list_del(struct list_head *entry) {
    // we need to assign the pointers of entries adjacent nodes so that they now skip over entry
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;

    // now that entry has been "removed" from the list, it can now become a list of itself
    entry->next = entry;
    entry->prev = entry;
}


/**
 * Returns non-zero if the list is empty, or 0 if the list is not empty.
 * 
 * @param head: head node of the list we are checking
 * @return: 0 if not empty, 1 if empty
 **/ 
int list_empty(struct list_head *head) {
    // if the list points to nothing or contains only itself --> empty
    if (head->next == NULL || head->next == head)
        return 1;
    return 0; // not empty
}


/**
 * Returns the length of the linked list given head node
 * 
 * @param head: head node of the list we are checking
 * @return: size of the lists
 **/ 
int list_size(struct list_head *head) {
    int size = 0;
    if (list_empty(head)) return size;

    // list traversal in order and stops when we cycled back to the start (circularly linked list)
    struct list_head *curr;
    for (curr = head->next; curr != head; curr = curr->next) {
        size++;
    }

    return size; 
}