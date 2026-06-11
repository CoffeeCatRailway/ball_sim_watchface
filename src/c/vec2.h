//
// Created by duncan on 6/11/26.
//

#ifndef BALL_SIM_WATCHFACE_VEC2_H
#define BALL_SIM_WATCHFACE_VEC2_H

typedef struct vec2_s {
    float x;
    float y;
} vec2_t;

#define vec2(x, y) ((vec2_t){x, y})
#define vec2zero (vec2(0.f, 0.f))
#define vec2one (vec2(1.f, 1.f))

static void v2copy(vec2_t *dest, vec2_t *src) {
    dest->x = src->x;
    dest->y = src->y;
}

static void v2copyf(vec2_t *dest, float x, float y) {
    dest->x = x;
    dest->y = y;
}

static void v2add(vec2_t *dest, vec2_t *a, vec2_t *b) {
    dest->x = a->x + b->x;
    dest->y = a->y + b->y;
}

static void v2mulf(vec2_t *dest, vec2_t *a, float s) {
    dest->x = a->x * s;
    dest->y = a->y * s;
}

#endif //BALL_SIM_WATCHFACE_VEC2_H