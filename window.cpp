#include <cstring>
#include <iostream>
#include <thread>

#include "window.h"
#include "mem.h"
#include "buttons.h"
#include "openal_output.h"
#include "sound.h"

#define FLAG_GPU_BG     0x01
#define FLAG_GPU_SPR    0x02
#define FLAG_GPU_SPR_SZ 0x04
#define FLAG_GPU_BG_TM  0x08
#define FLAG_GPU_BG_WIN_TS  0x10
#define FLAG_GPU_WIN    0x20
#define FLAG_GPU_WIN_TM 0x40
#define FLAG_GPU_DISP   0x80

#define PLT_COLOR0 0x03
#define PLT_COLOR1 0x0C
#define PLT_COLOR2 0x30
#define PLT_COLOR3 0xC0

#define KEY_RIGHT  0x01
#define KEY_LEFT   0x02
#define KEY_UP     0x04
#define KEY_DOWN   0x08
#define KEY_A      0x10
#define KEY_B      0x20
#define KEY_START  0x40
#define KEY_SELECT 0x80

void Window::poll_buttons() {

  glfwPollEvents();

  // Check if the ESC key was pressed or the window was closed
  if (glfwGetKey(game_window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
      glfwWindowShouldClose(game_window) == 1) {
    close = true;
  }

  breakpoint = glfwGetKey(game_window, GLFW_KEY_B) == GLFW_PRESS;

  BTN.state = 0;
  if (glfwGetKey(game_window, GLFW_KEY_LEFT) == GLFW_PRESS)
    BTN.state |= KEY_LEFT;
  if (glfwGetKey(game_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    BTN.state |= KEY_RIGHT;
  if (glfwGetKey(game_window, GLFW_KEY_UP) == GLFW_PRESS)
    BTN.state |= KEY_UP;
  if (glfwGetKey(game_window, GLFW_KEY_DOWN) == GLFW_PRESS)
    BTN.state |= KEY_DOWN;
  if (glfwGetKey(game_window, GLFW_KEY_Z) == GLFW_PRESS)
    BTN.state |= KEY_A;
  if (glfwGetKey(game_window, GLFW_KEY_X) == GLFW_PRESS)
    BTN.state |= KEY_B;
  if (glfwGetKey(game_window, GLFW_KEY_C) == GLFW_PRESS)
    BTN.state |= KEY_START;
  if (glfwGetKey(game_window, GLFW_KEY_V) == GLFW_PRESS)
    BTN.state |= KEY_SELECT;
  if (glfwGetKey(game_window, GLFW_KEY_F1) == GLFW_PRESS)
    SND.mute_ch1 = glfwGetKey(game_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  if (glfwGetKey(game_window, GLFW_KEY_F2) == GLFW_PRESS)
    SND.mute_ch2 = glfwGetKey(game_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  if (glfwGetKey(game_window, GLFW_KEY_F3) == GLFW_PRESS)
    SND.mute_ch3 = glfwGetKey(game_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  if (glfwGetKey(game_window, GLFW_KEY_F4) == GLFW_PRESS)
    SND.mute_ch4 = glfwGetKey(game_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

  if (glfwGetKey(game_window, GLFW_KEY_F5) == GLFW_PRESS && !f5_down) {
    f5_down = true;
  }
  if (glfwGetKey(game_window, GLFW_KEY_F6) == GLFW_PRESS && !f6_down) {
    f6_down = true;
  }

  if (glfwGetKey(game_window, GLFW_KEY_F5) == GLFW_RELEASE && f5_down) {
    save_state = true;
    f5_down = false;
  }
  if (glfwGetKey(game_window, GLFW_KEY_F6) == GLFW_RELEASE && f6_down) {
    load_state = true;
    f6_down = false;
  }
}

void Window::debug_pixel(uint8_t *addr) {
  addr[0] = 255;
  addr[1] = 0;
  addr[2] = 0;
}

void Window::draw_pixel(uint8_t *addr, uint8_t color_id) {
	assert(color_id <= 3);
	switch (color_id) {
		case COLOR_WHITE:
			memset(addr, 255, 3);
			break;
		case COLOR_GRAY1:
			memset(addr, 192, 3);
			break;
		case COLOR_GRAY2:
			memset(addr, 96, 3);
			break;
		default:
		case COLOR_BLACK:
			memset(addr, 0, 3);
			break;
	}
}

uint8_t * Window::get_tile(const uint8_t tile_id, const bool tileset1) {
  uint8_t *tile;
  if (tileset1) {
    tile = &MEM.TILESET1[tile_id * 16];
  } else {
    tile = MEM.getReadPtr(0x9000 + (int16_t)((int8_t)tile_id) * 16);
  }
  return tile;
}

void Window::render_buffer_line() {

  if (! (*MEM.LCD_CTRL & FLAG_GPU_DISP) ) return;

  uint8_t lcd_y = *MEM.SCAN_LN;
  assert(lcd_y < 144);

	uint8_t *BG_MAP = (*MEM.LCD_CTRL & FLAG_GPU_BG_TM) ? MEM.TILEMAP1 : MEM.TILEMAP0;

	uint8_t scrl_x = *MEM.SCRL_X;
	uint8_t scrl_y = *MEM.SCRL_Y;

	uint8_t bg_map_pixel_y = lcd_y + scrl_y;
	uint8_t bg_map_tile_y  = bg_map_pixel_y / TILE_H;
	uint8_t bg_tile_y   = bg_map_pixel_y % TILE_H;


  uint8_t *WIN_MAP = (*MEM.LCD_CTRL & FLAG_GPU_WIN_TM) ? MEM.TILEMAP1 : MEM.TILEMAP0;

  int window_x = *MEM.WIN_X - 7;
  int window_y = *MEM.WIN_Y;

  int win_map_pixel_y = lcd_y - window_y;
  int win_map_tile_y  = win_map_pixel_y / TILE_H;
  int win_tile_y  = win_map_pixel_y % TILE_H;

	for (uint8_t lcd_x = 0; lcd_x < GBE_WINDOW_W; ++lcd_x) {
    uint8_t color_id = 0;
    uint8_t color = 0;

    if (*MEM.LCD_CTRL & FLAG_GPU_BG) {
  		uint8_t bg_map_pixel_x = lcd_x + scrl_x;
  		uint8_t bg_map_tile_x  = bg_map_pixel_x / TILE_W;
  		uint8_t bg_tile_x      = bg_map_pixel_x % TILE_W;

  		// get pixel (tile_x, tile_y) from map tile (bg_map_tile_x, bg_map_tile_y)
		  // and draw it to (lcd_x, lcd_y)
  		uint8_t bg_tile_id = BG_MAP[bg_map_tile_x + bg_map_tile_y * TILEMAP_H];

  		uint8_t *bg_tile = get_tile(bg_tile_id, *MEM.LCD_CTRL & FLAG_GPU_BG_WIN_TS);

  		color_id = get_tile_pixel(bg_tile, bg_tile_x, bg_tile_y);
      color = apply_palette(color_id, *MEM.BG_PLT);
    }

    if ((*MEM.LCD_CTRL & FLAG_GPU_WIN) && (win_map_pixel_y >= 0)) {
      int win_map_pixel_x = lcd_x - window_x;
      int win_map_tile_x  = win_map_pixel_x / TILE_W;
      int win_tile_x  = win_map_pixel_x % TILE_H;

      if (win_map_pixel_x >= 0) {
        uint8_t win_tile_id = WIN_MAP[win_map_tile_x + win_map_tile_y * TILEMAP_H];

        uint8_t *win_tile = get_tile(win_tile_id, *MEM.LCD_CTRL & FLAG_GPU_BG_WIN_TS);
      
        color_id = get_tile_pixel(win_tile, win_tile_x, win_tile_y);
        color = apply_palette(color_id, *MEM.BG_PLT);
      }
    }

    if (*MEM.LCD_CTRL & FLAG_GPU_SPR) {
      for (int spr_id = 0; spr_id < 40; ++spr_id) {
        oam_entry spr = MEM.OAM[spr_id];
        const uint8_t spr_w = 8;
        uint8_t spr_h = (*MEM.LCD_CTRL & FLAG_GPU_SPR_SZ) ? 16 : 8;
        // x and y coords offset in memory..
        int spr_y = ((int)spr.y) - 16;
        int spr_x = ((int)spr.x) - 8;

        // does not intersect current scanline (lcd_y)
        if (lcd_y < spr_y || lcd_y >= spr_y + spr_h) continue; 
        // not onscreen in x direction
        if (spr_x == -spr_w) continue;
        // does not hit current x pixel
        if ((lcd_x > spr_x + 7) || (lcd_x < spr_x)) continue;

        uint8_t spr_tile_y = lcd_y - spr_y;        
        if (spr.yflip) spr_tile_y = (spr_h - 1) - spr_tile_y;

        uint8_t spr_tile_x = lcd_x - spr_x;
        if (spr.xflip) spr_tile_x = (spr_w - 1) - spr_tile_x;

        // ignore lsb if double-height sprite
        uint8_t spr_tile_id = (spr_h == 16) ? spr.tile_id & ~1 : spr.tile_id;
        uint8_t *spr_tile = get_tile(spr_tile_id, true);
        
        if (!spr.priority || color_id == 0) {
          uint8_t spr_color_id = get_tile_pixel(spr_tile, spr_tile_x, spr_tile_y);
          if (spr_color_id == 0) continue; // sprite color 0 is transparent
          color_id = spr_color_id;
          color = apply_palette(color_id, spr.palette ? *MEM.OBJ1_PLT : *MEM.OBJ0_PLT);
        }
      }  
    }

    unsigned i = rgb_buffer_index(lcd_x, lcd_y, GBE_WINDOW_W, GBE_WINDOW_H);
    draw_pixel(&gbe_buffer[i], color);
	} 
}

uint8_t Window::apply_palette(uint8_t color_id, uint8_t palette) {
	assert(color_id <= 3);
	switch (color_id) {
		case 0:
			return (palette & PLT_COLOR0);
		case 1:
			return (palette & PLT_COLOR1) >> 2;
		case 2:
			return (palette & PLT_COLOR2) >> 4;
		default:
		case 3:
			return (palette & PLT_COLOR3) >> 6;
	}
}

uint8_t Window::get_tile_pixel(uint8_t *tile, uint8_t x, uint8_t y) {

	assert(x <= 7);
	assert(y <= ((*MEM.LCD_CTRL & FLAG_GPU_SPR_SZ) ? 15 : 7));

	uint8_t *row_y    = tile + 2*y; // tiles have 2 bytes per row
	uint8_t bitmask_x = 0x80 >> x;

	bool bit0 = row_y[0] & bitmask_x;
	bool bit1 = row_y[1] & bitmask_x;
	uint8_t color_id = bit0 + bit1 * 2;  
	return color_id;
}

unsigned Window::rgb_buffer_index(unsigned x, unsigned y, unsigned w, unsigned h) {
	return x*3 + (h - 1 - y)*w*3;
}

void Window::render_tile(uint8_t *buffer, uint8_t *tile, unsigned lcd_x, unsigned lcd_y, unsigned buffer_w, unsigned buffer_h) {
	for (uint8_t yoff = 0; yoff < TILE_H; yoff++) {
		for (uint8_t xoff = 0; xoff < TILE_W; xoff++) {
			uint8_t color_id = get_tile_pixel(tile, xoff, yoff);

			unsigned i = rgb_buffer_index(lcd_x + xoff, lcd_y + yoff, buffer_w, buffer_h);
			draw_pixel(&buffer[i], color_id);
		}
	}
}

void Window::render_tilemap() {
	uint8_t *MAP = (*MEM.LCD_CTRL & FLAG_GPU_BG_TM) ? MEM.TILEMAP1 : MEM.TILEMAP0;

	for (uint8_t xoff = 0; xoff < TILEMAP_W; ++xoff) {
		for (uint8_t yoff = 0; yoff < TILEMAP_H; ++yoff) {
			uint8_t tile_id = MAP[xoff + yoff * TILEMAP_H];

			uint8_t *tile = get_tile(tile_id, *MEM.LCD_CTRL & FLAG_GPU_BG_WIN_TS);
			unsigned lcd_x = xoff * TILE_W;
			unsigned lcd_y = yoff * TILE_H;

			render_tile(tilemap_buffer, tile, lcd_x, lcd_y, TILEMAP_WINDOW_W, TILEMAP_WINDOW_H*2);
		}	
	}

  MAP = (*MEM.LCD_CTRL & FLAG_GPU_BG_TM) ? MEM.TILEMAP0 : MEM.TILEMAP1;

  for (uint8_t xoff = 0; xoff < TILEMAP_W; ++xoff) {
    for (uint8_t yoff = 0; yoff < TILEMAP_H; ++yoff) {
      uint8_t tile_id = MAP[xoff + yoff * TILEMAP_H];

      uint8_t *tile = get_tile(tile_id, *MEM.LCD_CTRL & FLAG_GPU_BG_WIN_TS);
      unsigned lcd_x = xoff * TILE_W;
      uint8_t lcd_y = yoff * TILE_H;

      render_tile(tilemap_buffer, tile, lcd_x, lcd_y + TILEMAP_WINDOW_H, TILEMAP_WINDOW_W, TILEMAP_WINDOW_H*2);  
    } 
  }
}

void Window::render_tileset() { 
	uint8_t *SET = MEM.TILESET1;

	uint16_t tile_id = 0;
	for (uint8_t yoff = 0; yoff < 24; ++yoff) {
		for (uint8_t xoff = 0; xoff < 16; ++xoff) {
		
			uint8_t *tile = &SET[tile_id * 16];
			tile_id++;
			uint8_t lcd_x = xoff * TILE_W;
			uint8_t lcd_y = yoff * TILE_H;

			render_tile(tileset_buffer, tile, lcd_x, lcd_y, TILESET_WINDOW_W, TILESET_WINDOW_H);	
		}	
	}	
}

void Window::draw_buffer() {

  // copy gbe buffer to window buffer
  const unsigned win_buffer_row_width = GBE_WINDOW_W*window_scale*3;
  const unsigned gbe_buffer_row_width = GBE_WINDOW_W*3;
  for (unsigned y = 0; y < GBE_WINDOW_H; ++y) {
    for (unsigned y_i = 0; y_i < window_scale; ++y_i) {
      for (unsigned x = 0; x < gbe_buffer_row_width; x += 3) {
        for (unsigned x_i = 0; x_i < window_scale*3; x_i += 3) {
          for (unsigned c = 0; c < 3; c++) {
            window_buffer[(y * window_scale + y_i) * win_buffer_row_width + (x * window_scale + x_i) + c] =
                    gbe_buffer[y * gbe_buffer_row_width + x + c];
          }
        }
      }
    }
  }

  // draw
  poll_buttons();
  glfwMakeContextCurrent(game_window);
  glfwSwapInterval(0);
  glClear( GL_COLOR_BUFFER_BIT );
  glClearColor(0.0f, 0.0f, 0.4f, 0.5f);
  glDrawPixels(GBE_WINDOW_W * window_scale, GBE_WINDOW_H * window_scale, GL_RGB, GL_UNSIGNED_BYTE, window_buffer);
  glfwSwapBuffers(game_window);

  refresh_debug();
}

void Window::refresh_debug() {
	render_tileset();
  draw_tileset();
  render_tilemap();
  draw_tilemap();
}

void Window::draw_tilemap() {
	glfwMakeContextCurrent(tilemap_window);
  glfwSwapInterval(0);
	glClear( GL_COLOR_BUFFER_BIT );
  glClearColor(0.0f, 0.0f, 0.4f, 0.5f);
  glDrawPixels(TILEMAP_WINDOW_W, TILEMAP_WINDOW_H * 2, GL_RGB, GL_UNSIGNED_BYTE, tilemap_buffer);
  glfwSwapBuffers(tilemap_window);  	
}

void Window::draw_tileset() {
	glfwMakeContextCurrent(tileset_window);
  glfwSwapInterval(0);
	glClear( GL_COLOR_BUFFER_BIT );
  glClearColor(0.0f, 0.0f, 0.4f, 0.5f);
  glDrawPixels(TILESET_WINDOW_W, TILESET_WINDOW_H, GL_RGB, GL_UNSIGNED_BYTE, tileset_buffer);
  glfwSwapBuffers(tileset_window);  	
}

void Window::init() {
  window_buffer = (uint8_t*)calloc(GBE_WINDOW_H * window_scale * GBE_WINDOW_W * window_scale * 3, sizeof(uint8_t));

	memset(gbe_buffer, 0, GBE_WINDOW_H * GBE_WINDOW_W * 3);
	memset(tilemap_buffer, 0, TILEMAP_WINDOW_H * 2 * TILEMAP_WINDOW_W * 3);
	memset(tileset_buffer, 0, TILESET_WINDOW_H * TILESET_WINDOW_W * 3);

  if (!glfwInit()) {
    printf("Failed to initialize GLFW\n");
    exit(1);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);

  // Open a window and create its OpenGL context
  tilemap_window = glfwCreateWindow(TILEMAP_WINDOW_W, TILEMAP_WINDOW_H * 2, "gbe tilemap", nullptr, nullptr);
  tileset_window = glfwCreateWindow(TILESET_WINDOW_W, TILESET_WINDOW_H, "gbe tileset", nullptr, nullptr);
  game_window = glfwCreateWindow(GBE_WINDOW_W * window_scale, GBE_WINDOW_H * window_scale, "gbe buffer", nullptr, nullptr);


  glfwSetInputMode(game_window, GLFW_STICKY_KEYS, GL_TRUE);
  glfwMakeContextCurrent(game_window);
  glewExperimental = GL_TRUE;
  glewInit();

  glfwSetWindowUserPointer(game_window, this);

  auto func = [](GLFWwindow* win, int w, int h)
  {
    static_cast<Window*>(glfwGetWindowUserPointer(win))->on_resize_game( w, h );
  };

  glfwSetWindowSizeCallback(game_window, func);
}

void Window::on_resize_game(int w, int h) {

  unsigned new_scale = std::min(w / GBE_WINDOW_W, h / GBE_WINDOW_H);

  if (window_scale != new_scale) {
    free(window_buffer);
    window_scale = new_scale;
    window_buffer = (uint8_t *)
            calloc(GBE_WINDOW_H * window_scale * GBE_WINDOW_W * window_scale * 3, sizeof(uint8_t));
  }
}