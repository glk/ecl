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

#ifndef ECL_IMPL_HPP
#define ECL_IMPL_HPP

#include <cassert>

namespace ecl {
namespace impl {

class NonCopyable
{
protected:
	NonCopyable() {}
	~NonCopyable() {}

private:
	NonCopyable(const NonCopyable&);
	const NonCopyable& operator=(const NonCopyable&);
};

template<typename EntryType>
class Iterator : NonCopyable {
public:
	typedef typename EntryType::ObjectType ObjectType;

	ObjectType *next() {
		ObjectType *obj = it_next;
		if (obj != NULL)
			it_next = EntryType::entry(obj)->next();
		return obj;
	}

	ObjectType *start_at(ObjectType *obj) {
		if (obj != NULL)
			it_next = EntryType::entry(obj)->next();
		else
			it_next = NULL;
		return obj;
	}

protected:
	ObjectType *it_next;
};

template<typename EntryType>
class ConstIterator : NonCopyable {
public:
	typedef typename EntryType::ObjectType ObjectType;

	const ObjectType *next() {
		const ObjectType *obj = it_next;
		if (obj != NULL)
			it_next = EntryType::entry(obj)->next();
		return obj;
	}

	const ObjectType *start_at(const ObjectType *obj) {
		if (obj != NULL)
			it_next = EntryType::entry(obj)->next();
		else
			it_next = NULL;
		return obj;
	}

protected:
	const ObjectType *it_next;
};

template<typename EntryType>
class ReverseIterator : NonCopyable {
public:
	typedef typename EntryType::ObjectType ObjectType;

	ObjectType *prev() {
		ObjectType *obj = rit_prev;
		if (obj != NULL)
			rit_prev = EntryType::entry(obj)->prev();
		return obj;
	}

	ObjectType *start_at(ObjectType *obj) {
		if (obj != NULL)
			rit_prev = EntryType::entry(obj)->prev();
		else
			rit_prev = NULL;
		return obj;
	}

protected:
	ObjectType *rit_prev;
};

template<typename EntryType>
class ConstReverseIterator : NonCopyable {
public:
	typedef typename EntryType::ObjectType ObjectType;

	const ObjectType *prev() {
		const ObjectType *obj = rit_prev;
		if (obj != NULL)
			rit_prev = EntryType::entry(obj)->prev();
		return obj;
	}

	const ObjectType *start_at(const ObjectType *obj) {
		if (obj != NULL)
			rit_prev = EntryType::entry(obj)->prev();
		else
			rit_prev = NULL;
		return obj;
	}

protected:
	const ObjectType *rit_prev;
};

template<typename ObjectType, typename EntryImpl>
ObjectType *entry_to_object(const EntryImpl *ent) {
	// Use non NULL value
	ObjectType *fake_val = (ObjectType *)ent;
	EntryImpl *fake_ent = fake_val;
	uintptr_t offset = reinterpret_cast<uintptr_t>(fake_ent) -
	    reinterpret_cast<uintptr_t>(fake_val);
	fake_val = reinterpret_cast<ObjectType *>(
	    reinterpret_cast<uintptr_t>(ent) - offset);
	return fake_val;
}

} // namespace impl

namespace policy {

	struct Generic {
		struct RemoveCtx { };

		template<typename HeadT>
		static void create_head(HeadT *head) { }

		template<typename HeadT>
		static void destroy_head(HeadT *head) { }

		template<typename EntryT>
		static void create_entry(EntryT *ent) { }

		template<typename EntryT>
		static void destroy_entry(EntryT *ent) { }

		template<typename HeadT>
		static void check_head(HeadT *head) { }

		template<typename HeadT>
		static void check_tail(HeadT *head) { }

		template<typename EntryT>
		static void check_next(EntryT *ent) { }

		template<typename EntryT>
		static void check_prev(EntryT *ent) { }

		template<typename EntryT>
		static void remove_pre(RemoveCtx &ctx, EntryT *ent) { }

		template<typename EntryT>
		static void remove_post(RemoveCtx &ctx, EntryT *ent) { }
	};

} // namespace policy

} // namespace ecl

#endif
