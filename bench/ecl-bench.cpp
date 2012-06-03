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
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <map>

#include "ecl/list.hpp"
#include "ecl/tailq.hpp"
#include "ecl/rbtree.hpp"

class DataTailq;
class DataTree;

struct DataTailqEntry : ecl::TailqEntry<DataTailqEntry, DataTailq> { };
typedef ecl::TailqHead<DataTailqEntry> DataTailqHead;

struct DataTreeEntry : ecl::RBTreeEntry<DataTreeEntry, DataTree> {
	template<typename T>
	static int compare_fn(T *a, T *b) {
		return compare_key_fn(a->gen, b);
	}

	template<typename T>
	static int compare_key_fn(int key, T *obj) {
		if (key > obj->gen)
			return 1;
		else if (key < obj->gen)
			return -1;
		return 0;
	}
};
typedef ecl::RBTreeHead<DataTreeEntry> DataTreeHead;

static int g_gen;

class DataTailq : public DataTailqEntry {
public:
	typedef DataTailqEntry list;

	DataTailq() : gen(g_gen++) { }

	int generation() {
		return gen;
	}

private:
	int gen;
	char dummy[26];
};

class DataTree : public DataTreeEntry {
public:
	typedef DataTreeEntry tree;

	friend struct DataTreeEntry;

	DataTree(int a) : gen(a) { }

	int generation() {
		return gen;
	}

private:
	int gen;
	char dummy[26];
};

class DataPlain {
public:
	DataPlain() : gen(g_gen++) { }

	DataPlain(int a) : gen(a) { }

	int generation() {
		return gen;
	}

private:
	int gen;
	char dummy[26];
};

static void
benchmark_result(const char *name, intmax_t n,
    struct timeval *tstart, struct timeval *tend)
{
	double t;

	t = tend->tv_sec - tstart->tv_sec;
	t += (double)(tend->tv_usec - tstart->tv_usec) / (double)1000000;
	printf("%s: %jd iterations in %lf seconds; %lf iterations/s\n",
	    name, n, t, (double)n/t);
}

static void
test_add_remove_ecl(int n)
{
	struct timeval tstart, tend;
	DataTailqHead head;
	DataTailq val;
	int i;

	gettimeofday(&tstart, NULL);

	for (i = 0; i < n; i++) {
		head.insert_head(&val);
		head.remove(head.first());
	}

	gettimeofday(&tend, NULL);

	benchmark_result("ecl: add-remove 1 entry", n, &tstart, &tend);
}

static void
test_add_remove_stl(int n)
{
	struct timeval tstart, tend;
	std::list<DataPlain *> head;
	DataPlain val;
	int i;

	gettimeofday(&tstart, NULL);

	for (i = 0; i < n; i++) {
		head.push_back(&val);
		head.remove(&val);
	}

	gettimeofday(&tend, NULL);

	benchmark_result("stl: add-remove 1 entry", n, &tstart, &tend);
}

static void
test_iterate_ecl(int nelem, int niter)
{
	struct timeval tstart, tend;
	DataTailqHead head;
	DataTailq *d;
	int i, j, x;

	for (i = 0; i < nelem; i++) {
		d = new DataTailq();
		head.insert_head(d);
	}

	gettimeofday(&tstart, NULL);

	for (j = 0; j < niter; j++) {
		x = 0;
		for (d = head.first(); d != NULL; d = d->next()) {
			x += d->generation();
		}
	}

	gettimeofday(&tend, NULL);

	while (!head.empty()) {
		d = head.first();
		head.remove(d);
		delete d;
	}

	benchmark_result("ecl: iterate", niter * nelem, &tstart, &tend);
}

static void
test_iterate_stl(int nelem, int niter)
{
	struct timeval tstart, tend;
	std::list<DataPlain *> head;
	std::list<DataPlain *>::iterator it, e;
	DataPlain *d;
	int i, j, x;

	for (i = 0; i < nelem; i++) {
		d = new DataPlain();
		head.push_front(d);
	}

	gettimeofday(&tstart, NULL);

	for (j = 0; j < niter; j++) {
		x = 0;
		for (it = head.begin(), e = head.end(); it != e; ++it) {
			x += d->generation();
		}
	}

	gettimeofday(&tend, NULL);

	while ((it = head.begin()) != head.end()) {
		d = *it;
		head.erase(it);
		delete d;
	}

	benchmark_result("stl: iterate", niter * nelem, &tstart, &tend);
}

static void
test_map_add_remove_ecl(int *keys, int nelem, int niter)
{
	struct timeval tstart, tend;
	DataTreeHead head;
	DataTree **buf;
	int i, j;

	buf = new DataTree*[nelem];
	for (i = 0; i < nelem; i++)
		buf[i] = new DataTree(keys[i]);

	gettimeofday(&tstart, NULL);

	for (j = 0; j < niter; j++) {
		for (i = 0; i < nelem; i++)
			head.insert(buf[i]);
		for (i = 0; i < nelem; i++)
			head.remove(buf[i]);
	}

	gettimeofday(&tend, NULL);

	assert(head.empty());

	for (i = 0; i < nelem; i++)
		delete buf[i];
	delete[] buf;

	benchmark_result("ecl: add/remove rbtree", niter * nelem, &tstart, &tend);
}

static void
test_map_add_remove_stl(int *keys, int nelem, int niter)
{
	struct timeval tstart, tend;
	std::map<int, DataPlain *> head;
	std::map<int, DataPlain *>::iterator iter;
	typedef std::map<int, DataPlain *>::value_type mappair;
	DataPlain **buf;
	int i, j;

	buf = new DataPlain*[nelem];
	for (i = 0; i < nelem; i++)
		buf[i] = new DataPlain(keys[i]);

	gettimeofday(&tstart, NULL);

	for (j = 0; j < niter; j++) {
		for (i = 0; i < nelem; i++)
			head.insert(mappair(buf[i]->generation(), buf[i]));
		for (i = 0; i < nelem; i++)
			head.erase(buf[i]->generation());
	}

	gettimeofday(&tend, NULL);

	assert(head.empty());

	for (i = 0; i < nelem; i++)
		delete buf[i];
	delete[] buf;

	benchmark_result("stl: add/remove rbtree", niter * nelem, &tstart, &tend);
}

static void
test_map_iterate_ecl(int *keys, int nelem, int niter)
{
	struct timeval tstart, tend;
	DataTreeHead head;
	DataTree **buf, *d;
	int i, j;

	buf = new DataTree*[nelem];
	for (i = 0; i < nelem; i++) {
		buf[i] = new DataTree(keys[i]);
		head.insert(buf[i]);
	}

	gettimeofday(&tstart, NULL);

	for (j = 0; j < niter; j++) {
		for (i = 0; i < nelem; i += 2) {
			d = head.find(keys[i]);
			if (d == NULL)
				continue;
			if (d->generation() != keys[i])
				abort();
		}
		for (i = 1; i < nelem; i += 2) {
			d = head.find(keys[i]);
			if (d == NULL)
				continue;
			if (d->generation() != keys[i])
				abort();
		}
		/* mostly negative */
		for (i = 0; i < nelem; i++) {
			d = head.find(i);
			if (d == NULL)
				continue;
			if (d->generation() != i)
				abort();
		}
	}

	gettimeofday(&tend, NULL);

	for (i = 0; i < nelem; i++)
		head.remove(buf[i]);

	assert(head.empty());

	for (i = 0; i < nelem; i++)
		delete buf[i];
	delete[] buf;

	benchmark_result("ecl: iterate rbtree", niter * nelem, &tstart, &tend);
}

static void
test_map_iterate_stl(int *keys, int nelem, int niter)
{
	struct timeval tstart, tend;
	std::map<int, DataPlain *> head;
	std::map<int, DataPlain *>::iterator iter;
	typedef std::map<int, DataPlain *>::value_type mappair;
	DataPlain **buf, *d;
	int i, j;

	buf = new DataPlain*[nelem];
	for (i = 0; i < nelem; i++) {
		buf[i] = new DataPlain(keys[i]);
		head.insert(mappair(buf[i]->generation(), buf[i]));
	}

	gettimeofday(&tstart, NULL);

	for (j = 0; j < niter; j++) {
		for (i = 0; i < nelem; i += 2) {
			iter = head.find(keys[i]);
			if (iter == head.end())
				continue;
			d = iter->second;
			if (d->generation() != keys[i])
				abort();
		}
		for (i = 1; i < nelem; i += 2) {
			iter = head.find(keys[i]);
			if (iter == head.end())
				continue;
			d = iter->second;
			if (d->generation() != keys[i])
				abort();
		}
		/* mostly negative */
		for (i = 0; i < nelem; i++) {
			iter = head.find(i);
			if (iter == head.end())
				continue;
			d = iter->second;
			if (d->generation() != i)
				abort();
		}
	}

	gettimeofday(&tend, NULL);

	head.clear();

	assert(head.empty());

	for (i = 0; i < nelem; i++)
		delete buf[i];
	delete[] buf;

	benchmark_result("stl: iterate rbtree", niter * nelem, &tstart, &tend);
}

static int
key_cmp(const void *xa, const void *xb)
{
	int a = *(int *)xa, b = *(int *)xb;

	if (a > b)
		return 1;
	else if (a < b)
		return -1;
	return 0;
}

static inline int
test_gen_random_key(void)
{
#if defined(__FreeBSD__)
	return (arc4random());
#else
	return (random());
#endif
}

static int *
test_gen_random_keys(int n)
{
	int i, *keys;

	keys = (int *)malloc(n * sizeof(int));
	for (i = 0; i < n; i++)
		keys[i] = test_gen_random_key();
	return keys;
}

static void
test_remove_dup_keys(int *keys, int n)
{
	int i, j, restart, *keys_sorted;

	keys_sorted = (int *)malloc(n * sizeof(int));
	do {
		restart = 0;
		memcpy(keys_sorted, keys, n * sizeof(int));
		qsort(keys_sorted, n, sizeof(int), key_cmp);
		for (i = 0; i < n - 1; i++) {
			if (key_cmp(&keys_sorted[i], &keys_sorted[i + 1]) != 0)
				continue;
			while (i < n - 1 &&
			    key_cmp(&keys_sorted[i + 1], &keys_sorted[i]) == 0)
				i++;
			restart = 1;
			for (j = 0; j < n;  j++) {
				if (key_cmp(&keys[j], &keys_sorted[i]) != 0)
					continue;
				keys[j] = test_gen_random_key();
			}
		}
	} while (restart != 0);
	free(keys_sorted);
}

int main()
{
	int *keys;

	test_add_remove_ecl(100000);
	test_add_remove_stl(100000);
	test_add_remove_ecl(2000000);
	test_add_remove_stl(2000000);

	test_iterate_ecl(100000, 10);
	test_iterate_stl(100000, 10);
	test_iterate_ecl(2000000, 10);
	test_iterate_stl(2000000, 10);

	keys = test_gen_random_keys(200000);
	test_remove_dup_keys(keys, 200000);
	test_map_add_remove_ecl(keys, 10000, 10);
	test_map_add_remove_stl(keys, 10000, 10);
	test_map_add_remove_ecl(keys, 200000, 10);
	test_map_add_remove_stl(keys, 200000, 10);
	test_map_iterate_ecl(keys, 10000, 10);
	test_map_iterate_stl(keys, 10000, 10);
	test_map_iterate_ecl(keys, 200000, 10);
	test_map_iterate_stl(keys, 200000, 10);
	free(keys);

	return (0);
}
