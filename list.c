/**
 * @file: list.c
 * @author: Michael Permyashkin
 * 
 * @brief: Implementations of linked list utilities functions for basic operations
 * 
 * Implementations of function signatures found in @file list.h. Functions perform
 * basic operations on the linked list such as:
 *      - adding a node
 *      - deleting a node
 *      - getting the length of the list
 *      - checking if the list is empty
 *      - printing contents of the list
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "internal.h"

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


/**
 * Joins two lists by insert list immediately after the head.
 *      Note: the original head node will point to the new list’s head node.  
 *      The new lists tail node will be updated to point to the original lists next node, completing the chain.
 * 
 * @param list: the linked list that goes directly in front of head (the front of the master list [A, B])
 * @param head: the linked list that goes after @list (the back of the master list [C, D])
 **/ 
void list_splice(struct list_head *list, struct list_head *head) {
    list->prev->next = head->next; // last node of list points to first node of head     (B -> C)
    head->next->prev = list->prev; // first node of head points to the last node of list (B <-> C)
    list->prev = head->prev;       // list prev points to the last node of head          (D <- list) 
    head->next = list->next;       // head now points to the first node of list          (head -> A)
    head->next->prev = head;       // head->next now points to A, so we now want:        (head <-> A)
}


/** Removes the item from the list.  
 *      Since the entry that is removed is no longer part of the original list, 
 *      update its next and prev pointers to itself (like LIST_INIT).
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

/**
 * Displays the content value of each element in the list provided, if the list is not empty
 * 
 * @head: the head node of a given linked list
 **/
void list_print(struct list_head *head) {
    if (!list_empty(head)) {
        struct argument_t *entry; // the wrapping structure of each node in the list
        struct list_head *curr; // the current node we are at in the traversal

        // list traversal in order and stops when we cycled back to the start (circularly linked list)
        for (curr = head->next; curr != head; curr = curr->next) {
            // extract the nodes structure and print the contents (the value of the argument)
            entry = list_entry(curr, struct argument_t, list);
            printf("(%s)\n", entry->value);
        }
    }
}

/**
 * Converts linked list to array
 * 
 * @head: the head node of a given linked list
 * 
 */ 
void list_to_arr(struct list_head *head, char *arr[]) {
    if (list_empty(head)) return;

    int i = 0;
    struct argument_t *entry; // the wrapping structure of each node in the list
    struct list_head *curr; // the current node we are at in the traversal

    // list traversal in order and stops when we cycled back to the start (circularly linked list)
    for (curr = head->next; curr != head; curr = curr->next) {
        // extract the nodes structure
        entry = list_entry(curr, struct argument_t, list);
        char *val = entry->value;
        arr[i++] = strdup(entry->value);
    }
    arr[i] = NULL;
}