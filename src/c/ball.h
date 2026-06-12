//
// Created by duncan on 6/11/26.
//

#ifndef BALL_SIM_WATCHFACE_BALL_H
#define BALL_SIM_WATCHFACE_BALL_H

#include <pebble.h>

#include "vec2.h"

typedef struct ball_s {
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    uint16_t radius;
} ball_t;

ball_t* ballCreate(Vec2 pos, Vec2 vel, uint16_t radius) {
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
void ballUpdate(ball_t *ball, sll dt) {
    // Note if 'Fixed-Point Arithmetic' is needed
    // ms = 1000/30, dt = ms/1000
    // 20 * dt = 20/30

    Vec2 newVel;
    v2mulsll(&newVel, &ball->acceleration, dt);
    v2add(&newVel, &ball->velocity, &newVel);

    Vec2 p;
    v2add(&p, &ball->velocity, &newVel);
    v2mulsll(&p, &p, slldiv2(dt));
    v2add(&ball->position, &ball->position, &p);

    v2copy(&ball->velocity, &newVel);
    v2copyi(&ball->acceleration, 0, 0);
}

void ballDraw(ball_t *ball, GContext *ctx, GPoint *offset, GColor color) {
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_circle(ctx, GPoint(sll2int(ball->position.x) + offset->x, sll2int(ball->position.y) + offset->y), ball->radius);
}

void ballDestroy(ball_t* ball) {
    free(ball);
}

#endif //BALL_SIM_WATCHFACE_BALL_H
