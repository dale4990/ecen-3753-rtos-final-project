/*
 * fifo.c
 *
 *  Created on: Mar 3, 2023
 *      Author: daniellee
 */

#include "fifo.h"

// initialize the FIFO
void fifo_init(fifo_t *fifo) {
  fifo->head = NULL;
  fifo->tail = NULL;
  fifo->count = 0;
}

// check if the FIFO is empty
int fifo_empty(fifo_t *fifo) {
  return fifo->head == NULL;
}

// check if the FIFO is full
int fifo_full(fifo_t *fifo) {
  return fifo->count == FIFO_SIZE;
}

// add an entry to the FIFO
void fifo_push(fifo_t *fifo, int state) {
  if (!fifo_full(fifo)) {
    node_t *new_node = malloc(sizeof(node_t));
    new_node->state = state;
    new_node->next = NULL;
    if (fifo_empty(fifo)) {
      fifo->head = new_node;
      fifo->tail = new_node;
    } else {
      fifo->tail->next = new_node;
      fifo->tail = new_node;
    }
    fifo->count++;
  }
}

// remove and return the oldest entry from the FIFO
int fifo_pop(fifo_t *fifo) {
  if (!fifo_empty(fifo)) {
    int state = fifo->head->state;
    node_t *temp = fifo->head;
    fifo->head = fifo->head->next;
    free(temp);
    fifo->count--;
    return state;
  } else {
    return -1; // error code for empty FIFO
  }
}

// get the newest entry in the FIFO without removing it
int fifo_peek(fifo_t *fifo) {
  if (!fifo_empty(fifo)) {
    return fifo->tail->state;
  } else {
    return -1; // error code for empty FIFO
  }
}


