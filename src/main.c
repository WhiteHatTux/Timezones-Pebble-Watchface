#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_LOCATION_NAME 1

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_other_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_bluetooth_layer;
static TextLayer *s_battery_layer;
static Layer *s_canvas_layer;

static GFont s_time_font;
static GFont s_time_font_big;

static char s_utctime_string[] = "00:00   ";
//This offset will be calculated from UTC and not taking into account daylight savings time
static int timezone_offset_config = -5;
static char s_localdate_string[16];


int get_new_hour(int tm_hour){
  int new_hour;
  
  // If offset is positive
  if (timezone_offset_config > 0) {
    // If offset will change time to the next day
    if ((tm_hour + timezone_offset_config) > 23) {
      int hours_for_tomo = (tm_hour + timezone_offset_config) % 24;
      new_hour = hours_for_tomo;
    } else {
      new_hour = tm_hour + timezone_offset_config;
    }
    // If offset is negative
  } else {
    // If offset will change time to yesterday
    if ((tm_hour + timezone_offset_config) < 0 ) {
      int hours_for_yest = tm_hour + timezone_offset_config;
      new_hour = 24 + hours_for_yest;
    } else {
      new_hour = tm_hour + timezone_offset_config;
    }
  }
  return new_hour;
}
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  struct tm *tick_other_time = gmtime(&temp);

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    snprintf(s_utctime_string, sizeof("00:00"), "%02d:%02d", get_new_hour(tick_other_time->tm_hour), tick_other_time->tm_min);
        
  } else {
    // Use 12 hour format
    snprintf(s_utctime_string, sizeof("00:00"), "%02d:%02d", get_new_hour(tick_other_time->tm_hour), tick_other_time->tm_min);
  }
  
  // Write date to s_localdate_string
  strftime(s_localdate_string, sizeof(s_localdate_string), "%Y-%m-%d", tick_other_time);  
  
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_other_time_layer, s_utctime_string);
  text_layer_set_text(s_date_layer, s_localdate_string);
}

static void update_proc(Layer *s_canvas_layer, GContext *ctx) {
  GPoint p0 = GPoint(0, 0);
  GPoint p1 = GPoint(144, 0);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, p0, p1);
}

void bt_handler(bool connected) {
  if (connected) {
    text_layer_set_text(s_bluetooth_layer, "");
  } else {
    text_layer_set_text(s_bluetooth_layer, "-");
    vibes_double_pulse();
  }  
}

static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  if (new_state.charge_percent < 30) {
    text_layer_set_text(s_battery_layer, "-");
  } else {
    text_layer_set_text(s_battery_layer, "");
  }
}

static void main_window_load(Window *window) {
  // Create GFonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SKETCHROCKWELL_30));
  s_time_font_big = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SKETCHROCKWELL_42));
  
  
  //Add bluetooth notification
  s_bluetooth_layer = text_layer_create(GRect(0, 0, 20, 35));
  text_layer_set_background_color(s_bluetooth_layer, COLOR_FALLBACK(GColorDarkGreen, GColorBlack));
  text_layer_set_text_color(s_bluetooth_layer, GColorWhite);
  
  text_layer_set_font(s_bluetooth_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_bluetooth_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bluetooth_layer));
  text_layer_set_text(s_bluetooth_layer, "");
    
  
  // Add second timezone
  s_other_time_layer = text_layer_create(GRect(20, 0, 104, 35));
  text_layer_set_background_color(s_other_time_layer, COLOR_FALLBACK(GColorDarkGreen, GColorBlack));
  text_layer_set_text_color(s_other_time_layer, GColorWhite);
  
  text_layer_set_font(s_other_time_layer, s_time_font);
  text_layer_set_text_alignment(s_other_time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_other_time_layer));
  
    
  //Add battery notification
  s_battery_layer = text_layer_create(GRect(124, 0, 20, 35));
  text_layer_set_background_color(s_battery_layer, COLOR_FALLBACK(GColorDarkGreen, GColorBlack));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  text_layer_set_text(s_battery_layer, "");
  
  
  // Draw line on a special Layer
  s_canvas_layer = layer_create(GRect(0, 36, 144, 1));
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
  
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 45, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, s_time_font_big);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  
  // Create date layer
  s_date_layer = text_layer_create(GRect(0, 100, 144, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(GRect(0, 130, 144, 38));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  // Get the current battery level
  battery_handler(battery_state_service_peek());
  
  // Show current connection state
  bt_handler(bluetooth_connection_service_peek());
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  static char temperature_buffer[8];
  static char location_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)t->value->int32);
  
      break;
    case KEY_LOCATION_NAME:
      snprintf(location_buffer, sizeof(location_buffer), "%s", t->value->cstring);
      
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s - %s", temperature_buffer, location_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_other_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_bluetooth_layer);
  text_layer_destroy(s_battery_layer);
  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_time_font_big);
  layer_destroy(s_canvas_layer);
}
static void  init(){
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register with Bluetooth events
  bluetooth_connection_service_subscribe(bt_handler);
  
  // Register to the Battery State Service
  battery_state_service_subscribe(battery_handler);
  
  // Make sure the time is displayed from the start
  update_time();
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}

