#include <stdlib.h>
#include <stdio.h>
#include "list.h"

typedef struct node{
	int val;
	void *data;
	struct node *next;

} node;


typedef struct list{
	node *head;
        node *tail;
	int size;
} list;

/*	initialize list passed to default vals*/
list *initList(list *list_p)
{
	list_p->head = NULL;
        list_p->tail = NULL;
	list_p->size = 0;
	return list_p;
}

/*	add a node to front of list_p, with tid val_p, and either an
	alien or bullet struct to be saved as the node's
	'data'*/

node* enqueue(list *list_p, void *data_p)
{
	node *newNode = malloc(sizeof(node));
	newNode->data = data_p;
	newNode->next = NULL;
	list_p->size += 1;

	if (list_p->head == NULL && list_p->tail == NULL)
	{
		list_p->head = newNode;
                list_p->tail = newNode;
	}
        else
        {
            list_p->tail->next = newNode;
            list_p->tail = newNode;
        }
	    
	return newNode;
}

/*	 removes node in list_p containing tid val_p */
node* dequeue(list *list_p)
{
	list_p->size += -1;
        node *foundNode = malloc(sizeof(node));
        
        if (list_p->head == NULL)
        {
            foundNode = NULL;
        }
        if(list_p->head == list_p->tail)
        {
            foundNode = list_p->head;
            list_p->head = NULL;
            list_p->tail = NULL;
        }
        else 
        {
            foundNode = list_p->head;
            list_p->head = list_p->head->next;
            
        }

	return foundNode;
}