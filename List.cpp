#include <iostream>
#include <string.h>
#include <cstring>
#include <time.h>

#include "List.h"

using namespace std;

void List::insertNode(int id)
{
	Listnode* n = new (Listnode);
	n->setId(id);

	if (this->count == 0)
		//first node
		n->setNext(NULL);
	else
		//not first node
		n->setNext(this->start);

	this->start = n;
	this->count++;
}
bool List::searchList(int id)
{
	Listnode* temp = this->start;
	while (temp != NULL)
	{
		if (temp->getId() == id)
			return 1;
		temp = temp->getNext();
	}
	return 0;
}
void List::deleteFirstNode()
{
	if (this->count == 0)
		return;
	else if (this->count > 1)
	{
		//has more than 1 nodes
		Listnode* second = this->start->getNext();
		delete this->start;
		this->start = second;
		this->count--;
	}
	else if (this->count == 1)
	{
		//if it has 1 node
		delete this->start;
		this->start = NULL;
		this->count--;
	}
}

List::~List()
{
	while (this->count)
		this->deleteFirstNode();
}

void List::removeNode(int id)
{
	//if it's the first node
	if (this->count > 0 && this->start->getId() == id)
		deleteFirstNode();

	Listnode* temp = this->start;
	Listnode* prev = NULL;
	while (temp != NULL)
	{
		if (temp->getId() == id)
		{
			prev->setNext(temp->getNext());
			delete temp;
			count--;
			return;
		}
		prev = temp;
		temp = temp->getNext();
	}
}

void List::resetExist()
{
	Listnode* temp = this->start;
	while (temp != NULL)
	{
		temp->setExist(false);
		temp = temp->getNext();
	}
}

Listnode* List::searchPid(int pid)
{
	Listnode* temp = this->start;
	while (temp != NULL)
	{
		if ((temp->getrPid() == pid) || (temp->getsPid()==pid))
			return temp;
		temp = temp->getNext();
	}
	return NULL;
}

void List::updatePid(int j, int pid, bool sender)
{
	Listnode* temp = this->start;
	while (temp != NULL)
	{
		if (sender == true)
		{
			if (temp->getId() == j)
			{
				temp->setsPid(pid);
				return;
			}
		}
		else
		{
			if (temp->getId() == j)
			{
				temp->setrPid(pid);
				return;
			}
		}
		temp = temp->getNext();
	}
}
