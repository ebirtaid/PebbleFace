#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_HOURLY 2
#define KEY_DAILY 3

Window *window;
TextLayer *text_hour_layer;
TextLayer *text_min_layer;
TextLayer *text_date_layer;
TextLayer *text_day_layer;
TextLayer *text_conditions_layer;
TextLayer *text_forecast_layer;
TextLayer *text_daily_layer;
Layer *line_layer;
Layer *box_layer;

//Show/hide weather
void timer_callback(){
  layer_set_hidden((Layer *)text_date_layer, false);
  layer_set_hidden((Layer *)text_day_layer, false);
  layer_set_hidden((Layer *)text_conditions_layer, true);
  layer_set_hidden((Layer *)text_forecast_layer, true);
  layer_set_hidden((Layer *)text_daily_layer, true);
}

void show_weather(){
  layer_set_hidden((Layer *)text_date_layer, true);
  layer_set_hidden((Layer *)text_day_layer, true);
  layer_set_hidden((Layer *)text_conditions_layer, false);
  layer_set_hidden((Layer *)text_forecast_layer, false);
}

void show_daily(){
  layer_set_hidden((Layer *)text_forecast_layer, true);
  layer_set_hidden((Layer *)text_daily_layer, false);
}

//Tap Handling
//no longer hides weather when shaken a second time
void tap_handler(AccelAxisType axis, int32_t direction) {
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction != 0 && !layer_get_hidden(text_layer_get_layer(text_date_layer))) {
      show_weather();
      app_timer_register(3000, show_daily, NULL);
      app_timer_register(6000, timer_callback, NULL);
    }
//    else{
//      timer_callback();
//    }
    break;
  case ACCEL_AXIS_Y:
    if (direction != 0 && !layer_get_hidden(text_layer_get_layer(text_date_layer))) {
      show_weather();
      app_timer_register(3000, show_daily, NULL);
      app_timer_register(6000, timer_callback, NULL);
    }
//    else{
//      timer_callback();
//    }
    break;
  case ACCEL_AXIS_Z:
    if (direction != 0 && !layer_get_hidden(text_layer_get_layer(text_date_layer))) {
      show_weather();
      app_timer_register(3000, show_daily, NULL);
      app_timer_register(6000, timer_callback, NULL);
    }
//    else{
//      timer_callback();
//    }
    break;
  }
}

//Parse weather
void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  //Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  static char hourly_buffer[64];
  static char daily_buffer[64];
  
  //Read first item
  Tuple *t = dict_read_first(iterator);

  //For all items
  while(t != NULL) {
    //Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
    case KEY_HOURLY:
      snprintf(hourly_buffer, sizeof(hourly_buffer), "%s", t->value->cstring);
    case KEY_DAILY:
      snprintf(daily_buffer, sizeof(daily_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    //Look for next item
    t = dict_read_next(iterator);
  }
  
  //Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
  snprintf(hourly_buffer, sizeof(hourly_buffer), "%s", hourly_buffer);
  snprintf(daily_buffer, sizeof(daily_buffer), "%s", daily_buffer);
  text_layer_set_text(text_conditions_layer, weather_layer_buffer);
  text_layer_set_text(text_forecast_layer, hourly_buffer);
  text_layer_set_text(text_daily_layer, daily_buffer);
}

void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

//Update line
void line_layer_update_callback(Layer *layer, GContext* ctx) {
  GPoint p0,p1;
  BatteryChargeState bat_stat = battery_state_service_peek();
  p0.x=0;
  p0.y=0;
  p1.x=(int16_t)(1.4 * bat_stat.charge_percent); // 140 @ 100 percent
  p1.y=0;
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, p0, p1);
}

//Box
void box_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

//Bluetooth
void bt_handler(bool connected){
  if(!connected){
    vibes_double_pulse();
    layer_set_hidden(line_layer, true);
  }
  else{
    layer_set_hidden(line_layer, false);
  }
}

//Battery
void battery_handler(BatteryChargeState batt){
  bool connected = bluetooth_connection_service_peek();
  bluetooth_connection_service_subscribe(&bt_handler);
  bt_handler(connected);
  if(batt.is_charging && batt.charge_percent <= 97){
    layer_set_hidden(line_layer, true);
    return;
  }
  if(!connected){
    layer_set_hidden(line_layer, true);
  }
  else{
    layer_set_hidden(line_layer, false);
  }
}

//Update Time and date.
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(line_layer);
	
  //Handle Date
  static char date_text[] = "XXXXXXXXX 00";
  static char day_text[] = "Mmmmmmmmm";
  int i;
  
  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  for (i=0; date_text[i]; i++) {
    if(date_text[i]>='a' && date_text[i]<='z') date_text[i] -= 32;
  }
  
  text_layer_set_text(text_date_layer, date_text);
  strftime(day_text, sizeof(day_text), "%A", tick_time);
  text_layer_set_text(text_day_layer, day_text);
  
  //Handle Time
  static char time_text[] = "00:00";
  static char min_text[] = "  ";
  static char hour_text[] = "  ";
  char *time_format;
  
  time_format = "%R";
  strftime(time_text, sizeof(time_text), time_format, tick_time);
  memmove(hour_text, &time_text[0], 2);
  memmove(min_text, &time_text[3], 2);
  
  text_layer_set_text(text_hour_layer, hour_text);
  text_layer_set_text(text_min_layer, min_text);
  
  //Get weather update every 30 minutes
  //20 minutes, change 20 to desired amount of minutes
  if(tick_time->tm_min % 12 == 0) {
  
    //Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    //Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    //Send the message!
    app_message_outbox_send();
  }
}

void handle_init(void) {
  // Create the window	
  window = window_create();
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
	
  ////Text Layers
  //Box Layer
  GRect box_frame = GRect(0, 91, 144, 168-91);
  box_layer = layer_create(box_frame);
  layer_set_update_proc(box_layer, box_layer_update_callback);
  layer_add_child(window_layer, box_layer);
	
  //Hour Layer
  text_hour_layer = text_layer_create(GRect(7, 91, 80, 168-91));
  text_layer_set_text_color(text_hour_layer, GColorBlack);
  text_layer_set_background_color(text_hour_layer, GColorClear);
	text_layer_set_text_alignment(text_hour_layer, GTextAlignmentCenter);
  text_layer_set_font(text_hour_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_65)));
  layer_add_child(window_layer, text_layer_get_layer(text_hour_layer));

  //Minutes Layer
  text_min_layer = text_layer_create(GRect(80, 97, 144-90, 168-91));
  text_layer_set_text_color(text_min_layer, GColorBlack);
  text_layer_set_background_color(text_min_layer, GColorClear);
  text_layer_set_text_alignment(text_min_layer, GTextAlignmentCenter);
  text_layer_set_font(text_min_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_43)));
  layer_add_child(window_layer, text_layer_get_layer(text_min_layer));
  
  //Create conditions Layer
  text_conditions_layer = text_layer_create(GRect(8, 10, 144-8, 168-10));
  text_layer_set_background_color(text_conditions_layer, GColorClear);
  text_layer_set_text_color(text_conditions_layer, GColorWhite);
  text_layer_set_font(text_conditions_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_16)));
  layer_add_child(window_layer, text_layer_get_layer(text_conditions_layer));
  text_layer_set_text(text_conditions_layer, "No data.");
  layer_set_hidden((Layer *)text_conditions_layer, true);
  
  //Create forecast Layer
  text_forecast_layer = text_layer_create(GRect(8, 45, 144-8, 168-10));
  text_layer_set_background_color(text_forecast_layer, GColorClear);
  text_layer_set_text_color(text_forecast_layer, GColorWhite);
  text_layer_set_font(text_forecast_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_16)));
  layer_add_child(window_layer, text_layer_get_layer(text_forecast_layer));
  layer_set_hidden((Layer *)text_forecast_layer, true);
  
  //Create daily Layer
  text_daily_layer = text_layer_create(GRect(8, 45, 144-8, 168-10));
  text_layer_set_background_color(text_daily_layer, GColorClear);
  text_layer_set_text_color(text_daily_layer, GColorWhite);
  text_layer_set_font(text_daily_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_15)));
  layer_add_child(window_layer, text_layer_get_layer(text_daily_layer));
  layer_set_hidden((Layer *)text_daily_layer, true);

  //Date Layer
  text_date_layer = text_layer_create(GRect(8, 10, 144-8, 168-10));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_18)));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));
 	
  //Day of week
  text_day_layer = text_layer_create(GRect(8, 45, 144-8, 168-10));
  text_layer_set_text_color(text_day_layer, GColorWhite);
  text_layer_set_background_color(text_day_layer, GColorClear);
  text_layer_set_font(text_day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_18)));
  layer_add_child(window_layer, text_layer_get_layer(text_day_layer));
	
  //Line
  line_layer = layer_create(GRect(8, 38, 139, 2));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  //BT
  bool connected = bluetooth_connection_service_peek();
  bluetooth_connection_service_subscribe(&bt_handler);
  bt_handler(connected);

  //Battery
  battery_state_service_subscribe(battery_handler);
  BatteryChargeState b = battery_state_service_peek();
  battery_handler(b);
  
  //Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  //Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  accel_tap_service_subscribe(tap_handler);
  
  //Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  handle_minute_tick(tick_time, MINUTE_UNIT);
  window_stack_push(window, true /* Animated */);
}

void handle_deinit(void) {	
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_day_layer);
  text_layer_destroy(text_hour_layer);
  text_layer_destroy(text_min_layer);
  text_layer_destroy(text_forecast_layer);
  text_layer_destroy(text_daily_layer);
  text_layer_destroy(text_conditions_layer);
  layer_destroy(line_layer);
  layer_destroy(box_layer);
  window_destroy(window);
}

int main(void) {
  handle_init();

  app_event_loop();
  
  handle_deinit();
}