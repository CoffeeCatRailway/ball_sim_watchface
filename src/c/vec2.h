//
// Created by duncan on 6/11/26.
//

#ifndef BALL_SIM_WATCHFACE_VEC2_H
#define BALL_SIM_WATCHFACE_VEC2_H

#include "math-sll.h"

typedef struct Vec2 {
    sll x;
    sll y;
} Vec2;

#define vec2(x, y) ((Vec2){int2sll(x), int2sll(y)})
#define vec2zero (vec2(0, 0))
#define vec2one (vec2(1, 1))

void v2copy(Vec2 *dest, Vec2 *src) {
    dest->x = src->x;
    dest->y = src->y;
}

void v2copyi(Vec2 *dest, int x, int y) {
    dest->x = int2sll(x);
    dest->y = int2sll(y);
}

void v2add(Vec2 *dest, Vec2 *a, Vec2 *b) {
    dest->x = slladd(a->x, b->x);
    dest->y = slladd(a->y, b->y);
}

void v2sub(Vec2 *dest, Vec2 *a, Vec2 *b) {
    dest->x = sllsub(a->x, b->x);
    dest->y = sllsub(a->y, b->y);
}

void v2mulsll(Vec2 *dest, Vec2 *a, sll s) {
    dest->x = sllmul(a->x, s);
    dest->y = sllmul(a->y, s);
}

void v2neg(Vec2 *dest, Vec2 *a) {
    dest->x = sllneg(a->x);
    dest->y = sllneg(a->y);
}

sll v2length(Vec2 *a) {
    sll x2 = sllmul(a->x, a->x);
    sll y2 = sllmul(a->y, a->y);
    return sllsqrt(slladd(x2, y2));
}

void v2normalize(Vec2 *dest, Vec2 *a) {
    sll len = v2length(a);
    if (len == 0) {
        dest->x = 0;
        dest->y = 0;
    }
    dest->x = slldiv(a->x, len);
    dest->y = slldiv(a->y, len);
}

#endif //BALL_SIM_WATCHFACE_VEC2_H
