/* Characters.h, written by Trevor Andal 
 * Written to gain the best understanding of 
 * embedded display drivers. It uses a 2D array
 * to hold the correct 7x5 bits */
#ifndef CHAR_H
#define CHAR_H

#pragma once
#include <stdint.h>

const uint8_t disp4[5] = { 0x0F, 0x08, 0x08, 0x08, 0x7F };

#endif
