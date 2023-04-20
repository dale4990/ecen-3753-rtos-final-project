/*
 * fifo.h
 *
 *  Created on: Mar 3, 2023
 *      Author: daniellee
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef FIFO_H_
#define FIFO_H_

#define FIFO_SIZE 10

typedef struct node {
  int state;
  struct node *next;
} node_t;

typedef struct {
  node_t *head;
  node_t *tail;
  int count;
} fifo_t;

// initialize the FIFO
void fifo_init(fifo_t *fifo);

// check if the FIFO is empty
int fifo_empty(fifo_t *fifo);

// check if the FIFO is full
int fifo_full(fifo_t *fifo);

// add an entry to the FIFO
void fifo_push(fifo_t *fifo, int state);

// remove and return the oldest entry from the FIFO
int fifo_pop(fifo_t *fifo);

// get the newest entry in the FIFO without removing it
int fifo_peek(fifo_t *fifo);

#endif /* FIFO_H_ */
