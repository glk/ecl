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

#ifndef ECL_STAILQ_HPP
#define ECL_STAILQ_HPP

#include "impl.hpp"

namespace ecl {

namespace policy { // {{{

struct STailq {
	struct Default : policy::Generic { };

	struct Debug {
		struct RemoveCtx {
			void **old_next;
		};

		template<typename EntryT>
		static void create_entry(EntryT *ent) {
			ent->stqe_next = NULL;
		}

		template<typename EntryT>
		static void destroy_entry(EntryT *ent) {
			assert(ent->stqe_next == NULL || ent->stqe_next == (void *)-1);
		}

		template<typename EntryT>
		static void remove_pre(RemoveCtx &ctx, EntryT *ent) {
			ctx.old_next = (void **)&ent->stqe_next;
		}

		template<typename EntryT>
		static void remove_post(RemoveCtx &ctx, EntryT *ent) {
			*ctx.old_next = (void *)-1;
		}
	};
};

} // namespace policy }}}

template<typename EntryT>
struct STailqPolicy : policy::STailq::Debug { };
// struct STailqPolicy : policy::STailq::Default { };

template <typename EntryT>
class STailqHead : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef typename EntryType::ObjectType ObjectType;
	typedef typename EntryType::Policy Policy;

	friend struct policy::STailq;

	STailqHead() {
		init();
	}

	bool empty() const {
		return (stqh_first == NULL);
	}

	ObjectType *first() {
		return stqh_first;
	}

	const ObjectType *first() const {
		return stqh_first;
	}

	ObjectType *last() {
		return stqh_last;
	}

	const ObjectType *last() const {
		return stqh_last;
	}

	void concat(STailqHead *nhead) {
		if (!nhead->empty()) {
			if (this->stqh_last != NULL)
				entry(this->stqh_last)->stqe_next =
				    nhead->stqh_first;
			else
				this->stqh_first = nhead->stqh_first;
			this->stqh_last = nhead->stqh_last;
			nhead->init();
		}
	}

	void insert_after(ObjectType *listobj, ObjectType *obj) {
		if ((entry(obj)->stqe_next = entry(listobj)->stqe_next) == NULL)
			stqh_last = obj;
		entry(listobj)->stqe_next = obj;
	}

	void insert_head(ObjectType *obj) {
		if ((entry(obj)->stqe_next = stqh_first) == NULL)
			stqh_last = obj;
		stqh_first = obj;
	}

	void insert_tail(ObjectType *obj) {
		entry(obj)->stqe_next = NULL;
		if (stqh_last != NULL)
			entry(stqh_last)->stqe_next = obj;
		stqh_last = obj;
	}

	void remove(ObjectType *listobj) {
		if (stqh_first == listobj) {
			remove_head();
		} else {
			ObjectType *curobj = stqh_first;
			while (entry(curobj)->stqe_next != listobj)
				curobj = entry(curobj)->stqe_next;
			remove_after(curobj);
		}
	}

	void remove_after(ObjectType *listobj) {
		typename Policy::RemoveCtx ctx;
		EntryType *ent;

		ent = entry(entry(listobj)->stqe_next);
		Policy::remove_pre(ctx, ent);
		entry(listobj)->stqe_next = ent->stqe_next;
		if (ent->stqe_next == NULL)
			stqh_last = listobj;
		Policy::remove_post(ctx, ent);
	}

	void remove_head() {
		typename Policy::RemoveCtx ctx;
		EntryType *ent;

		ent = entry(stqh_first);
		Policy::remove_pre(ctx, ent);
		stqh_first = ent->stqe_next;
		if (stqh_first == NULL)
			stqh_last = NULL;
		Policy::remove_post(ctx, ent);
	}

	void swap(STailqHead *nhead) {
		ObjectType *swap_first = this->stqh_first;
		ObjectType *swap_last = this->stqh_last;

		this->stqh_first = nhead->stqh_first;
		this->stqh_last = nhead->stqh_last;
		nhead->stqh_first = swap_first;
		nhead->stqh_last = swap_last;
		if (this->empty())
			this->stqh_last = NULL;
		if (nhead->empty())
			nhead->stqh_last = NULL;
	}

protected:
	inline void init() {
		stqh_first = NULL;
		stqh_last = NULL;
	}

	static EntryType *entry(ObjectType *obj) {
		return EntryType::entry(obj);
	}

private:
	ObjectType *stqh_first;
	ObjectType *stqh_last;
};

template <typename EntryT, typename ObjectT>
class STailqEntry : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef ObjectT ObjectType;
	typedef STailqPolicy<EntryType> Policy;

	friend class STailqHead<EntryType>;
	friend class impl::Iterator<EntryType>;
	friend class impl::ConstIterator<EntryType>;
	friend struct policy::STailq;

	struct Iterator : impl::Iterator<EntryType> {
		typedef impl::Iterator<EntryType> Base;
		ObjectType *init(STailqHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ConstIterator : impl::ConstIterator<EntryType> {
		typedef impl::ConstIterator<EntryType> Base;
		const ObjectType *init(const STailqHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	STailqEntry() {
		Policy::create_entry(this);
	}

	~STailqEntry() {
		Policy::destroy_entry(this);
	}

	ObjectType *next() {
		return this->stqe_next;
	}

	const ObjectType *next() const {
		return this->stqe_next;
	}

protected:
	static EntryType *entry(ObjectType *obj) {
		return obj;
	}

	static const EntryType *entry(const ObjectType *obj) {
		return obj;
	}

	ObjectType *object() {
		return impl::entry_to_object<ObjectType, STailqEntry>(this);
	}

	ObjectType *stqe_next;
};

} // namespace ecl

#endif
