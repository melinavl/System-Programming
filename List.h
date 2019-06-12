#ifndef LIST_H_
#define LIST_H_
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

class Listnode
{
	int id;
	int spid;
	int rpid;
	bool exist;
	int counts;
	int countr;
	Listnode *next;
public:
	Listnode()
	{
		id = 0;
		exist = false;
		next = NULL;
		spid = 0;
		counts = countr = 0;
		rpid = 0;
	}
	int getId() const
	{
		return id;
	}
	void setId(int bitcoinId)
	{
		this->id = bitcoinId;
	}
	Listnode* getNext()
	{
		return next;
	}
	void setNext(Listnode* next)
	{
		this->next = next;
	}
	bool getExist()
	{
		return exist;
	}
	void setExist(bool exist)
	{
		this->exist = exist;
	}
	void incCounterR()
	{
		this->countr++;
	}
	void incCounterS()
	{
		this->counts++;
	}
	int getCounterR()
	{
		return this->countr;
	}
	int getCounterS()
	{
		return this->counts;
	}
	int getsPid() const
	{
		return spid;
	}
	void setsPid(int pid)
	{
		this->spid = pid;
	}
	int getrPid() const
	{
		return rpid;
	}
	void setrPid(int pid)
	{
		this->rpid = pid;
	}
};

class List
{
	Listnode* start;
	int count;
public:
	List()
	{
		start = NULL;
		count = 0;
	}
	void insertNode(int id);
	void deleteFirstNode();
	bool searchList(int id);
	void removeNode(int id);
	Listnode* searchPid(int pid);
	Listnode* getStart()
	{
		return start;
	}
	void resetExist();
	void updatePid(int j, int pid, bool sender);
	~List();
};

#endif /* LIST_H_ */
