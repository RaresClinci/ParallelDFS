// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
// determines who has access to the task queue(and the sum) and when
pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;

/* TODO: Define graph task argument. */
typedef struct args {
	unsigned int idx;
} args_t;

static void process_node(unsigned int);

void process_caller(void *arg)
{
	args_t *call = (args_t *)arg;
	// printf("%d\n", call->idx);
	process_node(call->idx);
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	// beginning operations on the node
	pthread_mutex_lock(&graph_mutex);

	// adding the node to the sum
	sum += graph->nodes[idx]->info;

	// adding the neighbours to the queue
	unsigned int i;

	for (i = 0; i < graph->nodes[idx]->num_neighbours; i++) {
		if (graph->visited[graph->nodes[idx]->neighbours[i]] == NOT_VISITED) {
			graph->visited[graph->nodes[idx]->neighbours[i]] = PROCESSING;

			args_t *arg = (args_t *)malloc(sizeof(args_t));

			DIE(arg == NULL, "malloc");
			arg->idx = graph->nodes[idx]->neighbours[i];

			os_task_t *task = create_task(process_caller, (void *)arg, free);

			enqueue_task(tp, task);
		}
	}
	graph->visited[idx] = DONE;

	pthread_mutex_unlock(&graph_mutex);

	// unlocking threadpool mutex based on conditions
	pthread_mutex_lock(&(tp->mutex));
	if (list_empty(&(tp->head))) {
		// both can be unlocked if the queue is empty, the execution has ended
		pthread_cond_signal(&(tp->cond_stop));
		tp->done = 1;
	} else {
		// the other tasks shouldn't be waiting anymore
		pthread_cond_signal(&(tp->cond_wait));
	}
	pthread_mutex_unlock(&(tp->mutex));
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	return 0;
}
