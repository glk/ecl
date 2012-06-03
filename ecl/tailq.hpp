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

#ifndef ECL_TAILQ_HPP
#define ECL_TAILQ_HPP

#include "impl.hpp"

namespace ecl {

namespace policy { // {{{

struct Tailq {
	struct Default : Generic { };

	struct Debug {
		struct RemoveCtx {
			void **old_next;
			void **old_prev;
		};

		template<typename EntryT>
		static void create_entry(EntryT *ent) {
			ent->tqe.next = NULL;
			ent->tqe.prevent = NULL;
		}

		template<typename EntryT>
		static void destroy_entry(EntryT *ent) {
			assert(ent->tqe.next == NULL || ent->tqe.next == (void *)-1);
			assert(ent->tqe.prevent == NULL || ent->tqe.prevent == (void *)-1);
		}

		template<typename HeadT>
		static void check_head(HeadT *head) {
			if (head->tqh.next != NULL) {
				typename HeadT::EntryType *first = HeadT::entry(head->tqh.next);
				assert(first->tqe.prevent == &head->tqh);
			}
		}

		template<typename HeadT>
		static void check_tail(HeadT *head) {
			assert(head->tqh.prevent->next == NULL);
		}

		template<typename EntryT>
		static void check_next(EntryT *ent) {
			if (ent->tqe.next != NULL) {
				EntryT *next = EntryT::entry(ent->tqe.next);
				assert(next->tqe.prevent == &ent->tqe);
			}
		}

		template<typename EntryT>
		static void check_prev(EntryT *ent) {
			assert(ent->tqe.prevent->next == ent);
		}

		template<typename EntryT>
		static void remove_pre(RemoveCtx &ctx, EntryT *ent) {
			ctx.old_next = (void **)&ent->tqe.next;
			ctx.old_prev = (void **)&ent->tqe.prevent;
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
struct TailqPolicy : policy::Tailq::Debug { };
// struct TailqPolicy : policy::Tailq::Default { };

template <typename EntryT>
class TailqHead : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef typename EntryType::ObjectType ObjectType;
	typedef typename EntryType::Policy Policy;

	friend struct policy::Tailq;

	TailqHead() {
		init();
	}

	bool empty() const {
		return (tqh.next == NULL);
	}

	ObjectType *first() {
		return tqh.next;
	}

	const ObjectType *first() const {
		return tqh.next;
	}

	ObjectType *last() {
		return tqh.prevent->prevent->next;
	}

	const ObjectType *last() const {
		return tqh.prevent->prevent->next;
	}

	void concat(TailqHead *nhead) {
		if (!nhead->empty()) {
			this->tqh.prevent->next = nhead->tqh.next;
			entry(nhead->tqh.next)->tqe.prevent = this->tqh.prevent;
			this->tqh.prevent = nhead->tqh.prevent;
			nhead->init();
		}
	}

	void insert_after(ObjectType *listobj, ObjectType *obj) {
		Policy::check_next(entry(listobj));

		if ((entry(obj)->tqe.next = entry(listobj)->tqe.next) != NULL) {
			entry(entry(obj)->tqe.next)->tqe.prevent =
			    &entry(obj)->tqe;
		} else {
			tqh.prevent = &entry(obj)->tqe;
		}
		entry(listobj)->tqe.next = obj;
		entry(obj)->tqe.prevent = &entry(listobj)->tqe;
	}

	void insert_before(ObjectType *listobj, ObjectType *obj) {
		Policy::check_prev(entry(listobj));

		entry(obj)->tqe.prevent = entry(listobj)->tqe.prevent;
		entry(obj)->tqe.next = listobj;
		entry(listobj)->tqe.prevent->next = obj;
		entry(listobj)->tqe.prevent = &entry(obj)->tqe;
	}

	void insert_head(ObjectType *obj) {
		Policy::check_head(this);

		if ((entry(obj)->tqe.next = tqh.next) != NULL)
			entry(tqh.next)->tqe.prevent =
			    &entry(obj)->tqe;
		else
			tqh.prevent = &entry(obj)->tqe;
		tqh.next = obj;
		entry(obj)->tqe.prevent = &tqh;
	}

	void insert_tail(ObjectType *obj) {
		Policy::check_tail(this);

		entry(obj)->tqe.next = NULL;
		entry(obj)->tqe.prevent = tqh.prevent;
		tqh.prevent->next = obj;
		tqh.prevent = &entry(obj)->tqe;
	}

	void remove(ObjectType *obj) {
		typename Policy::RemoveCtx ctx;

		Policy::remove_pre(ctx, entry(obj));
		Policy::check_next(entry(obj));
		Policy::check_prev(entry(obj));

		if ((entry(obj)->tqe.next) != NULL)
			entry(entry(obj)->tqe.next)->tqe.prevent =
			    entry(obj)->tqe.prevent;
		else
			tqh.prevent = entry(obj)->tqe.prevent;
		entry(obj)->tqe.prevent->next = entry(obj)->tqe.next;

		Policy::remove_post(ctx, entry(obj));
	}

	void swap(TailqHead *nhead) {
		ObjectType *swap_first = this->tqh.next;
		typename EntryType::Data *swap_last = this->tqh.prevent;

		this->tqh.next = nhead->tqh.next;
		this->tqh.prevent = nhead->tqh.prevent;
		nhead->tqh.next = swap_first;
		nhead->tqh.prevent = swap_last;
		if ((swap_first = this->tqh.next) != NULL)
			entry(swap_first)->tqe.prevent = &this->tqh;
		else
			this->tqh.prevent = &this->tqh;
		if ((swap_first = nhead->tqh.next) != NULL)
			entry(swap_first)->tqe.prevent = &nhead->tqh;
		else
			nhead->tqh.prevent = &nhead->tqh;
	}

protected:
	inline void init() {
		tqh.next = NULL;
		tqh.prevent = &tqh;
	}

	static EntryType *entry(ObjectType *obj) {
		return EntryType::entry(obj);
	}

private:
	typename EntryType::Data tqh;
};

template <typename EntryT, typename ObjectT>
class TailqEntry : impl::NonCopyable {
public:
	typedef EntryT EntryType;
	typedef ObjectT ObjectType;
	typedef TailqPolicy<EntryType> Policy;

	friend class TailqHead<EntryType>;
	friend class impl::Iterator<EntryType>;
	friend class impl::ConstIterator<EntryType>;
	friend class impl::ReverseIterator<EntryType>;
	friend class impl::ConstReverseIterator<EntryType>;
	friend struct policy::Tailq;

	struct Iterator : impl::Iterator<EntryType> {
		typedef impl::Iterator<EntryType> Base;
		ObjectType *init(TailqHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ConstIterator : impl::ConstIterator<EntryType> {
		typedef impl::ConstIterator<EntryType> Base;
		const ObjectType *init(const TailqHead<EntryType> *head) {
			return Base::start_at(head->first());
		}
	};

	struct ReverseIterator : impl::ReverseIterator<EntryType> {
		typedef impl::ReverseIterator<EntryType> Base;
		ObjectType *init(TailqHead<EntryType> *head) {
			return Base::start_at(head->last());
		}
	};

	struct ConstReverseIterator : impl::ConstReverseIterator<EntryType> {
		typedef impl::ConstReverseIterator<EntryType> Base;
		const ObjectType *init(const TailqHead<EntryType> *head) {
			return Base::start_at(head->last());
		}
	};

	TailqEntry() {
		Policy::create_entry(this);
	}

	~TailqEntry() {
		Policy::destroy_entry(this);
	}

	ObjectType *next() {
		return this->tqe.next;
	}

	const ObjectType *next() const {
		return this->tqe.next;
	}

	ObjectType *prev() {
		return tqe.prevent->prevent->next;
	}

	const ObjectType *prev() const {
		return tqe.prevent->prevent->next;
	}

protected:
	struct Data {
		ObjectType *next;
		Data *prevent;
	};

	Data tqe;

	static EntryType *entry(ObjectType *obj) {
		return obj;
	}

	static const EntryType *entry(const ObjectType *obj) {
		return obj;
	}

	ObjectType *object() {
		return impl::entry_to_object<ObjectType, TailqEntry>(this);
	}
};

} // namespace ecl

#endif
