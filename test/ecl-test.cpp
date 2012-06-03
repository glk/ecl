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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include "ecl/slist.hpp"
#include "ecl/stailq.hpp"
#include "ecl/list.hpp"
#include "ecl/tailq.hpp"
#include "ecl/rbtree.hpp"

// {{{ genetric

template<typename HeadT1, typename HeadT2, typename EntryT>
void test_basic_init_generic(HeadT1 &q1, HeadT2 &q2, EntryT *s[], int n)
{
	/* Fix cimpilation with gcc */
	typedef typename EntryT::list1 list1;
	typedef typename EntryT::list2 list2;

	EntryT *si;
	int i;

	for (i = 0; i < n; i++) {
		q1.insert_head(s[i]);
	}

	si = q1.first();
	for (i = n - 1; i >= 0; i--, si = si->list1::next()) {
		q2.insert_head(s[i]);
		assert(si != NULL);
		assert(si->generation() == i + 1);
	}

	si = q2.first();
	for (i = 0; i < n; i++, si = si->list2::next()) {
		assert(si != NULL);
		assert(si->generation() == i + 1);
	}
}

// }}}

class ValList; // {{{

struct ValList_Entry1 : ecl::ListEntry<ValList_Entry1, ValList> { };
struct ValList_Entry2 : ecl::ListEntry<ValList_Entry2, ValList> { };

typedef ecl::ListHead<ValList_Entry1> HeadList1;
typedef ecl::ListHead<ValList_Entry2> HeadList2;

extern template class ecl::ListEntry<ValList_Entry1, ValList>;
extern template class ecl::ListHead<ValList_Entry1>;
extern template class ecl::ListHead<ValList_Entry2>;

class ValList : public ValList_Entry1, public ValList_Entry2 {
public:
	typedef ValList_Entry1 list1;
	typedef ValList_Entry2 list2;

	ValList(int gen_) : gen(gen_) { }

	int generation() const {
		return gen;
	}

private:
	int gen;
};

void test_basic_list(int n)
{
	ValList **s, *si, *s1, *s2;
	const ValList *sc;
	HeadList1 q1, q1a;
	HeadList2 q2;
	const HeadList1 *q1c = &q1;
	int i;

	s = new ValList*[n];
	for (i = 0; i < n; i++)
		s[i] = new ValList(i + 1);

	test_basic_init_generic(q1, q2, s, n);

	for (i = 0; i < n; i+=2) {
		s[i]->list1::remove();
		s[i]->list2::remove();
	}
	for (i = 1; i < n; i+=2) {
		s[i]->list1::remove();
		s[i]->list2::remove();
	}
	assert(q1.empty());
	assert(q2.empty());

	q1.insert_head(s[0]);
	assert(s[0]->list1::next() == NULL);
	q2.insert_head(s[0]);
	assert(s[0]->list2::next() == NULL);
	for (i = 1; i < n; i++) {
		s[i]->list1::insert_before(s[0]);
		assert(s[0]->list1::next() == NULL);
		s[i]->list2::insert_after(s[0]);
		assert(s[0]->list2::next() == s[i]);
	}

	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 == NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 == NULL);

	q1a.insert_head(new ValList(n + 2));
	q1a.insert_head(new ValList(n + 3));
	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 != NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 != NULL);

	while (!q1a.empty()) {
		si = q1a.first();
		si->list1::remove();
		assert(si->generation() > n + 1);
		delete si;
	}

	for (i = 0, sc = q1c->first(); sc != NULL; sc = sc->list1::next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);
	ValList::list1::ConstIterator cit;
	for (i = 0, sc = cit.init(q1c); sc != NULL; sc = cit.next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);

	ValList::list1::Iterator it1;
	for (si = it1.init(&q1); si != NULL; si = it1.next())
		si->list1::remove();
	assert(q1.empty());
	ValList::list2::Iterator it2;
	for (si = it2.start_at(q2.first()); si != NULL; si = it2.next())
		si->list2::remove();
	assert(q2.empty());

	for (i = 0; i < n; i++)
		delete s[i];
	delete[] s;
}

template class ecl::ListEntry<ValList_Entry1, ValList>;
template class ecl::ListHead<ValList_Entry1>;
template class ecl::ListHead<ValList_Entry2>;

// }}}

class ValSList; // {{{

struct ValSList_Entry1 : ecl::SListEntry<ValSList_Entry1, ValSList> { };
struct ValSList_Entry2 : ecl::SListEntry<ValSList_Entry2, ValSList> { };

typedef ecl::SListHead<ValSList_Entry1> HeadSList1;
typedef ecl::SListHead<ValSList_Entry2> HeadSList2;

extern template class ecl::SListEntry<ValSList_Entry1, ValSList>;
extern template class ecl::SListHead<ValSList_Entry1>;
extern template class ecl::SListHead<ValSList_Entry2>;

class ValSList : public ValSList_Entry1, public ValSList_Entry2 {
public:
	typedef ValSList_Entry1 list1;
	typedef ValSList_Entry2 list2;

	ValSList(int gen_) : gen(gen_) { }

	int generation() const {
		return gen;
	}

private:
	int gen;
};

void test_basic_slist(int n)
{
	ValSList **s, *si, *sprev, *s1, *s2;
	const ValSList *sc;
	HeadSList1 q1, q1a;
	HeadSList2 q2;
	const HeadSList1 *q1c = &q1;
	int i;

	s = new ValSList*[n];
	for (i = 0; i < n; i++)
		s[i] = new ValSList(i + 1);

	test_basic_init_generic(q1, q2, s, n);

	while (!q1.empty())
		q1.remove_head();
	assert(q1.empty());
	for (i = 0; i < n; i+=2)
		q2.remove(s[i]);
	for (i = 1; i < n; i+=2)
		q2.remove(s[i]);
	assert(q2.empty());

	q1.insert_head(s[0]);
	assert(s[0]->list1::next() == NULL);
	for (i = 1; i < n; i++) {
		s[i]->list1::insert_after(s[i - 1]);
		assert(s[i - 1]->list1::next() == s[i]);
	}

	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 == NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 == NULL);

	q1a.insert_head(new ValSList(n + 2));
	q1a.insert_head(new ValSList(n + 3));
	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 != NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 != NULL);

	while (!q1a.empty()) {
		si = q1a.first();
		q1a.remove(si);
		assert(si->generation() > n + 1);
		delete si;
	}

	for (i = 0, sc = q1c->first(); sc != NULL; sc = sc->list1::next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);
	ValSList::list1::ConstIterator cit;
	for (i = 0, sc = cit.init(q1c); sc != NULL; sc = cit.next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);

	ValSList::list1::Iterator it;
	for (si = it.init(&q1), sprev = NULL; si != NULL; si = it.next()) {
		if (sprev != NULL) {
			ValSList *snext = si->list1::next();
			assert(sprev->list1::next() == si);
			sprev->list1::remove_after();
			assert(sprev->list1::next() == snext);
		} else
			sprev = si;
	}
	assert(sprev->list1::next() == NULL);
	assert(q1.first() == sprev);
	q1.remove_head();
	assert(q1.empty());

	for (i = 0; i < n; i++)
		delete s[i];
	delete[] s;
}

template class ecl::SListEntry<ValSList_Entry1, ValSList>;
template class ecl::SListHead<ValSList_Entry1>;
template class ecl::SListHead<ValSList_Entry2>;

// }}}

class ValTailq; // {{{

struct ValTailq_Entry1 : ecl::TailqEntry<ValTailq_Entry1, ValTailq> { };
struct ValTailq_Entry2 : ecl::TailqEntry<ValTailq_Entry2, ValTailq> { };

typedef ecl::TailqHead<ValTailq_Entry1> HeadTailq1;
typedef ecl::TailqHead<ValTailq_Entry2> HeadTailq2;

extern template class ecl::TailqEntry<ValTailq_Entry1, ValTailq>;
extern template class ecl::TailqHead<ValTailq_Entry1>;
extern template class ecl::TailqHead<ValTailq_Entry2>;

class ValTailq : public ValTailq_Entry1, public ValTailq_Entry2 {
public:
	typedef ValTailq_Entry1 list1;
	typedef ValTailq_Entry2 list2;

	ValTailq(int gen_) : gen(gen_) { }

	int generation() const {
		return gen;
	}

private:
	int gen;
};

void test_basic_tailq(int n)
{
	ValTailq **s, *si, *sprev, *snext, *s1, *s2;
	const ValTailq *sc;
	HeadTailq1 q1, q1a;
	HeadTailq2 q2;
	const HeadTailq1 *q1c = &q1;
	int i;

	s = new ValTailq*[n];
	for (i = 0; i < n; i++)
		s[i] = new ValTailq(i + 1);

	test_basic_init_generic(q1, q2, s, n);

	si = q1.first();
	assert(si != NULL);
	sprev = si->list1::prev();
	assert(sprev == NULL);
	if (n > 1)
		assert(si->list1::next() != NULL);
	for (sprev = si, si = si->list1::next(); si != NULL;
	    si = si->list1::next()) {
		assert(si->list1::prev() == sprev);
		sprev = si;
	}

	si = q2.last();
	assert(si != NULL);
	snext = si->list2::next();
	assert(snext == NULL);
	if (n > 1)
		assert(si->list2::prev() != NULL);
	for (snext = si, si = si->list2::prev(); si != NULL;
	    si = si->list2::prev()) {
		assert(si->list2::next() == snext);
		snext = si;
	}

	for (i = 0; i < n; i+=2) {
		q1.remove(s[i]);
		q2.remove(s[i]);
	}
	for (i = 1; i < n; i+=2) {
		q1.remove(s[i]);
		q2.remove(s[i]);
	}
	assert(q1.empty());
	assert(q2.empty());

	for (i = 0; i < n; i++)
		q1.insert_tail(s[i]);
	for (i = 0; i < n; i+=2)
		q1.remove(s[i]);
	for (i = 1; i < n; i+=2)
		q1.remove(s[i]);
	assert(q1.empty());
	assert(q2.empty());

	q1.insert_head(s[0]);
	assert(s[0]->list1::next() == NULL);
	assert(s[0]->list1::prev() == NULL);
	q2.insert_head(s[0]);
	assert(s[0]->list2::next() == NULL);
	assert(s[0]->list2::prev() == NULL);
	for (i = 1; i < n; i++) {
		q1.insert_before(s[0], s[i]);
		assert(s[0]->list1::prev() == s[i]);
		q2.insert_after(s[0], s[i]);
		assert(s[0]->list2::next() == s[i]);
	}

	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 == NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 == NULL);

	q1a.insert_head(new ValTailq(n + 2));
	q1a.insert_head(new ValTailq(n + 3));
	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 != NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 != NULL);

	q1.concat(&q1a);
	assert(q1a.empty());
	assert(!q1.empty());
	assert(q1.last()->generation() > n + 1);
	while ((si = q1.last())->generation() > n + 1) {
		q1.remove(si);
		delete si;
	}
	s1 = q1.first();
	s2 = q1.last();
	q1a.concat(&q1);
	assert(q1.empty());
	q1.concat(&q1a);
	assert(q1a.empty());
	assert(s1 == q1.first());
	assert(s2 == q1.last());

	for (i = 0, sc = q1c->first(); sc != NULL; sc = sc->list1::next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);
	ValTailq::list1::ConstIterator cit;
	for (i = 0, sc = cit.init(q1c); sc != NULL; sc = cit.next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);
	ValTailq::list1::ConstReverseIterator crit;
	for (i = 0, sc = crit.init(q1c); sc != NULL; sc = crit.prev())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);

	ValTailq::list1::Iterator it;
	for (si = it.init(&q1); si != NULL; si = it.next())
		q1.remove(si);
	assert(q1.empty());

	ValTailq::list2::ReverseIterator rit;
	for (si = rit.start_at(q2.last()); si != NULL; si = rit.prev())
		q2.remove(si);
	assert(q2.empty());

	for (i = 0; i < n; i++)
		delete s[i];
	delete[] s;
}

template class ecl::TailqEntry<ValTailq_Entry1, ValTailq>;
template class ecl::TailqHead<ValTailq_Entry1>;
template class ecl::TailqHead<ValTailq_Entry2>;

// }}}

class ValSTailq; // {{{

struct ValSTailq_Entry1 : ecl::STailqEntry<ValSTailq_Entry1, ValSTailq> { };
struct ValSTailq_Entry2 : ecl::STailqEntry<ValSTailq_Entry2, ValSTailq> { };

typedef ecl::STailqHead<ValSTailq_Entry1> HeadSTailq1;
typedef ecl::STailqHead<ValSTailq_Entry2> HeadSTailq2;

extern template class ecl::STailqEntry<ValSTailq_Entry1, ValSTailq>;
extern template class ecl::STailqHead<ValSTailq_Entry1>;
extern template class ecl::STailqHead<ValSTailq_Entry2>;

class ValSTailq : public ValSTailq_Entry1, public ValSTailq_Entry2 {
public:
	typedef ValSTailq_Entry1 list1;
	typedef ValSTailq_Entry2 list2;

	ValSTailq(int gen_) : gen(gen_) { }

	int generation() const {
		return gen;
	}

private:
	int gen;
};

void test_basic_stailq(int n)
{
	ValSTailq **s, *si, *sprev, *s1, *s2;
	const ValSTailq *sc;
	HeadSTailq1 q1, q1a;
	HeadSTailq2 q2;
	const HeadSTailq1 *q1c = &q1;
	int i;

	s = new ValSTailq*[n];
	for (i = 0; i < n; i++)
		s[i] = new ValSTailq(i + 1);

	test_basic_init_generic(q1, q2, s, n);

	while (!q1.empty())
		q1.remove_head();
	assert(q1.empty());
	for (i = 0; i < n; i+=2)
		q2.remove(s[i]);
	for (i = 1; i < n; i+=2)
		q2.remove(s[i]);
	assert(q2.empty());

	q1.insert_head(s[0]);
	assert(q1.first() == s[0]);
	assert(s[0]->list1::next() == NULL);
	for (i = 1; i < n; i++) {
		q1.insert_after(s[i - 1], s[i]);
		assert(s[i - 1]->list1::next() == s[i]);
	}

	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 == NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 == NULL);

	q1a.insert_head(new ValSTailq(n + 2));
	q1a.insert_head(new ValSTailq(n + 3));
	s1 = q1.first();
	s2 = q1a.first();
	assert(s1 != s2);
	assert(s2 != NULL);
	q1.swap(&q1a);
	assert(s2 == q1.first());
	assert(s1 == q1a.first());
	q1.swap(&q1a);
	assert(s1 == q1.first());
	assert(s2 == q1a.first());
	assert(s2 != NULL);

	q1.concat(&q1a);
	assert(q1a.empty());
	assert(!q1.empty());
	assert(q1.last()->generation() > n + 1);
	while ((si = q1.last())->generation() > n + 1) {
		q1.remove(si);
		delete si;
	}
	s1 = q1.first();
	s2 = q1.last();
	q1a.concat(&q1);
	assert(q1.empty());
	q1.concat(&q1a);
	assert(q1a.empty());
	assert(s1 == q1.first());
	assert(s2 == q1.last());

	for (i = 0, sc = q1c->first(); sc != NULL; sc = sc->list1::next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);
	ValSTailq::list1::ConstIterator cit;
	for (i = 0, sc = cit.init(q1c); sc != NULL; sc = cit.next())
		i += sc->generation();
	assert(i == n * (n + 1) / 2);

	ValSTailq::list1::Iterator it;
	for (si = it.init(&q1), sprev = NULL; si != NULL; si = it.next()) {
		if (sprev != NULL) {
			ValSTailq *snext = si->list1::next();
			assert(sprev->list1::next() == si);
			q1.remove_after(sprev);
			assert(sprev->list1::next() == snext);
		} else
			sprev = si;
	}
	assert(sprev->list1::next() == NULL);
	assert(q1.first() == sprev);
	q1.remove_head();
	assert(q1.empty());

	for (i = 0; i < n; i++)
		delete s[i];
	delete[] s;
}

template class ecl::STailqEntry<ValSTailq_Entry1, ValSTailq>;
template class ecl::STailqHead<ValSTailq_Entry1>;
template class ecl::STailqHead<ValSTailq_Entry2>;

// }}}

class ValRBTree; // {{{

struct ValRBTree_Entry1 : ecl::RBTreeEntry<ValRBTree_Entry1, ValRBTree> {
	template<typename T>
	static int compare_fn(const T *a, const T *b) {
		return compare_key_fn(a->gen, b);
	}

	template<typename T>
	static int compare_key_fn(int key, const T *obj) {
		if (key > obj->gen)
			return 1;
		else if (key < obj->gen)
			return -1;
		return 0;
	}
};

struct ValRBTree_Entry2 : ecl::RBTreeEntry<ValRBTree_Entry2, ValRBTree> {
	template<typename T>
	static int compare_fn(const T *a, const T *b) {
		return compare_key_fn(a->gen, b);
	}

	template<typename T>
	static int compare_key_fn(int key, const T *obj) {
		if (key > obj->gen)
			return -1;
		else if (key < obj->gen)
			return 1;
		return 0;
	}
};

typedef ecl::RBTreeHead<ValRBTree_Entry1> HeadRBTree1;
typedef ecl::RBTreeHead<ValRBTree_Entry2> HeadRBTree2;

extern template class ecl::RBTreeEntry<ValRBTree_Entry1, ValRBTree>;
extern template class ecl::RBTreeHead<ValRBTree_Entry1>;
extern template class ecl::RBTreeHead<ValRBTree_Entry2>;

class ValRBTree : public ValRBTree_Entry1, public ValRBTree_Entry2 {
public:
	typedef ValRBTree_Entry1 list1;
	typedef ValRBTree_Entry2 list2;

	friend struct ValRBTree_Entry1;
	friend struct ValRBTree_Entry2;

	ValRBTree(int gen_) : gen(gen_) { }

	int generation() const {
		return gen;
	}

private:
	int gen;
};

void test_basic_rbtree(int n)
{
	ValRBTree **s, *si, *sprev, *snext;
	const ValRBTree *sc;
	HeadRBTree1 q1;
	HeadRBTree2 q2;
	const HeadRBTree1 *q1c = &q1;
	int i;

	s = new ValRBTree*[n];
	for (i = 0; i < n; i++)
		s[i] = new ValRBTree(i);

	for (i = 0; i < n; i+=2) {
		q1.insert(s[i]);
		q2.insert(s[i]);
	}
	for (i = 1; i < n; i+=2) {
		q1.insert(s[i]);
		q2.insert(s[i]);
	}

	assert(q1.min() == s[0]);
	assert(q1.max() == s[n - 1]);
	assert(q2.min() == s[n - 1]);
	assert(q2.max() == s[0]);

	for (si = q1.first(), i = 0; si != NULL; si = si->list1::next(), i++) {
		assert(i < n);
		assert(si == s[i]);
		if (i > 0)
			assert(si->list1::prev() == s[i - 1]);
		if (i + 1 < n)
			assert(si->list1::next() == s[i + 1]);
		else
			assert(si->list1::next() == NULL);
	}
	assert(i == n);

	ValRBTree::list2::ReverseIterator rit;
	for (si = rit.init(&q2), i = 0; si != NULL; si = rit.prev(), i++) {
		assert(si == s[i]);
	}

	for (i = 0; i < n; i++) {
		assert(q1.find(i) == s[i]);
		assert(q2.find(i) == s[i]);
	}

	for (i = 0, sc = q1c->first(); sc != NULL; sc = sc->list1::next())
		i += sc->generation();
	assert(i == n * (n - 1) / 2); // start at 0
	ValRBTree::list1::ConstIterator cit;
	for (i = 0, sc = cit.init(q1c); sc != NULL; sc = cit.next())
		i += sc->generation();
	assert(i == n * (n - 1) / 2); // start at 0
	ValRBTree::list1::ConstReverseIterator crit;
	for (i = 0, sc = crit.init(q1c); sc != NULL; sc = crit.prev())
		i += sc->generation();
	assert(i == n * (n - 1) / 2); // start at 0

	sc = q1c->find(n / 2);
	assert(sc == s[n / 2]);

	for (i = 0; i < n; i+=2) {
		q1.remove(s[i]);
		q2.remove(s[i]);
	}
	for (i = 1; i < n; i+=2) {
		q1.remove(s[i]);
		q2.remove(s[i]);
	}
	assert(q1.empty());
	assert(q2.empty());

	for (i = 0; i < n; i += 2) {
		q1.insert(s[i]);
		q2.insert(s[i]);
	}

	assert(q1.min() == s[0]);
	assert(q1.max() == s[n - 2 + (n % 2)]);
	assert(q2.min() == s[n - 2 + (n % 2)]);
	assert(q2.max() == s[0]);

	for (i = 0; i < n; i++) {
		snext = q1.nfind(i);
		sprev = q1.pfind(i);
		if (i == 0) {
			assert(snext == s[i + (i % 2)]);
			assert(sprev == s[0]);
		} else if (i == n - 1) {
			if ((n - 1) % 2 == 0)
				assert(snext == s[i]);
			else
				assert(snext == NULL);
			assert(sprev == s[i - (i % 2)]);
		} else {
			assert(snext == s[i + (i % 2)]);
			assert(sprev == s[i - (i % 2)]);
		}
	}

	while (!q1.empty())
		q1.remove(q1.root());

	ValRBTree::list2::Iterator it;
	for (si = it.init(&q2); si != NULL; si = it.next())
		q2.remove(si);
	assert(q2.empty());

	for (i = 0; i < n; i++)
		delete s[i];
	delete[] s;
}

template class ecl::RBTreeEntry<ValRBTree_Entry1, ValRBTree>;
template class ecl::RBTreeHead<ValRBTree_Entry1>;
template class ecl::RBTreeHead<ValRBTree_Entry2>;

// }}}

int main()
{
	const int n = 5000;

	test_basic_list(n);

	test_basic_tailq(n);

	test_basic_slist(n);

	test_basic_stailq(n);

	test_basic_rbtree(1001);

	test_basic_rbtree(n);

	return (0);
}
