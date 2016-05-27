#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
  int p;
  int t;
  int m; 
  uthread_cond_t paper_ready;
  uthread_cond_t tobacco_ready;
  uthread_cond_t match_ready;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  agent->p = 0;
  agent->t = 0;
  agent->m = 0; 
  agent->paper_ready = uthread_cond_create(agent->mutex);
  agent->tobacco_ready = uthread_cond_create(agent->mutex);
  agent->match_ready = uthread_cond_create(agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//


void* paper_track(void* aa) {
	struct Agent* a = aa;
	uthread_mutex_lock(a->mutex);
	while(1) {
		uthread_cond_wait(a->paper);
		a->p++;
		
		if (a->m > 0) {
			uthread_cond_signal(a->tobacco_ready);
		}
		if (a->t > 0) {
			uthread_cond_signal(a->match_ready);
		}
	}
	uthread_mutex_unlock(a->mutex);
	return NULL;
}

void* tobacco_track(void* aa) {
	struct Agent* a = aa;
	uthread_mutex_lock(a->mutex);
	while(1) {
		uthread_cond_wait(a->tobacco);
		a->t++;
		
		if (a->m > 0) {
			uthread_cond_signal(a->paper_ready);
		}
		if (a->p > 0) {
			uthread_cond_signal(a->match_ready);
		}
	}
	uthread_mutex_unlock(a->mutex);
	return NULL;
}

void* match_track(void* aa) {
	struct Agent* a = aa;
	uthread_mutex_lock(a->mutex);
	while(1) {
		uthread_cond_wait(a->match);
		a->m++;
		
		if (a->p > 0) {
			uthread_cond_signal(a->tobacco_ready);
		}
		if (a->t > 0) {
			uthread_cond_signal(a->paper_ready);
		}
	}
	uthread_mutex_unlock(a->mutex);
	return NULL;
}


/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked


void* paper(void* aa) {
	struct Agent* a = aa;
	uthread_mutex_lock(a->mutex);
	while(1) {
		uthread_cond_wait(a->paper_ready);
		smoke_count[2]++;
		a->t--;
		a->m--;
		uthread_cond_signal(a->smoke);
	}
	uthread_mutex_unlock(a->mutex);
	return NULL;
}

void* tobacco(void* aa) {
	struct Agent* a = aa;
	uthread_mutex_lock(a->mutex);
	while(1) {
		uthread_cond_wait(a->tobacco_ready);
		smoke_count[4]++;
		a->p--;
		a->m--;
		uthread_cond_signal(a->smoke);
	}
	uthread_mutex_unlock(a->mutex);
	return NULL;
}

void* match(void* aa) {
	struct Agent* a = aa;
	uthread_mutex_lock(a->mutex);
	while(1) {
		uthread_cond_wait(a->match_ready);
		smoke_count[1]++;
		a->t--;
		a->p--;
		uthread_cond_signal(a->smoke);
	}
	uthread_mutex_unlock(a->mutex);
	return NULL;
}

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  // TODO
  
	uthread_create(match_track, a);
    uthread_create(paper_track, a);
    uthread_create(tobacco_track, a);
    
    uthread_create(match, a);
    uthread_create(paper, a);
    uthread_create(tobacco, a);
  
  uthread_join (uthread_create (agent, a), 0);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}