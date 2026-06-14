#include <pebble.h>
#include "math-sll.h"

#include "vec2.h"
#include "ball.h"

#define BALL_COUNT 40
#define BALL_RADIUS 15

static Window *s_window;
static Layer *s_ballLayer;

static const uint32_t s_waitTimeMS = 1000 / 30;
static sll s_deltaTime;

static Vec2 s_gravity;
static GPoint s_halfScreenGP;
static int16_t s_worldRadius;

static GColor s_ballColor;
static GColor s_borderColor;
static Ball s_ballArr[BALL_COUNT];

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

static void ballContrainWorld(Ball *ball) {
    sll dist, minDist = int2sll(s_worldRadius - ball->radius - 1);
    Vec2 dir;
    v2normalize(&dir, &dist, &ball->position);

    // outside world
    if (dist > minDist) {
        // Vec2 vel;
        // ballGetVelocityPrev(ball, &vel, s_deltaTime);
        // v2neg(&vel, &vel);
        // v2mulsll(&vel, &vel, dbl2sll(9.0 / 10.0));

        v2mulsll(&ball->position, &dir, minDist);
        // ballSetVelocity(ball, vel, s_deltaTime);
        // v2add(&ball->positionPrev, &ball->position, &vel);
    }
}

static void simUpdate() {
    for (int i = 0; i < BALL_COUNT; i++) {
        Ball *ball = &s_ballArr[i];
        // contrain + collide
        for (int j = 0; j < BALL_COUNT; j++) {
            Ball *ball2 = &s_ballArr[j];
            ballCollideBall(ball, ball2);
        }
        ballContrainWorld(ball);

        // update
        v2add(&ball->acceleration, &ball->acceleration, &s_gravity);
        ballUpdate(ball, s_deltaTime);
    }
    layer_mark_dirty(s_ballLayer);
}

static void ballLayerUpdateProc(Layer *layer, GContext *ctx) {
    for (int i = 0; i < BALL_COUNT; i++) {
        ballDraw(&s_ballArr[i], ctx, &s_halfScreenGP, s_ballColor);
    }

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
    int16_t minBound;
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

    // ball
    // s_ball = ballCreateP(vec2zero, 10);
    // ballSetVelocity(s_ball, vec2(100, 0), s_deltaTime);
    for (int i = 0; i < BALL_COUNT; i++) {
        Ball *ball = &s_ballArr[i];
        sll is = int2sll(i);

        Vec2 pos = vec2zero;
        pos.x = sllmul(sllcos(is), int2sll(s_worldRadius / 2));
        pos.y = sllmul(sllsin(is), int2sll(s_worldRadius / 2));
        ballCreate(ball, pos, BALL_RADIUS);

        // sll per = sllsub(sllmul2(slldiv(int2sll(i), int2sll(BALL_COUNT))), CONST_1);
        // Vec2 vel = vec2(100, 0);
        // v2mulsll(&vel, &vel, per);
        // ballSetVelocity(ball, vel, s_deltaTime);
    }

    // layers
    s_ballLayer = layer_create(bounds);
    layer_set_update_proc(s_ballLayer, ballLayerUpdateProc);
    layer_add_child(windowLayer, s_ballLayer);

    // start simulation
    app_timer_register(s_waitTimeMS, handleTimerTick, NULL);
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

    battery_state_service_subscribe(batteryCallback);
    batteryCallback(battery_state_service_peek());

    connection_service_subscribe((ConnectionHandlers){
        .pebble_app_connection_handler = bluetoothCallback
    });
    bluetoothCallback(connection_service_peek_pebble_app_connection());
}

static void deinit() {
    connection_service_unsubscribe();
    battery_state_service_unsubscribe();
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
