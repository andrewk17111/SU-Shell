/**
 * @file: list.h
 * @author: Michael Permyashkin
 * 
 * @brief: Header file for linked list utility functions for basic operations and function
 * like macros for initalization and node extraction
 * 
 * Defintions of function-like macros to assist with list initialization and list node
 * types. Functions provide ability to add, remove and delete nodes, check list size and whether
 * the list is empty
 */ 

#include <stddef.h>

#ifndef LIST_H
#define LIST_H

/**
 * List head initializations
 * Sets next, previous == self
 */
#define LIST_HEAD_INIT(name) \
    {                        \
        &(name), &(name)     \
    }

/** 
 * Initalizes head node where the head will initially point prev, next to itself
 * expands to -> struct list_head name = { &(name), &(name) }
 * 
 * @param name: name of the linked list
 */
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

/**
 * list_entry - cast a member of a structure of to the containing structure
 * 
 * @param ptr:        the pointer to the member
 * @param type:       the tupe of the container struct this is embedded in
 * @param member:    the name of the member within the struct
 */
#define list_entry(ptr, type, member) ({         \
    void *__mptr = (void *)(ptr);                \
    ((type *)(__mptr - offsetof(type, member))); \
})

/**
 * Type definition of list head which has holds points to the list nodes
 * directly proceeding and preceeding the given node
 */
struct list_head
{
    struct list_head *next, *prev;
};

/**
 * Adds the new item at to the list, at the front
 * 
 * @param new: the node we are adding
 * @param head: the head node of the list we are prepending @new to
 **/ 
void list_add(struct list_head *new, struct list_head *head);

/**
 * Adds the new item at to the list, at the end
 * 
 * @param new: the node we are adding
 * @param head: the head node of the list we are appending @new to
 **/ 
void list_add_tail(struct list_head *new, struct list_head *head);

/** 
 * Removes the item from the list.  
 * Since the entry that is removed is no longer part of the original list, 
 * update its next and prev pointers to itself (like LIST_INIT).
 *
 * @param entry: the node we want to remove from the list
 **/ 
void list_del(struct list_head *entry);

/**
 * Returns non-zero if the list is empty, or 0 if the list is not empty.
 * 
 * @param head: head node of the list we are checking
 * @return: 0 if not empty, 1 if empty
 **/ 
int list_empty(struct list_head *head);

/**
 * Returns the length of the linked list given head node
 * 
 * @param head: head node of the list we are checking
 * @return: size of the lists
 **/ 
int list_size(struct list_head *head);

#endif