//
// Created by duncan on 6/11/26.
//

#ifndef BALL_SIM_WATCHFACE_BALL_H
#define BALL_SIM_WATCHFACE_BALL_H

#include <pebble.h>

#include "vec2.h"

typedef struct Ball {
	Vec2 position;
	Vec2 positionPrev;
	Vec2 acceleration;
	int radius;
} Ball;

Ball *ballCreateP(Vec2 pos, int radius) {
	Ball *ball = malloc(sizeof(Ball));
	v2copy(&ball->position, &pos);
	v2copy(&ball->positionPrev, &pos);
	v2copyi(&ball->acceleration, 0, 0);
	ball->radius = radius;
	return ball;
}

void ballCreate(Ball *ball, Vec2 pos, int radius) {
	v2copy(&ball->position, &pos);
	v2copy(&ball->positionPrev, &pos);
	v2copyi(&ball->acceleration, 0, 0);
	ball->radius = radius;
}

void ballUpdate(Ball *ball, sll dt) {
	Vec2 delta;
	v2sub(&delta, &ball->position, &ball->positionPrev);
	v2copy(&ball->positionPrev, &ball->position);

	Vec2 acc;
	v2mulsll(&acc, &ball->acceleration, sllmul(dt, dt));
	v2add(&delta, &delta, &acc);
	v2add(&ball->position, &ball->position, &delta);

	v2copyi(&ball->acceleration, 0, 0);
}

void ballGetVelocityPrev(Ball *ball, Vec2 *vel, sll dt) {
	// (curr - prev) / 2t
	v2sub(vel, &ball->position, &ball->positionPrev);
	v2divsll(vel, vel, dt);
}

void ballSetVelocity(Ball *ball, Vec2 vel, sll dt) {
	v2mulsll(&vel, &vel, dt);
	v2sub(&ball->positionPrev, &ball->position, &vel);
}

void ballDraw(Ball *ball, GContext *ctx, GPoint *offset, GColor color) {
	graphics_context_set_fill_color(ctx, color);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	GPoint p = GPoint(sll2int(ball->position.x) + offset->x, sll2int(ball->position.y) + offset->y);
	graphics_fill_circle(ctx, p, ball->radius);
	graphics_draw_circle(ctx, p, ball->radius);
}

void ballDestroyP(Ball *ball) {
	free(ball);
}

#endif //BALL_SIM_WATCHFACE_BALL_H
