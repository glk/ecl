/*-
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

/*
 * Derived from sys/queue.h
 */

#ifndef ECL_LIST_HPP
#define ECL_LIST_HPP

#include "impl.hpp"

namespace ecl {

namespace policy { // {{{

struct List {
	struct Default : policy::Generic { };

	struct Debug {
		struct RemoveCtx {
			void **old_next;
			void **old_prev;
		};

		template<typename EntryT>
		static void create_entry(EntryT *ent) {
			ent->le_next = NULL;
			ent->le_prev = NULL;
		}

		template<typename EntryT>
		static void destroy_entry(EntryT *ent) {
			assert(ent->le_next == NULL || ent->le_next == (void *)-1);
			assert(ent->le_prev == NULL || ent->le_prev == (void *)-1);
		}

		template<typename HeadT>
		static void check_head(HeadT *head) {
			if (head->lh_first != NULL) {
				typename HeadT::EntryType *first = HeadT::entry(head->lh_first);
				assert(first->le_prev == &head->lh_first);
			}
		}

		template<typename EntryT>
		static void check_next(EntryT *ent) {
			if (ent->le_next != NULL) {
				EntryT *next = EntryT::entry(ent->le_next);
				assert(next->le_prev == &ent->le_next);
			}
		}

		template<typename EntryT>
		static void check_prev(EntryT *ent) {
			assert(*ent->le_prev == ent);
		}

		template<typename EntryT>
		static void remove_pre(RemoveCtx &ctx, EntryT *ent) {
			ctx.old_next = (void **)&ent->le_next;
			ctx.old_prev = (void **)&ent->le_prev;
		}

		template<typename EntryT>
		static void remove_post(RemoveCtx &ctx, EntryT *ent) {
			*ctx.old_next = (void *)-1;
			*ctx.old_prev = (void *)-1;
		}
	};
};

} // namespace policy }}}

template<typename EntryT>
struct ListPolicy : policy::List::Debug { };
// struct ListPolicy : policy::List::Default { };

template <typename EntryT>
class ListHead : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef typename EntryType::ObjectType ObjectType;
	typedef typename EntryType::Policy Policy;

	friend struct policy::List;

	ListHead() : lh_first(NULL) { }

	bool empty() const {
		return (lh_first == NULL);
	}

	ObjectType *first() {
		return lh_first;
	}

	const ObjectType *first() const {
		return lh_first;
	}

	void insert_head(ObjectType *obj) {
		Policy::check_head(this);

		if ((entry(obj)->le_next = lh_first) != NULL)
			entry(lh_first)->le_prev = &entry(obj)->le_next;
		lh_first = obj;
		entry(obj)->le_prev = &lh_first;
	}

	void swap(ListHead *nhead) {
		ObjectType *swap_tmp = this->lh_first;
		this->lh_first = nhead->lh_first;
		nhead->lh_first = swap_tmp;
		if ((swap_tmp = this->lh_first) != NULL)
			entry(swap_tmp)->le_prev = &this->lh_first;
		if ((swap_tmp = nhead->lh_first) != NULL)
			entry(swap_tmp)->le_prev = &nhead->lh_first;
	}

protected:
	static EntryType *entry(ObjectType *obj) {
		return EntryType::entry(obj);
	}

private:
	ObjectType *lh_first;
};

template <typename EntryT, typename ObjectT>
class ListEntry : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef ObjectT ObjectType;
	typedef ListPolicy<EntryType> Policy;

	friend class ListHead<EntryType>;
	friend class impl::Iterator<EntryType>;
	friend class impl::ConstIterator<EntryType>;
	friend struct policy::List;

	struct Iterator : impl::Iterator<EntryType> {
		typedef impl::Iterator<EntryType> Base;
		ObjectType *init(ListHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ConstIterator : impl::ConstIterator<EntryType> {
		typedef impl::ConstIterator<EntryType> Base;
		const ObjectType *init(const ListHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	ListEntry() {
		Policy::create_entry(this);
	}

	~ListEntry() {
		Policy::destroy_entry(this);
	}

	ObjectType *next() {
		return this->le_next;
	}

	const ObjectType *next() const {
		return this->le_next;
	}

	void insert_after(ObjectType *listobj) {
		Policy::check_next(entry(listobj));

		if ((this->le_next = entry(listobj)->le_next) != NULL)
			entry(entry(listobj)->le_next)->le_prev = &this->le_next;
		entry(listobj)->le_next = object();
		this->le_prev = &entry(listobj)->le_next;
	}

	void insert_before(ObjectType *listobj) {
		Policy::check_prev(entry(listobj));

		this->le_prev = entry(listobj)->le_prev;
		this->le_next = listobj;
		*entry(listobj)->le_prev = object();
		entry(listobj)->le_prev = &this->le_next;
	}

	void remove() {
		typename Policy::RemoveCtx ctx;

		Policy::remove_pre(ctx, this);
		Policy::check_next(this);
		Policy::check_prev(this);

		if (this->le_next != NULL)
			entry(this->le_next)->le_prev = this->le_prev;
		*this->le_prev = this->le_next;

		Policy::remove_post(ctx, this);
	}

protected:
	static EntryType *entry(ObjectType *obj) {
		return obj;
	}

	static const EntryType *entry(const ObjectType *obj) {
		return obj;
	}

	ObjectType *object() {
		return impl::entry_to_object<ObjectType, ListEntry>(this);
	}

	ObjectType *le_next;
	ObjectType **le_prev;
};

} // namespace ecl

#endif
