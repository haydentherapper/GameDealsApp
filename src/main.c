#include <pebble.h>
#include <data-processor.h>
  
#define NUM_MENU_SECTIONS 1

static Window *s_main_window;
static MenuLayer *s_menu_layer;

static int s_num_menu_items;
static char **s_strings;

enum {
  NUM_MENU_ITEMS = 0,
  POSTS = 1,
  COMMENTS = 2,
};

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  
  while(t != NULL) {
    switch (t->key) {
      case NUM_MENU_ITEMS:
        s_num_menu_items = t->value->uint16;
        break;
      case POSTS:
      {
        ProcessingState *state = data_processor_create(t->value->cstring, '|');
        uint8_t num_strings = data_processor_count(state);
        s_strings = malloc(sizeof(char*) * num_strings);
        for (uint8_t n = 0; n < num_strings; n += 1) {
          s_strings[n] = data_processor_get_string(state);
        }
        data_processor_destroy(state);
        break;
      }
      case COMMENTS:
        break;
    }
    // Get next pair, if any
    t = dict_read_next(iterator);
  }
  
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
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

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *callback_context) {
  return NUM_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  switch (section_index) {
    case 0:
      return s_num_menu_items;
    default:
      return 0;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, "Top posts");
      break;
    default:
      menu_cell_basic_header_draw(ctx, cell_layer, "Posts");
      break;
  }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case 0:
      // This is a basic menu item with a title and subtitle
      menu_cell_basic_draw(ctx, cell_layer, s_strings[cell_index->row], NULL, NULL);
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // TODO: Load comments of post
}
  
static void main_window_load(Window *window) {
  // Initialize MenuLayer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  s_menu_layer = menu_layer_create(bounds);
  
  // Create MenuLayer
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  
  // Add basic click config
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

static void request_posts() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Cannot create iter");
    return;
  }

  dict_write_uint8(iter, 0, 0);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void init() {
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(s_main_window, true);
  
  request_posts();
}

static void deinit() {
  window_destroy(s_main_window);
  for (int i = 0; i < s_num_menu_items; i++) {
    free(s_strings[i]);
  }
  free(s_strings);
}
  
int main(void) {
  init();
  app_event_loop();
  deinit();
}