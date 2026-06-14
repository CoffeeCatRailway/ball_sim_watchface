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

static Ball s_ballArr[BALL_COUNT];

// static Layer *s_batteryLayer;
// static int s_batteryLevel;
//
// static BitmapLayer *s_btIconLayer;
// static GBitmap *s_btIconBitmap;
//
// static void batteryUpdateProc(Layer *layer, GContext *ctx) {
//     GRect bounds = layer_get_bounds(layer);
//
//     int barWidth = (s_batteryLevel * (bounds.size.w - 4)) / 100;
//
//     graphics_context_set_stroke_color(ctx, GColorWhite);
//     graphics_draw_round_rect(ctx, bounds, 2);
//
//     GColor barColor;
//     if (s_batteryLevel <= 20)
//         barColor = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
//     else if (s_batteryLevel <= 40)
//         barColor = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
//     else
//         barColor = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
//
//     graphics_context_set_fill_color(ctx, barColor);
//     graphics_fill_rect(ctx, GRect(2, 2, barWidth, bounds.size.h - 4), 1, GCornerNone);
// }
//
// static void batteryCallback(BatteryChargeState state) {
//     s_batteryLevel = state.charge_percent;
//
//     layer_mark_dirty(s_batteryLayer);
// }
//
// static void bluetoothCallback(bool connected) {
//     layer_set_hidden(bitmap_layer_get_layer(s_btIconLayer), connected);
//
//     if (!connected) {
//         vibes_double_pulse();
//     }
// }

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

static void ballLayerUpdateProc(Layer *layer, GContext *ctx) {
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

        // draw
        ballDraw(ball, ctx, &s_halfScreenGP, GColorGreen);
    }

    graphics_context_set_stroke_width(ctx, 2);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, s_halfScreenGP, s_worldRadius - 1);
}

static void handleTimerTick() {
    layer_mark_dirty(s_ballLayer);
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

    // // battery layer
    // int barWidth = bounds.size.w / 2;
    // int barX = (bounds.size.w - barWidth) / 2;
    // int barY = bounds.size.h / PBL_IF_ROUND_ELSE(8, 28);
    // s_batteryLayer = layer_create(GRect(barX, barY, barWidth, 8));
    // layer_set_update_proc(s_batteryLayer, batteryUpdateProc);
    //
    // layer_add_child(windowLayer, s_batteryLayer);
    //
    // s_btIconBitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
    //
    // int btY = barY + 12;
    // s_btIconLayer = bitmap_layer_create(GRect((bounds.size.w - 30) / 2, btY, 30, 30));
    // bitmap_layer_set_bitmap(s_btIconLayer, s_btIconBitmap);
    // bitmap_layer_set_compositing_mode(s_btIconLayer, GCompOpSet);
    //
    // layer_add_child(windowLayer, bitmap_layer_get_layer(s_btIconLayer));
    // bluetoothCallback(connection_service_peek_pebble_app_connection());
}

static void windowUnload(Window *window) {
    // ballDestroy(s_ball);

    layer_destroy(s_ballLayer);
    // gbitmap_destroy(s_btIconBitmap);
    // bitmap_layer_destroy(s_btIconLayer);
}

static void init() {
    s_window = window_create();
    window_set_background_color(s_window, GColorBlack);
    window_set_window_handlers(s_window, (WindowHandlers){
                                   .load = windowLoad,
                                   .unload = windowUnload,
                               });
    window_stack_push(s_window, true);

    // batteryCallback(battery_state_service_peek());
    //
    // connection_service_subscribe((ConnectionHandlers){
    //     .pebble_app_connection_handler = bluetoothCallback
    // });
}

static void deinit() {
    // connection_service_unsubscribe();
    // battery_state_service_unsubscribe();
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
