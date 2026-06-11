#include <pebble.h>

#include "vec2.h"
#include "ball.h"

static Window *s_window;
static Layer *s_ballLayer;

static const uint32_t s_waitTimeMS = 1000 / 30;
static const float s_deltaTime = (float) s_waitTimeMS / 1000.f;

static ball_t *s_ball;
static vec2_t s_gravity = vec2(0.f, 200.f);

static int16_t s_screenWidth, s_screenHeight;
static int16_t s_screenWidthHalf, s_screenHeightHalf;

// static GFont s_fontJersey56;
// static GFont s_fontJersey24;
//
// static Layer *s_batteryLayer;
// static int s_batteryLevel;
//
// static BitmapLayer *s_btIconLayer;
// static GBitmap *s_btIconBitmap;
//
// static TextLayer *s_timeLayer;
// static TextLayer *s_dateLayer;
//
// static void update() {
//     time_t temp = time(NULL);
//     struct tm *tickTime = localtime(&temp);
//
//     // time layer
//     static char s_timeBuffer[8];
//     strftime(s_timeBuffer, sizeof(s_timeBuffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tickTime);
//
//     text_layer_set_text(s_timeLayer, s_timeBuffer);
//
//     // date layer
//     static char s_dateBuffer[20];
//     strftime(s_dateBuffer, sizeof(s_dateBuffer), "%a %d/%b %m/%Y", tickTime);
//
//     text_layer_set_text(s_dateLayer, s_dateBuffer);
// }
//
// static void tickHandler(struct tm *tickTime, TimeUnits unitsChanged) {
//     update();
// }
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

static void ballLayerUpdateProc(Layer *layer, GContext *ctx) {
    v2add(&s_ball->acceleration, &s_ball->acceleration, &s_gravity);
    ballUpdate(s_ball, s_deltaTime);

#if defined(PBL_ROUND)
    vec2_t posCentered;
    v2sub(&posCentered, &s_ball->position, &vec2(s_screenWidthHalf, s_screenHeightHalf));
    if (v2lengthsq(&posCentered) > (float) (s_screenWidthHalf * s_screenWidthHalf)) {
        v2neg(&s_ball->velocity, &s_ball->velocity);
    }
#elif defined(PBL_RECT)
    if (s_ball->position.x > (float) (s_screenWidth - s_ball->radius) || s_ball->position.x < (float) s_ball->radius) {
        s_ball->velocity.x = -s_ball->velocity.x;
    }
    if (s_ball->position.y > (float) (s_screenHeight - s_ball->radius) || s_ball->position.y < (float) s_ball->radius) {
        s_ball->velocity.y = -s_ball->velocity.y;
    }
#endif

    ballDraw(s_ball, ctx, GColorGreen);

    graphics_context_set_stroke_width(ctx, 2);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, GPoint(s_screenWidthHalf, s_screenHeightHalf), s_screenWidthHalf - 1);
}

static void handleTimerTick() {
    layer_mark_dirty(s_ballLayer);
    app_timer_register(s_waitTimeMS, handleTimerTick, NULL);
}

static void windowLoad(Window *window) {
    Layer *windowLayer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(windowLayer);

    s_screenWidth = bounds.size.w;
    s_screenHeight = bounds.size.h;
    s_screenWidthHalf = s_screenWidth / 2;
    s_screenHeightHalf = s_screenHeight / 2;

    s_ball = ballCreate(vec2((float) s_screenWidthHalf, (float) s_screenHeightHalf), vec2(100.f, 0.f), 10);

    s_ballLayer = layer_create(bounds);
    layer_set_update_proc(s_ballLayer, ballLayerUpdateProc);
    layer_add_child(windowLayer, s_ballLayer);

    app_timer_register(s_waitTimeMS, handleTimerTick, NULL);

    // s_fontJersey56 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_56));
    // s_fontJersey24 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_24));
    //
    // int dateHeight = 30;
    // int blockHeight = 56 + dateHeight;
    // int timeY = (bounds.size.h - blockHeight) / 2 - 10;
    // int dateY = timeY + 56;
    //
    // // time layer
    // s_timeLayer = text_layer_create(GRect(0, timeY, bounds.size.w, 60));
    // text_layer_set_background_color(s_timeLayer, GColorClear);
    // text_layer_set_text_color(s_timeLayer, GColorWhite);
    // text_layer_set_font(s_timeLayer, s_fontJersey56);
    // text_layer_set_text_alignment(s_timeLayer, GTextAlignmentCenter);
    //
    // layer_add_child(windowLayer, text_layer_get_layer(s_timeLayer));
    //
    // // date layer
    // s_dateLayer = text_layer_create(GRect(0, dateY, bounds.size.w, 30));
    // text_layer_set_background_color(s_dateLayer, GColorClear);
    // text_layer_set_text_color(s_dateLayer, GColorWhite);
    // text_layer_set_font(s_dateLayer, s_fontJersey24);
    // text_layer_set_text_alignment(s_dateLayer, GTextAlignmentCenter);
    //
    // layer_add_child(windowLayer, text_layer_get_layer(s_dateLayer));
    //
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
    ballDestroy(s_ball);

    layer_destroy(s_ballLayer);
    // text_layer_destroy(s_timeLayer);
    // text_layer_destroy(s_dateLayer);
    //
    // fonts_unload_custom_font(s_fontJersey56);
    // fonts_unload_custom_font(s_fontJersey24);
    //
    // layer_destroy(s_batteryLayer);
    //
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

    // update();
    //
    // tick_timer_service_subscribe(MINUTE_UNIT, tickHandler);
    // battery_state_service_subscribe(batteryCallback);
    //
    // batteryCallback(battery_state_service_peek());
    //
    // connection_service_subscribe((ConnectionHandlers){
    //     .pebble_app_connection_handler = bluetoothCallback
    // });
}

static void deinit() {
    // connection_service_unsubscribe();
    // battery_state_service_unsubscribe();
    // tick_timer_service_unsubscribe();
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
