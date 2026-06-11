//
// Created by duncan on 6/11/26.
//

#ifndef BALL_SIM_WATCHFACE_BALL_H
#define BALL_SIM_WATCHFACE_BALL_H

#include <pebble.h>

#include "vec2.h"

typedef struct ball_s {
    vec2_t position;
    vec2_t velocity;
    vec2_t acceleration;
    uint16_t radius;
} ball_t;

ball_t* ballCreate(vec2_t pos, vec2_t vel, uint16_t radius) {
    ball_t* ball = malloc(sizeof(ball_t));
    v2copy(&ball->position, &pos);
    v2copy(&ball->velocity, &vel);
    v2copy(&ball->acceleration, &vec2zero);
    ball->radius = radius;
    return ball;
}

/**
 * Verlet Integration
 * <code>
 * newVel = vel + acc * dt
 * pos += (vel + newVel) * 0.5 * dt
 * vel = newVel
 * acc = (0,0)
 * </code>
 */
void ballUpdate(ball_t *ball, float dt) {
    // Note if 'Fixed-Point Arithmetic' is needed
    // ms = 1000/30, dt = ms/1000
    // 20 * dt = 20/30

    vec2_t newVel;
    v2mulf(&newVel, &ball->acceleration, dt);
    v2add(&newVel, &ball->velocity, &newVel);

    vec2_t p;
    v2add(&p, &ball->velocity, &newVel);
    v2mulf(&p, &p, .5f * dt);
    v2add(&ball->position, &ball->position, &p);

    v2copy(&ball->velocity, &newVel);
    v2copyf(&ball->acceleration, 0.f, 0.f);
}

#define fastRound(x) ((int16_t) (x + .5f))

void ballDraw(ball_t *ball, GContext *ctx, GColor color) {
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_circle(ctx, GPoint(fastRound(ball->position.x), fastRound(ball->position.y)), ball->radius);
}

void ballDestroy(ball_t* ball) {
    free(ball);
}

#endif //BALL_SIM_WATCHFACE_BALL_H
