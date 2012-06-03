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

#ifndef ECL_SLIST_HPP
#define ECL_SLIST_HPP

#include "impl.hpp"

namespace ecl {

namespace policy { // {{{

struct SList {
	struct Default : policy::Generic { };

	struct Debug {
		struct RemoveCtx {
			void **old_next;
		};

		template<typename EntryT>
		static void create_entry(EntryT *ent) {
			ent->sle_next = NULL;
		}

		template<typename EntryT>
		static void destroy_entry(EntryT *ent) {
			assert(ent->sle_next == NULL || ent->sle_next == (void *)-1);
		}

		template<typename EntryT>
		static void remove_pre(RemoveCtx &ctx, EntryT *ent) {
			ctx.old_next = (void **)&ent->sle_next;
		}

		template<typename EntryT>
		static void remove_post(RemoveCtx &ctx, EntryT *ent) {
			*ctx.old_next = (void *)-1;
		}
	};
};

} // namespace policy }}}

template<typename EntryT>
struct SListPolicy : policy::SList::Debug { };
// struct SListPolicy : policy::SList::Default { };

template <typename EntryT>
class SListHead : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef typename EntryType::ObjectType ObjectType;
	typedef typename EntryType::Policy Policy;

	friend struct policy::SList;

	SListHead() : slh_first(NULL) { }

	bool empty() const {
		return (slh_first == NULL);
	}

	ObjectType *first() {
		return slh_first;
	}

	const ObjectType *first() const {
		return slh_first;
	}

	void insert_head(ObjectType *obj) {
		entry(obj)->sle_next = slh_first;
		slh_first = obj;
	}

	void remove(ObjectType *listobj) {
		if (slh_first == listobj) {
			remove_head();
		} else {
			ObjectType *curobj = slh_first;
			while (entry(curobj)->sle_next != listobj)
				curobj = entry(curobj)->sle_next;
			entry(curobj)->remove_after();
		}
	}

	void remove_head() {
		typename Policy::RemoveCtx ctx;
		EntryType *ent;

		ent = entry(slh_first);
		Policy::remove_pre(ctx, ent);
		slh_first = ent->sle_next;
		Policy::remove_post(ctx, ent);
	}

	void swap(SListHead *nhead) {
		ObjectType *swap_first = this->slh_first;
		this->slh_first = nhead->slh_first;
		nhead->slh_first = swap_first;
	}

protected:
	static EntryType *entry(ObjectType *obj) {
		return EntryType::entry(obj);
	}

private:
	ObjectType *slh_first;
};

template <typename EntryT, typename ObjectT>
class SListEntry : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef ObjectT ObjectType;
	typedef SListPolicy<EntryType> Policy;

	friend class SListHead<EntryType>;
	friend class impl::Iterator<EntryType>;
	friend class impl::ConstIterator<EntryType>;
	friend struct policy::SList;

	struct Iterator : impl::Iterator<EntryType> {
		typedef impl::Iterator<EntryType> Base;
		ObjectType *init(SListHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ConstIterator : impl::ConstIterator<EntryType> {
		typedef impl::ConstIterator<EntryType> Base;
		const ObjectType *init(const SListHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	SListEntry() {
		Policy::create_entry(this);
	}

	~SListEntry() {
		Policy::destroy_entry(this);
	}

	ObjectType *next() {
		return this->sle_next;
	}

	const ObjectType *next() const {
		return this->sle_next;
	}

	void insert_after(ObjectType *listobj) {
		this->sle_next = entry(listobj)->sle_next;
		entry(listobj)->sle_next = object();
	}

	void remove_after() {
		typename Policy::RemoveCtx ctx;
		EntryType *ent;

		ent = entry(sle_next);
		Policy::remove_pre(ctx, ent);
		sle_next = ent->sle_next;
		Policy::remove_post(ctx, ent);
	}

protected:
	static EntryType *entry(ObjectType *obj) {
		return obj;
	}

	static const EntryType *entry(const ObjectType *obj) {
		return obj;
	}

	ObjectType *object() {
		return impl::entry_to_object<ObjectType, SListEntry>(this);
	}

	ObjectType *sle_next;
};

} // namespace ecl

#endif
