// -*- c++ -*-
#ifndef LIST_H
#define LIST_H

struct ListNode {
	ListNode *prev, *next;

	void Delete() {
		prev->next = next;
		next->prev = prev;
		// next = prev = 0;
	}

	void InsertBefore(ListNode *children) {
		next = children;
		prev = children->prev;

		children->prev = this;
		prev->next = this;
	}
	void InsertAfter(ListNode *parent) {
		next = parent->next;
		prev = parent;

		prev->next = this;
		next->prev = this;
	}

	void InitHead() {
		next = prev = this;
	}

	bool is_empty() { return next == this; }
};

#endif /* LIST_H */
