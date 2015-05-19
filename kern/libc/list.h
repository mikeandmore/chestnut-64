// -*- c++ -*-
#ifndef LIST_H
#define LIST_H

struct ListNode {
	ListNode *prev, *next;

	void Delete() {
		// TODO: here
		prev->next = next;
		next->prev = prev;
	}

	void InsertAfter(ListNode *parent) {
		// TODO: here
		next = parent->next;
		prev = parent;

		parent->next = this;
		next->prev = this;
	}
};

#endif /* LIST_H */
