#include <pebble.h>
#include "math-sll.h"

#include "vec2.h"
#include "ball.h"

#define FPS 30
// Sub steps are still wip
#define SUB_STEPS 1
// #define MULTI_UPDATE_LOOP

#define BALL_COUNT 45
#define BALL_RADIUS 13

static Window *s_window;
static Layer *s_ballLayer;

static const uint32_t s_waitTimeMS = 1000 / FPS;
static sll s_deltaTime;

static Vec2 s_gravity;
static GPoint s_halfScreenGP;
static int s_worldRadius;

static GColor s_ballColor;
static GColor s_borderColor;
static Ball s_ballArr[BALL_COUNT];

static const GPathInfo MINUTE_HAND_POINTS = {
	4, (GPoint []){
		{-8, 10},
		{8, 10},
		{8, -120},
		{-8, -120}
	}
};
static GPath *s_minuteHand;
static int s_minuteHandAngle;
static sll s_minuteHandLength = _int2sll(120);
static Vec2 s_minuteHandPos, s_minuteHandNorm, s_minuteHandTan;

static const GPathInfo HOUR_HAND_POINTS = {
	4, (GPoint []){
		{-6, 10},
		{6, 10},
		{6, -80},
		{-6, -80}
	}
};
static GPath *s_hourHand;
static int s_hourHandAngle;
static sll s_hourHandLength = _int2sll(85);
static Vec2 s_hourHandPos, s_hourHandNorm, s_hourHandTan;

static sll s_handThickness = _int2sll(8);

static void updateHands() {
	time_t temp = time(NULL);
	tm *tickTime = localtime(&temp);

	s_minuteHandAngle = TRIG_MAX_ANGLE * tickTime->tm_min / 60;
	s_hourHandAngle = TRIG_MAX_ANGLE * (tickTime->tm_hour % 12 * 6 + tickTime->tm_min / 10) / (12 * 6);

	sll minuteHandAngleSll = sllmul(slldiv(int2sll(s_minuteHandAngle), int2sll(TRIG_MAX_ANGLE)), sllmul2(CONST_PI));
	s_minuteHandNorm.x = sllsin(minuteHandAngleSll);
	s_minuteHandNorm.y = -sllcos(minuteHandAngleSll);
	s_minuteHandTan.x = s_minuteHandNorm.y;
	s_minuteHandTan.y = -s_minuteHandNorm.x;
	s_minuteHandPos.x = sllmul(s_minuteHandNorm.x, s_minuteHandLength);
	s_minuteHandPos.y = sllmul(s_minuteHandNorm.y, s_minuteHandLength);

	sll hourHandAngleSll = sllmul(slldiv(int2sll(s_hourHandAngle), int2sll(TRIG_MAX_ANGLE)), sllmul2(CONST_PI));
	s_hourHandNorm.x = sllsin(hourHandAngleSll);
	s_hourHandNorm.y = -sllcos(hourHandAngleSll);
	s_hourHandTan.x = s_hourHandNorm.y;
	s_hourHandTan.y = -s_hourHandNorm.x;
	s_hourHandPos.x = sllmul(s_hourHandNorm.x, s_hourHandLength);
	s_hourHandPos.y = sllmul(s_hourHandNorm.y, s_hourHandLength);
}

static void tickTimerCallback(tm *tickTime, TimeUnits unitsChanged) {
	updateHands();
	layer_mark_dirty(s_ballLayer);
}

static void batteryCallback(BatteryChargeState state) {
	if (state.charge_percent <= 20) {
		s_ballColor = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
	} else if (state.charge_percent <= 40) {
		s_ballColor = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
	} else {
		s_ballColor = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
	}
	layer_mark_dirty(s_ballLayer);
}

static void bluetoothCallback(bool connected) {
	if (!connected) {
		s_borderColor = PBL_IF_COLOR_ELSE(GColorVeryLightBlue, GColorBlack);
		vibes_double_pulse();
	} else {
		s_borderColor = GColorWhite;
	}
	layer_mark_dirty(s_ballLayer);
}

static void ballCollideBall(Ball *ball1, Ball *ball2) {
	sll dist, minDist = int2sll(ball1->radius + ball2->radius);
	Vec2 dir;
	v2sub(&dir, &ball1->position, &ball2->position);
	v2normalize(&dir, &dist, &dir);

	// inside other
	if (dist < minDist) {
		sll force = slldiv2(sllsub(dist, minDist));

		Vec2 forceV;
		v2mulsll(&forceV, &dir, force);
		v2sub(&ball1->position, &ball1->position, &forceV);

		v2mulsll(&forceV, &dir, force);
		v2add(&ball2->position, &ball2->position, &forceV);
	}
}

static void ballConstrainWorld(Ball *ball) {
	sll dist, minDist = int2sll(s_worldRadius - ball->radius - 1);
	Vec2 dir;
	v2normalize(&dir, &dist, &ball->position);

	// outside world
	if (dist > minDist) {
		v2mulsll(&ball->position, &dir, minDist);
	}
}

static void ballCollideHand(Ball *ball, sll handLength, Vec2 *handPos, Vec2 *handNorm, Vec2 *handTan) {
	sll distAlongHand = v2dot(&ball->position, handNorm);

	// Default to along the hand
	Vec2 norm;
	v2copy(&norm, handTan);
	sll distAwayFromHand = v2dot(&ball->position, &norm);
	if (distAwayFromHand < 0) {
		v2neg(&norm, &norm);
		distAwayFromHand = -distAwayFromHand;
	}

	// Check if ball is colliding with hand end
	if (distAlongHand < 0 || distAlongHand > handLength) {
		// Check what end the ball is colliding with
		if (distAlongHand < 0) {
			// ball.pos - hand.start, hand.start is (0,0) so just normalize ball.pos
			v2normalize(&norm, &distAwayFromHand, &ball->position);
		} else {
			v2sub(&norm, &ball->position, handPos);
			v2normalize(&norm, &distAwayFromHand, &norm);
		}
		distAwayFromHand = v2dot(&ball->position, &norm);
		if (distAwayFromHand < 0) {
			distAwayFromHand = -distAwayFromHand;
		}
	}

	sll minDist = slladd(s_handThickness, int2sll(ball->radius));
	if (distAwayFromHand < minDist) {
		sll force = sllsub(distAwayFromHand, minDist);
		Vec2 forceV;
		v2mulsll(&forceV, &norm, force);
		v2sub(&ball->position, &ball->position, &forceV);
	}
}

static void simUpdate() {
	sll dt = slldiv(s_deltaTime, int2sll(SUB_STEPS));
	for (short subStep = 0; subStep < SUB_STEPS; subStep++) {
		for (int i = 0; i < BALL_COUNT; i++) {
			Ball *ball = &s_ballArr[i];
			// constrain + collide
			for (int j = 0; j < BALL_COUNT; j++) {
				Ball *ball2 = &s_ballArr[j];
				ballCollideBall(ball, ball2);
			}

			ballCollideHand(ball, s_minuteHandLength, &s_minuteHandPos, &s_minuteHandNorm, &s_minuteHandTan);
			ballCollideHand(ball, s_hourHandLength, &s_hourHandPos, &s_hourHandNorm, &s_hourHandTan);

			ballConstrainWorld(ball);
#ifdef MULTI_UPDATE_LOOP
		}
		for (int i = 0; i < BALL_COUNT; i++) {
			Ball *ball = &s_ballArr[i];
#endif
			// update
			v2add(&ball->acceleration, &ball->acceleration, &s_gravity);
			ballUpdate(ball, dt);
		}
	}
	layer_mark_dirty(s_ballLayer);
}

static void ballLayerUpdateProc(Layer *layer, GContext *ctx) {
	for (int i = 0; i < BALL_COUNT; i++) {
		ballDraw(&s_ballArr[i], ctx, &s_halfScreenGP, s_ballColor);
	}

	// hands
	graphics_context_set_fill_color(ctx, s_borderColor);
	graphics_context_set_stroke_color(ctx, GColorBlack);

	gpath_rotate_to(s_minuteHand, s_minuteHandAngle);
	gpath_draw_filled(ctx, s_minuteHand);
	gpath_draw_outline(ctx, s_minuteHand);

	gpath_rotate_to(s_hourHand, s_hourHandAngle);
	gpath_draw_filled(ctx, s_hourHand);
	gpath_draw_outline(ctx, s_hourHand);

	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(s_halfScreenGP.x - 1, s_halfScreenGP.y - 1, 3, 3), 0, GCornerNone);

	// debug
	// GPoint normPoint = GPoint(sll2int(sllmul2n(s_minuteHandNorm.x, 4)) + s_halfScreenGP.x, sll2int(sllmul2n(s_minuteHandNorm.y, 4)) + s_halfScreenGP.y);
	// GPoint tanPoint = GPoint(sll2int(sllmul2n(s_minuteHandTan.x, 4)) + s_halfScreenGP.x, sll2int(sllmul2n(s_minuteHandTan.y, 4)) + s_halfScreenGP.y);
	//
	// graphics_context_set_stroke_color(ctx, GColorRed);
	// graphics_draw_line(ctx, s_halfScreenGP, normPoint);
	// graphics_context_set_stroke_color(ctx, GColorBlue);
	// graphics_draw_line(ctx, s_halfScreenGP, tanPoint);

	// border
	graphics_context_set_stroke_width(ctx, 4);
	graphics_context_set_stroke_color(ctx, s_borderColor);
	graphics_draw_circle(ctx, s_halfScreenGP, s_worldRadius - 2);
}

static void handleTimerTick() {
	simUpdate();
	app_timer_register(s_waitTimeMS, handleTimerTick, NULL);
}

static void windowLoad(Window *window) {
	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(windowLayer);
	int minBound;
	if (bounds.size.w < bounds.size.h) {
		minBound = bounds.size.w;
	} else {
		minBound = bounds.size.h;
	}

	// globals
	s_deltaTime = dbl2sll((double) s_waitTimeMS / 1000.0);

	v2copyi(&s_gravity, 0, 100);

	s_halfScreenGP = GPoint(bounds.size.w / 2, bounds.size.h / 2);
	s_worldRadius = minBound / 2;

	// balls
	sll ballCount10 = slladd(CONST_10, int2sll(BALL_COUNT));
	sll ballSpiralRadius = int2sll(s_worldRadius - BALL_RADIUS);
	sll spiralMul = dbl2sll(4.0);
	for (int i = 0; i < BALL_COUNT; i++) {
		Ball *ball = &s_ballArr[i];
		sll is = int2sll(i);
		sll spiral = sllmul(slldiv(slladd(CONST_10, is), ballCount10), ballSpiralRadius);

		Vec2 pos = vec2zero;
		is = sllmul(is, spiralMul);
		pos.x = sllcos(is);
		pos.y = sllsin(is);
		v2mulsll(&pos, &pos, spiral);
		ballCreate(ball, pos, BALL_RADIUS);
	}

	// layers
	s_ballLayer = layer_create(bounds);
	layer_set_update_proc(s_ballLayer, ballLayerUpdateProc);
	layer_add_child(windowLayer, s_ballLayer);

	// start simulation
	app_timer_register(s_waitTimeMS, handleTimerTick, NULL);
	layer_mark_dirty(s_ballLayer);
}

static void windowUnload(Window *window) {
	layer_destroy(s_ballLayer);
}

static void init() {
	s_window = window_create();
	window_set_background_color(s_window, GColorBlack);
	window_set_window_handlers(s_window, (WindowHandlers){
								   .load = windowLoad,
								   .unload = windowUnload,
							   });
	window_stack_push(s_window, true);

	tick_timer_service_subscribe(MINUTE_UNIT, tickTimerCallback);
	updateHands();

	battery_state_service_subscribe(batteryCallback);
	batteryCallback(battery_state_service_peek());

	connection_service_subscribe((ConnectionHandlers){
		.pebble_app_connection_handler = bluetoothCallback
	});
	bluetoothCallback(connection_service_peek_pebble_app_connection());

	s_minuteHand = gpath_create(&MINUTE_HAND_POINTS);
	s_hourHand = gpath_create(&HOUR_HAND_POINTS);

	Layer *windowLayer = window_get_root_layer(s_window);
	GRect bounds = layer_get_bounds(windowLayer);
	GPoint center = grect_center_point(&bounds);
	gpath_move_to(s_minuteHand, center);
	gpath_move_to(s_hourHand, center);
}

static void deinit() {
	gpath_destroy(s_minuteHand);
	gpath_destroy(s_hourHand);

	connection_service_unsubscribe();
	battery_state_service_unsubscribe();
	tick_timer_service_unsubscribe();
	window_destroy(s_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
