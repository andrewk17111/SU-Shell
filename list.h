/**
 * File: list.h
 * Purpose: Contains function signatures for linked list operations. Defines
 *      function like macros which we use to initialize a linked list and nodes
 *      of the list.
 * 
 * @author: Michael Permyashkin
 **/ 

#include <stddef.h>

#ifndef LIST_H
#define LIST_H

/**
 * List head initializations
 * Sets next, previous == self
 **/
#define LIST_HEAD_INIT(name) \
    {                        \
        &(name), &(name)     \
    }

/** 
 * Initalizes head node where the head will initially point prev, next to itself
 * expands to -> struct list_head name = { &(name), &(name) }
 * 
 * @param name: name of the linked list
 **/
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

/**
 * list_entry - cast a member of a structure of to the containing structure
 * 
 * @param ptr:        the pointer to the member
 * @param type:       the tupe of the container struct this is embedded in
 * @param member:    the name of the member within the struct
 **/
#define list_entry(ptr, type, member) ({         \
    void *__mptr = (void *)(ptr);                \
    ((type *)(__mptr - offsetof(type, member))); \
})

/**
 * Type definition of list head which has holds points to the list nodes
 * directly proceeding and preceeding the given node
 **/
struct list_head
{
    struct list_head *next, *prev;
};

void list_add(struct list_head *new, struct list_head *head);

void list_add_tail(struct list_head *new, struct list_head *head);

void list_del(struct list_head *entry);

int list_empty(struct list_head *head);

int list_size(struct list_head *head);

void list_splice(struct list_head *list, struct list_head *head);

void list_print(struct list_head *head);

#endif