/*
 * emheperal.h
 *
 *  Created on: 22 Aug 2012
 *      Author: richard
 */

#ifndef EPHEMERAL_H_
#define EPHEMERAL_H_

uint32_t ephemeral_seq;

#define START_PORT		49152
#define END_PORT		65535

#define EPHEMERAL_PORT() START_PORT+(ephemeral_seq++ % (END_PORT-START_PORT))

#endif /* EPHEMERAL_H_ */
