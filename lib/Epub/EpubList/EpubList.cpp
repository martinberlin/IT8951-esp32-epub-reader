#include "EpubList.h"

#ifndef UNIT_TEST
  #include <esp_log.h>
#else
  #define vTaskDelay(t)
  #define ESP_LOGE(args...)
  #define ESP_LOGI(args...)
  #define ESP_LOGD(args...)
#endif

static const char *TAG = "PUBLIST";

#define PADDING 20
#define EPUBS_PER_PAGE 5

void EpubList::next()
{
  state.selected_item = (state.selected_item + 1) % state.num_epubs;
}

void EpubList::prev()
{
  state.selected_item = (state.selected_item - 1 + state.num_epubs) % state.num_epubs;
}

bool EpubList::load(const char *path)
{
  // Solves bug that is rendering List with only empty rectangles (After deep-sleep)
  // Research why
  if (false)  // state.is_loaded
  {
    ESP_LOGI(TAG, "Already loaded books");
    return true;
  }
  
  renderer->show_busy();
  // trigger a proper redraw
  state.previous_rendered_page = -1;
  // read in the list of epubs
  state.num_epubs = 0;
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(path)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      ESP_LOGD(TAG, "Found file: %s", ent->d_name);
      // ignore any hidden files starting with "." and any directories
      if (ent->d_name[0] == '.' || ent->d_type == DT_DIR)
      {
        continue;
      }
      int name_length = strlen(ent->d_name);
      if (name_length < 5 || strcmp(ent->d_name + name_length - 5, ".epub") != 0)
      {
        continue;
      }
      ESP_LOGD(TAG, "Loading epub %s", ent->d_name);
      Epub *epub = new Epub(std::string("/fs/") + ent->d_name);
      if (epub->load())
      {
        strncpy(state.epub_list[state.num_epubs].path, epub->get_path().c_str(), MAX_PATH_SIZE);
        strncpy(state.epub_list[state.num_epubs].title, replace_html_entities(epub->get_title()).c_str(), MAX_TITLE_SIZE);
        state.num_epubs++;
        if (state.num_epubs == MAX_EPUB_LIST_SIZE)
        {
          ESP_LOGE(TAG, "Too many epubs, max is %d", MAX_EPUB_LIST_SIZE);
          break;
        }
      }
      else
      {
        ESP_LOGE(TAG, "Failed to load epub %s", ent->d_name);
      }
      delete epub;
    }
    closedir(dir);
    std::sort(
        state.epub_list,
        state.epub_list + state.num_epubs,
        [](const EpubListItem &a, const EpubListItem &b)
        {
          return strcmp(a.title, b.title) < 0;
        });
  }
  else
  {
    renderer->clear_screen();
    uint16_t y = renderer->get_page_height()/2-80;
    renderer->show_img(18, y+41, warning_width, warning_height, warning_data);
    const char * warning = "Please insert SD Card";
    renderer->draw_rect(1, y, renderer->get_text_width(warning, true, false)+150, 115, 80);
    renderer->draw_text_box(warning, warning_width+25, y+4, renderer->get_page_width(), 80, true, false);
    renderer->draw_text_box("Restarting in 20 secs.", warning_width+25, y+34, renderer->get_page_width(), 80, false, false);
    //renderer->flush_display();
    ESP_LOGE(TAG, "Is SD-Card inserted and properly connected?\nCould not open directory %s", path);
    #ifndef UNIT_TEST
      vTaskDelay(pdMS_TO_TICKS(1000*20));
      esp_restart();
    #endif
    
    return false;
  }
  // sanity check our state
  if (state.selected_item >= state.num_epubs)
  {
    state.selected_item = 0;
    state.previous_rendered_page = -1;
    state.previous_selected_item = -1;
  }
  state.is_loaded = true;
  return true;
}

void EpubList::render()
{
  ESP_LOGD(TAG, "Rendering EPUB list");
  // what page are we on?
  int current_page = state.selected_item / EPUBS_PER_PAGE;
  // draw a page of epubs
  int cell_height = renderer->get_page_height() / EPUBS_PER_PAGE;
  ESP_LOGD(TAG, "Cell height is %d", cell_height);
  int start_index = current_page * EPUBS_PER_PAGE;
  int ypos = 0;
  // starting a fresh page or rendering from scratch?
  ESP_LOGI(TAG, "Current page is %d, previous page %d, redraw=%d", current_page, state.previous_rendered_page, m_needs_redraw);
  if (current_page != state.previous_rendered_page || m_needs_redraw)
  {
    m_needs_redraw = false;
    renderer->show_busy();
    renderer->clear_screen();
    state.previous_selected_item = -1;
    // trigger a redraw of the items
    state.previous_rendered_page = -1;
  }
  for (int i = start_index; i < start_index + EPUBS_PER_PAGE && i < state.num_epubs; i++)
  {
    // do we need to draw a new page of items?
    if (current_page != state.previous_rendered_page)
    {
      ESP_LOGI(TAG, "Rendering item %d", i);
      Epub *epub = new Epub(state.epub_list[i].path);
      epub->load();
      // draw the cover page
      int image_xpos = PADDING;
      int image_ypos = ypos + PADDING;
      int image_height = cell_height - PADDING * 2;
      int image_width = 2 * image_height / 3;
      size_t image_data_size = 0;
      uint8_t *image_data = epub->get_item_contents(epub->get_cover_image_item(), &image_data_size);
      renderer->draw_image(epub->get_cover_image_item(), image_data, image_data_size, image_xpos, image_ypos, image_width, image_height);
      free(image_data);
      // draw the title
      int text_xpos = image_xpos + image_width + PADDING;
      int text_ypos = ypos + PADDING / 2;
      int text_width = renderer->get_page_width() - (text_xpos + PADDING);
      int text_height = cell_height - PADDING * 2;
      // use the text block to layout the title
      TextBlock *title_block = new TextBlock(LEFT_ALIGN);
      title_block->add_span(state.epub_list[i].title, false, false);
      title_block->layout(renderer, epub, text_width);
      // work out the height of the title
      int title_height = title_block->line_breaks.size() * renderer->get_line_height();
      // center the title in the cell
      int y_offset = title_height < text_height ? (text_height - title_height) / 2 : 0;
      // draw each line of the title making sure we don't run over the cell
      renderer->start_write();
      for (int i = 0; i < title_block->line_breaks.size() && y_offset + renderer->get_line_height() < text_height; i++)
      {
        title_block->render(renderer, i, text_xpos, text_ypos + y_offset);
        y_offset += renderer->get_line_height();
      }
      renderer->end_write();
      delete title_block;
      delete epub;
    }
    // clear the selection box around the previous selected item
    if (state.previous_selected_item == i)
    {
      for (int i = 0; i < 5; i++)
      {
        renderer->draw_rect(i, ypos + PADDING / 2 + i, renderer->get_page_width() - 2 * i, cell_height - PADDING - 2 * i, 255);
      }
    }
    // draw the selection box around the current selection
    if (state.selected_item == i)
    {
      for (int i = 0; i < 5; i++)
      {
        renderer->draw_rect(i, ypos + PADDING / 2 + i, renderer->get_page_width() - 2 * i, cell_height - PADDING - 2 * i, 0);
      }
    }
    ypos += cell_height;
  }
  state.previous_selected_item = state.selected_item;
  state.previous_rendered_page = current_page;
}