#include "apachetop.h"

Queue::Queue(void)
{
	size = 0;
	head = NULL;
	tail = NULL;
}
Queue::~Queue(void)
{
	delete head;
}

void Queue::push(void *q)
{
	Node *temp;

	temp = new Node;

	temp->x = q;
	temp->next = NULL;

	if (size == 0)
	{
		head = temp;
		tail = temp;
	}
	else
	{
		tail->next = temp;
		tail = temp;
	}
	size++;
}

// remove and return first entry
int Queue::pop(void **q)
{
	Node *temp;

	if (!head)
	{
		*q = NULL;
		return -1;
	}

	// return the first entry
	*q = head->x;

	if (head->next)
		// we have another node to point at
		temp = head->next;
	else
		// no more nodes
		temp = NULL;

	delete head;
	head = temp;

	size--;

	return 0; // success
}

int Queue::entries(void)
{
	return size;
}
