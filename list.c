#include <stdlib.h>
#include <stdio.h>
#include "list.h"

list *initList(list *list_p)
{
	list_p->head = NULL;
        list_p->tail = NULL;
	list_p->size = 0;
	return list_p;
}

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