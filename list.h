/*FIFO linked list queue data structure*/

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

/*initialize list with NULL head/tail and 0 size*/
list *initList(list *list_p);

/*enqueues data_p to list_p, returns created node*/
node* enqueue(list *list_p, void *data_p);

/*dequeues head of list_p, returns it*/
node* dequeue(list *list_p);