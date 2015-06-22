#include <pebble.h>

Window *window;
TextLayer *text_hour_layer;
TextLayer *text_min_layer;
TextLayer *text_date_layer;
TextLayer *text_day_layer;
Layer *line_layer;
Layer *box_layer;

void line_layer_update_callback(Layer *layer, GContext* ctx) {
GPoint p0,p1;
BatteryChargeState bat_stat = battery_state_service_peek();
p0.x=0;
p0.y=0;
p1.x= (int16_t) (1.4 * bat_stat.charge_percent); // 140 @ 100 percent
p1.y=0;
graphics_context_set_stroke_color(ctx, GColorWhite);
graphics_draw_line(ctx, p0, p1);
}

void box_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void bt_handler(bool connected){
	if(!connected){
		vibes_double_pulse();
		layer_set_hidden(line_layer, true);
	}
  else{
    layer_set_hidden(line_layer, false);
  }
}

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

// Update Time and date.
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
	
  // Handle Time
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
}

void handle_deinit(void) {	
  tick_timer_service_unsubscribe();
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_hour_layer);
	text_layer_destroy(text_min_layer);
	layer_destroy(line_layer);
	layer_destroy(box_layer);
	window_destroy(window);
}

void handle_init(void) {
  // Create the window	
  window = window_create();
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
	
  ////Text Layers
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
  
  //Box Layer
  GRect box_frame = GRect(0, 91, 144, 168-91);
  box_layer = layer_create(box_frame);
  layer_set_update_proc(box_layer, box_layer_update_callback);
  layer_add_child(window_layer, box_layer);
	
  //Hour Layer
  text_hour_layer = text_layer_create(GRect(0, 91, 80, 168-91));
  text_layer_set_text_color(text_hour_layer, GColorBlack);
  text_layer_set_background_color(text_hour_layer, GColorClear);
	text_layer_set_text_alignment(text_hour_layer, GTextAlignmentRight);
  text_layer_set_font(text_hour_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_65)));
  layer_add_child(window_layer, text_layer_get_layer(text_hour_layer));

  //Minutes Layer
  text_min_layer = text_layer_create(GRect(90, 97, 144-90, 168-91));
  text_layer_set_text_color(text_min_layer, GColorBlack);
  text_layer_set_background_color(text_min_layer, GColorClear);
	text_layer_set_text_alignment(text_min_layer, GTextAlignmentCenter);
  text_layer_set_font(text_min_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SEGOE_43)));
  layer_add_child(window_layer, text_layer_get_layer(text_min_layer));
	
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
	
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  handle_minute_tick(tick_time, MINUTE_UNIT);
  window_stack_push(window, true /* Animated */);
}

int main(void) {
  handle_init();

  app_event_loop();
  
  handle_deinit();
}