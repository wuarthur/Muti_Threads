#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

struct Pool {
  // TODO
  uthread_sem_t lock;
  uthread_sem_t count_from_empty;
  uthread_sem_t count_from_full;
  int           items;
};

struct Pool* createPool() {
  struct Pool* pool = malloc (sizeof (struct Pool));
  // TODO
  pool->lock = uthread_sem_create(1);
  pool->count_from_empty = uthread_sem_create(0);
  pool->count_from_full = uthread_sem_create(MAX_ITEMS);
  pool->items     = 0;
  return pool;
};

void* producer (void* pv) {
  struct Pool* p = pv;

  // TODO
  for (int i=0; i < NUM_ITERATIONS; i++) {
  	uthread_sem_wait(p->count_from_full);
  	uthread_sem_wait(p->lock);
	p->items++;
	histogram[p->items]++;
	assert(p->items <= MAX_ITEMS);
	uthread_sem_signal(p->lock);
	uthread_sem_signal(p->count_from_empty);
	}
  return NULL;
}

void* consumer (void* pv) {
  struct Pool* p = pv;

  // TODO
  for (int i=0; i < NUM_ITERATIONS; i++) {
  	uthread_sem_wait(p->count_from_empty);
  	uthread_sem_wait(p->lock);
	p->items--;
	histogram[p->items]++;
	assert(p->items >= 0);
	uthread_sem_signal(p->lock);
	uthread_sem_signal(p->count_from_full);
	}
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];

  // TODO

  uthread_init (4);
  uthread_t producers[NUM_PRODUCERS];
  uthread_t consumers[NUM_CONSUMERS];
  struct Pool* p = createPool();

  for (int i = 0; i < NUM_PRODUCERS; i++) {
	consumers[i] = uthread_create(consumer, p);
	producers[i] = uthread_create(producer, p);
	}

  for (int i = 0; i < NUM_PRODUCERS; i++) {
	uthread_join(consumers[i], 0);
	uthread_join(producers[i], 0);
	}

  
  
  
  printf ("producer_wait_count=%d, consumer_wait_count=%d\n", 	producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}