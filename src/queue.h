#ifndef _QUEUE_H_
#define _QUEUE_H_

class Node
{
	public:
	void *x;
	Node *next;

	Node() { next = NULL; }
};


class Queue
{
	private:
	Node *head;
	Node *tail;
	int size;

	public:
	Queue(void);
	~Queue(void);

	void push(void *r);
	int pop(void **r);
	int entries(void);
};

#endif
