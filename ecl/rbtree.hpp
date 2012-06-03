/*-
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2012 Gleb Kurtsou <gleb@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ECL_RBTREE_HPP
#define ECL_RBTREE_HPP

#include "impl.hpp"

namespace ecl {

namespace policy { // {{{

struct RBTree {
	struct Default : policy::Generic { };

};

} // namespace policy }}}

struct RBColor {
	enum Enum {
		BLACK = 0,
		RED = 1,
	};
};

template<typename EntryT>
struct RBTreePolicy : policy::RBTree::Default { };

template <typename EntryT>
class RBTreeHead : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef typename EntryType::ObjectType ObjectType;
	typedef typename EntryType::Policy Policy;

	friend struct policy::RBTree;

	RBTreeHead() : rbh_root(NULL) { }

	bool empty() const {
		return (rbh_root == NULL);
	}

	ObjectType *root() {
		return rbh_root;
	}

	const ObjectType *root() const {
		return rbh_root;
	}

	ObjectType *first() {
		return min();
	}

	const ObjectType *first() const {
		return min();
	}

	ObjectType *last() {
		return max();
	}

	const ObjectType *last() const {
		return max();
	}

	ObjectType *min() {
		return min_impl();
	}

	const ObjectType *min() const {
		return min_impl();
	}

	ObjectType *max() {
		return max_impl();
	}

	const ObjectType *max() const {
		return max_impl();
	}

	template<typename KeyType>
	ObjectType *find(const KeyType &key) {
		return find_impl(key);
	}

	/* Finds the node with the same key as elm */
	template<typename KeyType>
	const ObjectType *find(const KeyType &key) const {
		return find_impl(key);
	}

	ObjectType *find_element(const ObjectType *elm) {
		return find_element_impl(elm);
	}

	const ObjectType *find_element(const ObjectType *elm) const {
		return find_element_impl(elm);
	}

	/* Finds the first node greater than or equal to the search key */
	template<typename KeyType>
	ObjectType *nfind(const KeyType &key) {
		return nfind_impl(key);
	}

	/* Finds the first node greater than or equal to the search key */
	template<typename KeyType>
	const ObjectType *nfind(const KeyType &key) const {
		return nfind_impl(key);
	}

	ObjectType *nfind_element(const ObjectType *elm) {
		return nfind_element_impl(elm);
	}

	const ObjectType *nfind_element(const ObjectType *elm) const {
		return nfind_element_impl(elm);
	}

	/* Finds the first node greater than or equal to the search key */
	template<typename KeyType>
	ObjectType *pfind(const KeyType &key) {
		return pfind_impl(key);
	}

	template<typename KeyType>
	const ObjectType *pfind(const KeyType &key) const {
		return pfind_impl(key);
	}

	ObjectType *pfind_element(const ObjectType *elm) {
		return pfind_element_impl(elm);
	}

	const ObjectType *pfind_element(const ObjectType *elm) const {
		return pfind_element_impl(elm);
	}

	ObjectType *insert(ObjectType *obj) {
		ObjectType *tmp;
		ObjectType *parent = NULL;
		int comp = 0;

		tmp = rbh_root;
		while (tmp) {
			parent = tmp;
			comp = compare(obj, parent);
			if (comp < 0)
				tmp = entry(tmp)->rbe_left;
			else if (comp > 0)
				tmp = entry(tmp)->rbe_right;
			else
				return tmp;
		}
		entry(obj)->init(parent);
		if (parent != NULL) {
			if (comp < 0)
				entry(parent)->rbe_left = obj;
			else
				entry(parent)->rbe_right = obj;
		} else
			rbh_root = obj;
		insert_color(obj);
		return NULL;
	}

	ObjectType *remove(ObjectType *elm) {
		ObjectType *child, *parent, *old;
		int color;

		old = elm;
		if (entry(elm)->rbe_left == NULL)
			child = entry(elm)->rbe_right;
		else if (entry(elm)->rbe_right == NULL)
			child = entry(elm)->rbe_left;
		else {
			remove_nontrivial(elm);
			return old;
		}
		parent = entry(elm)->rbe_parent;
		color = entry(elm)->rbe_color;
		if (child)
			entry(child)->rbe_parent = parent;
		if (parent) {
			if (entry(parent)->rbe_left == elm)
				entry(parent)->rbe_left = child;
			else
				entry(parent)->rbe_right = child;
		} else
			rbh_root = child;
		if (color == RBColor::BLACK)
			remove_color(parent, child);
		return old;
	}

protected:
	static int compare(const ObjectType *a, const ObjectType *b) {
		return EntryType::compare(a, b);
	}

	template<typename KeyType>
	static int compare_key(const KeyType &k, const ObjectType *obj) {
		return EntryType::compare_key(k, obj);
	}

	static EntryType *entry(ObjectType *obj) {
		return EntryType::entry(obj);
	}

	static void set_blackred(ObjectType *black, ObjectType *red) {
		entry(black)->rbe_color = RBColor::BLACK;
		entry(red)->rbe_color = RBColor::RED;
	}

	ObjectType *min_impl() const {
		ObjectType *parent, *tmp;

		for (tmp = rbh_root, parent = NULL; tmp;) {
			parent = tmp;
			tmp = entry(tmp)->rbe_left;
		}
		return parent;
	}

	ObjectType *max_impl() const {
		ObjectType *parent, *tmp;

		for (tmp = rbh_root, parent = NULL; tmp;) {
			parent = tmp;
			tmp = entry(tmp)->rbe_right;
		}
		return parent;
	}

	template<typename KeyType>
	ObjectType *find_impl(const KeyType &key) const {
		ObjectType *tmp = rbh_root;
		int comp;

		while (tmp) {
			comp = compare_key(key, tmp);
			if (comp < 0)
				tmp = entry(tmp)->rbe_left;
			else if (comp > 0)
				tmp = entry(tmp)->rbe_right;
			else
				return tmp;
		}
		return NULL;
	}

	ObjectType *find_element_impl(const ObjectType *elm) const {
		ObjectType *tmp = rbh_root;
		int comp;

		while (tmp) {
			comp = compare(elm, tmp);
			if (comp < 0)
				tmp = entry(tmp)->rbe_left;
			else if (comp > 0)
				tmp = entry(tmp)->rbe_right;
			else
				return tmp;
		}
		return NULL;
	}

	template<typename KeyType>
	ObjectType *nfind_impl(const KeyType &key) const {
		ObjectType *tmp = rbh_root;
		ObjectType *res = NULL;
		int comp;

		while (tmp) {
			comp = compare_key(key, tmp);
			if (comp < 0) {
				res = tmp;
				tmp = entry(tmp)->rbe_left;
			} else if (comp > 0)
				tmp = entry(tmp)->rbe_right;
			else
				return tmp;
		}
		return res;
	}

	ObjectType *nfind_element_impl(const ObjectType *elm) const {
		ObjectType *tmp = rbh_root;
		ObjectType *res = NULL;
		int comp;

		while (tmp) {
			comp = compare(elm, tmp);
			if (comp < 0) {
				res = tmp;
				tmp = entry(tmp)->rbe_left;
			} else if (comp > 0)
				tmp = entry(tmp)->rbe_right;
			else
				return tmp;
		}
		return res;
	}

	template<typename KeyType>
	ObjectType *pfind_impl(const KeyType &key) const {
		ObjectType *tmp = rbh_root;
		ObjectType *res = NULL;
		int comp;

		while (tmp) {
			comp = compare_key(key, tmp);
			if (comp < 0)
				tmp = entry(tmp)->rbe_left;
			else if (comp > 0) {
				res = tmp;
				tmp = entry(tmp)->rbe_right;
			} else
				return tmp;
		}
		return res;
	}

	ObjectType *pfind_element_impl(const ObjectType *elm) const {
		ObjectType *tmp = rbh_root;
		ObjectType *res = NULL;
		int comp;

		while (tmp) {
			comp = compare(elm, tmp);
			if (comp < 0)
				tmp = entry(tmp)->rbe_left;
			else if (comp > 0) {
				res = tmp;
				tmp = entry(tmp)->rbe_right;
			} else
				return tmp;
		}
		return res;
	}

	void insert_color(ObjectType *elm) {
		ObjectType *parent, *gparent, *tmp;

		while ((parent = entry(elm)->rbe_parent) != NULL &&
		    entry(parent)->rbe_color == RBColor::RED) {
			gparent = entry(parent)->rbe_parent;
			if (parent == entry(gparent)->rbe_left) {
				tmp = entry(gparent)->rbe_right;
				if (tmp && entry(tmp)->rbe_color == RBColor::RED) {
					entry(tmp)->rbe_color = RBColor::BLACK;
					set_blackred(parent, gparent);
					elm = gparent;
					continue;
				}
				if (entry(parent)->rbe_right == elm) {
					rotate_left(parent);
					tmp = parent;
					parent = elm;
					elm = tmp;
				}
				set_blackred(parent, gparent);
				rotate_right(gparent);
			} else {
				tmp = entry(gparent)->rbe_left;
				if (tmp && entry(tmp)->rbe_color == RBColor::RED) {
					entry(tmp)->rbe_color = RBColor::BLACK;
					set_blackred(parent, gparent);
					elm = gparent;
					continue;
				}
				if (entry(parent)->rbe_left == elm) {
					rotate_right(parent);
					tmp = parent;
					parent = elm;
					elm = tmp;
				}
				set_blackred(parent, gparent);
				rotate_left(gparent);
			}
		}
		entry(rbh_root)->rbe_color = RBColor::BLACK;
	}

	void remove_color(ObjectType *parent, ObjectType *elm) {
		ObjectType *tmp;

		while ((elm == NULL || entry(elm)->rbe_color == RBColor::BLACK) &&
		    elm != rbh_root) {
			if (entry(parent)->rbe_left == elm) {
				tmp = entry(parent)->rbe_right;
				if (entry(tmp)->rbe_color == RBColor::RED) {
					set_blackred(tmp, parent);
					rotate_left(parent);
					tmp = entry(parent)->rbe_right;
				}
				if ((entry(tmp)->rbe_left == NULL ||
				    entry(entry(tmp)->rbe_left)->rbe_color == RBColor::BLACK) &&
				    (entry(tmp)->rbe_right == NULL ||
				    entry(entry(tmp)->rbe_right)->rbe_color == RBColor::BLACK)) {
					entry(tmp)->rbe_color = RBColor::RED;
					elm = parent;
					parent = entry(elm)->rbe_parent;
				} else {
					if (entry(tmp)->rbe_right == NULL ||
					    entry(entry(tmp)->rbe_right)->rbe_color == RBColor::BLACK) {
						ObjectType *oleft;
						if ((oleft = entry(tmp)->rbe_left)
						    != NULL)
							entry(oleft)->rbe_color = RBColor::BLACK;
						entry(tmp)->rbe_color = RBColor::RED;
						rotate_right(tmp);
						tmp = entry(parent)->rbe_right;
					}
					entry(tmp)->rbe_color = entry(parent)->rbe_color;
					entry(parent)->rbe_color = RBColor::BLACK;
					if (entry(tmp)->rbe_right)
						entry(entry(tmp)->rbe_right)->rbe_color = RBColor::BLACK;
					rotate_left(parent);
					elm = rbh_root;
					break;
				}
			} else {
				tmp = entry(parent)->rbe_left;
				if (entry(tmp)->rbe_color == RBColor::RED) {
					set_blackred(tmp, parent);
					rotate_right(parent);
					tmp = entry(parent)->rbe_left;
				}
				if ((entry(tmp)->rbe_left == NULL ||
				    entry(entry(tmp)->rbe_left)->rbe_color == RBColor::BLACK) &&
				    (entry(tmp)->rbe_right == NULL ||
				    entry(entry(tmp)->rbe_right)->rbe_color == RBColor::BLACK)) {
					entry(tmp)->rbe_color = RBColor::RED;
					elm = parent;
					parent = entry(elm)->rbe_parent;
				} else {
					if (entry(tmp)->rbe_left == NULL ||
					    entry(entry(tmp)->rbe_left)->rbe_color == RBColor::BLACK) {
						ObjectType *oright;
						if ((oright = entry(tmp)->rbe_right)
						    != NULL)
							entry(oright)->rbe_color = RBColor::BLACK;
						entry(tmp)->rbe_color = RBColor::RED;
						rotate_left(tmp);
						tmp = entry(parent)->rbe_left;
					}
					entry(tmp)->rbe_color = entry(parent)->rbe_color;
					entry(parent)->rbe_color = RBColor::BLACK;
					if (entry(tmp)->rbe_left)
						entry(entry(tmp)->rbe_left)->rbe_color = RBColor::BLACK;
					rotate_right(parent);
					elm = rbh_root;
					break;
				}
			}
		}
		if (elm)
			entry(elm)->rbe_color = RBColor::BLACK;
	}

	ObjectType *remove_nontrivial(ObjectType *elm) {
		ObjectType *child, *parent, *left, *old;
		int color;

		old = elm;
		elm = entry(elm)->rbe_right;
		while ((left = entry(elm)->rbe_left) != NULL)
			elm = left;
		child = entry(elm)->rbe_right;
		parent = entry(elm)->rbe_parent;
		color = entry(elm)->rbe_color;
		if (child)
			entry(child)->rbe_parent = parent;
		if (parent) {
			if (entry(parent)->rbe_left == elm)
				entry(parent)->rbe_left = child;
			else
				entry(parent)->rbe_right = child;
		} else
			rbh_root = child;
		if (entry(elm)->rbe_parent == old)
			parent = elm;
		entry(elm)->init_copy(entry(old));
		if (entry(old)->rbe_parent) {
			if (entry(entry(old)->rbe_parent)->rbe_left == old)
				entry(entry(old)->rbe_parent)->rbe_left = elm;
			else
				entry(entry(old)->rbe_parent)->rbe_right = elm;
		} else
			rbh_root = elm;
		entry(entry(old)->rbe_left)->rbe_parent = elm;
		if (entry(old)->rbe_right)
			entry(entry(old)->rbe_right)->rbe_parent = elm;
		if (color == RBColor::BLACK)
			remove_color(parent, child);
		return old;
	}

	void rotate_left(ObjectType *elm) {
		ObjectType *tmp;

		tmp = entry(elm)->rbe_right;
		if ((entry(elm)->rbe_right = entry(tmp)->rbe_left) != NULL) {
			entry(entry(tmp)->rbe_left)->rbe_parent = elm;
		}
		if ((entry(tmp)->rbe_parent = entry(elm)->rbe_parent) != NULL) {
			if ((elm) == entry(entry(elm)->rbe_parent)->rbe_left)
				entry(entry(elm)->rbe_parent)->rbe_left = tmp;
			else
				entry(entry(elm)->rbe_parent)->rbe_right = tmp;
		} else
			rbh_root = tmp;
		entry(tmp)->rbe_left = elm;
		entry(elm)->rbe_parent = tmp;
	}

	void rotate_right(ObjectType *elm) {
		ObjectType *tmp;

		tmp = entry(elm)->rbe_left;
		if ((entry(elm)->rbe_left = entry(tmp)->rbe_right) != NULL) {
			entry(entry(tmp)->rbe_right)->rbe_parent = elm;
		}
		if ((entry(tmp)->rbe_parent = entry(elm)->rbe_parent) != NULL) {
			if ((elm) == entry(entry(elm)->rbe_parent)->rbe_left)
				entry(entry(elm)->rbe_parent)->rbe_left = tmp;
			else
				entry(entry(elm)->rbe_parent)->rbe_right = tmp;
		} else
			rbh_root = tmp;
		entry(tmp)->rbe_right = elm;
		entry(elm)->rbe_parent = tmp;
	}

private:
	ObjectType *rbh_root;
};

template <typename EntryT, typename ObjectT>
class RBTreeEntry : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef ObjectT ObjectType;
	typedef RBTreePolicy<EntryType> Policy;

	friend class RBTreeHead<EntryType>;
	friend class impl::Iterator<EntryType>;
	friend class impl::ConstIterator<EntryType>;
	friend class impl::ReverseIterator<EntryType>;
	friend class impl::ConstReverseIterator<EntryType>;
	friend struct policy::RBTree;

	struct Iterator : impl::Iterator<EntryType> {
		typedef impl::Iterator<EntryType> Base;
		ObjectType *init(RBTreeHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ConstIterator : impl::ConstIterator<EntryType> {
		typedef impl::ConstIterator<EntryType> Base;
		const ObjectType *init(const RBTreeHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ReverseIterator : impl::ReverseIterator<EntryType> {
		typedef impl::ReverseIterator<EntryType> Base;
		ObjectType *init(RBTreeHead<EntryType> *head) {
			return Base::start_at(head->last());
		}
	};

	struct ConstReverseIterator : impl::ConstReverseIterator<EntryType> {
		typedef impl::ConstReverseIterator<EntryType> Base;
		const ObjectType *init(const RBTreeHead<EntryType> *head) {
			return Base::start_at(head->last());
		}
	};

	static int compare(const ObjectType *a, const ObjectType *b) {
		return EntryType::compare_fn(a, b);
	}

	template<typename KeyType>
	static int compare_key(const KeyType &key, const ObjectType *obj) {
		return EntryType::compare_key_fn(key, obj);
	}

	RBTreeEntry() {
		Policy::create_entry(this);
	}

	~RBTreeEntry() {
		Policy::destroy_entry(this);
	}

	ObjectType *left() {
		return this->rbe_left;
	}

	const ObjectType *left() const {
		return this->rbe_left;
	}

	ObjectType *right() {
		return this->rbe_right;
	}

	const ObjectType *right() const {
		return this->rbe_right;
	}

	ObjectType *parent() {
		return this->rbe_parent;
	}

	const ObjectType *parent() const {
		return this->rbe_parent;
	}

	ObjectType *next() {
		return next_impl();
	}

	const ObjectType *next() const {
		return next_impl();
	}

	ObjectType *prev() {
		return prev_impl();
	}

	const ObjectType *prev() const {
		return prev_impl();
	}

protected:
	static EntryType *entry(ObjectType *obj) {
		return obj;
	}

	static const EntryType *entry(const ObjectType *obj) {
		return obj;
	}

	void init(ObjectType *parent) {
		rbe_parent = parent;
		rbe_left = rbe_right = NULL;
		rbe_color = RBColor::RED;
	}

	void init_copy(EntryType *a) {
		rbe_parent = a->rbe_parent;
		rbe_left = a->rbe_left;
		rbe_right = a->rbe_right;
		rbe_color = a->rbe_color;
	}

	ObjectType *object() const {
		return impl::entry_to_object<ObjectType, RBTreeEntry>(this);
	}

	ObjectType *next_impl() const {
		ObjectType *elm = object();

		if (entry(elm)->rbe_right) {
			elm = entry(elm)->rbe_right;
			while (entry(elm)->rbe_left)
				elm = entry(elm)->rbe_left;
		} else {
			if (entry(elm)->rbe_parent &&
			    (elm == entry(entry(elm)->rbe_parent)->rbe_left))
				elm = entry(elm)->rbe_parent;
			else {
				while (entry(elm)->rbe_parent &&
				    (elm == entry(entry(elm)->rbe_parent)->rbe_right))
					elm = entry(elm)->rbe_parent;
				elm = entry(elm)->rbe_parent;
			}
		}
		return elm;
	}

	ObjectType *prev_impl() const {
		ObjectType *elm = object();

		if (entry(elm)->rbe_left) {
			elm = entry(elm)->rbe_left;
			while (entry(elm)->rbe_right)
				elm = entry(elm)->rbe_right;
		} else {
			if (entry(elm)->rbe_parent &&
			    (elm == entry(entry(elm)->rbe_parent)->rbe_right))
				elm = entry(elm)->rbe_parent;
			else {
				while (entry(elm)->rbe_parent &&
				    (elm == entry(entry(elm)->rbe_parent)->rbe_left))
					elm = entry(elm)->rbe_parent;
				elm = entry(elm)->rbe_parent;
			}
		}
		return elm;
	}

	ObjectType *rbe_left;
	ObjectType *rbe_right;
	ObjectType *rbe_parent;
	RBColor::Enum rbe_color;
};

} // namespace ecl

#endif
