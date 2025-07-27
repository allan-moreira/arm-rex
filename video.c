#include "address_map_arm.h"

void video_text(int, int, char *);
void video_box(int, int, int, int, short);
int  resample_rgb(int, int);
int  get_data_bits(int);
void draw_sprite(int x, int y, int *sprite, int w, int h, int scale);
void clear_screen(short);
void clear_text_screen(void);
int  check_collision(int, int, int, int, int, int, int, int);
void draw_ground(short color);

#define STANDARD_X 320
#define STANDARD_Y 240

#define INTEL_BLUE 0x0071C5
#define WHITE      0xFFFFFF
#define BLACK      0x000000
#define GREEN      0x00FF00

#define SCALE 3

int bird_right[64] = {0,0,0,0,0,0,1,0,
	              0,0,0,0,0,1,1,0,
	              0,0,0,0,1,1,1,0,
	              0,1,0,1,1,1,1,0,
	              1,1,1,1,1,1,1,1,
	              0,0,0,1,1,1,1,0,
	              0,0,0,0,0,0,0,1,
	              0,0,0,0,0,0,0,0};
	              

int bird_left[64] = {0,0,0,0,0,0,0,0,
	              0,0,0,0,0,0,0,0,
	              0,0,0,0,0,0,0,0,
	              0,1,0,1,1,1,1,0,
	              1,1,1,1,1,1,1,1,
	              0,0,0,1,1,1,1,0,
	              0,0,0,0,0,1,1,1,
	              0,0,0,0,0,0,1,0};

// Sprite do T-Rex (8x8 simples - exemplo)
int right[96] = {0,0,0,1,1,1,1,1,
                  0,0,0,1,1,0,1,1,
                  0,0,0,1,1,1,1,1,
                  0,0,0,1,1,1,1,1,
                  0,0,0,1,1,1,0,0,
                  1,0,1,1,1,1,1,0,
                  1,0,1,1,1,1,1,1,
                  1,1,1,1,1,1,1,0,
                  0,1,1,1,1,1,1,0,
                  0,0,1,0,0,1,0,0,
                  0,0,1,0,0,1,1,0,
                  0,0,1,1,0,0,0,0};
		  
int left[96] = { 0,0,0,1,1,1,1,1,
                  0,0,0,1,1,0,1,1,
                  0,0,0,1,1,1,1,1,
                  0,0,0,1,1,1,1,1,
                  0,0,0,1,1,1,0,0,
                  1,0,1,1,1,1,1,0,
                  1,0,1,1,1,1,1,1,
                  1,1,1,1,1,1,1,0,
                  0,1,1,1,1,1,1,0,
                  0,0,1,0,0,1,0,0,
                  0,0,1,1,0,1,0,0,
                  0,0,0,0,0,1,1,0};

// Sprite de cacto (5x8)
int cactus_sprite[30] = { 
    1,0,0,0,0,
    1,0,1,0,1,
    0,1,1,0,1,
    0,0,1,1,0,
    0,0,1,0,0,
    0,0,1,0,0
};

int screen_x, screen_y, res_offset, col_offset;

int bird_x = -100;  // começa fora da tela
int bird_y = 150;
int bird_w = 8 * SCALE;
int bird_h = 8 * SCALE;
int bird_timer = 0;
int bird_active = 0;
int bird_direction = 0; // 0 = left, 1 = right
int *current_bird_sprite = bird_left;

int main() {
    volatile int *video_resolution = (int *)(PIXEL_BUF_CTRL_BASE + 0x8);
    screen_x = *video_resolution & 0xFFFF;
    screen_y = (*video_resolution >> 16) & 0xFFFF;

    volatile int *rgb_status = (int *)(RGB_RESAMPLER_BASE);
    int db = get_data_bits(*rgb_status & 0x3F);
    res_offset = (screen_x == 160) ? 1 : 0;
    col_offset = (db == 8) ? 1 : 0;

    short black = resample_rgb(db, BLACK);
    short white = resample_rgb(db, WHITE);
    short green = resample_rgb(db, GREEN);

    clear_screen(black);
    clear_text_screen();

    int trex_x = 30;
    int trex_y = 200;
    int trex_w = 8 * SCALE;
    int trex_h = 12 * SCALE;

    int cactus_x = 320;
    int cactus_y = 200;
    int cactus_w = 5 * SCALE;
    int cactus_h = 6 * SCALE;

    int jump = 0, velocity = 0;
    int gravity = 1;
    int ground = 200;

    int score = 0;
    int state = 0;
    int *current_sprite = right;
    int frame_counter = 0;

    // Passarinho
    int bird_x = -100;
    int bird_y = 150;
    int bird_w = 8 * SCALE;
    int bird_h = 8 * SCALE;
    int bird_timer = 0;
    int bird_active = 0;
    int bird_direction = 0; // 0 = left, 1 = right
    int *current_bird_sprite = bird_left;

    volatile int *key_ptr = (int *)KEY_BASE;

    draw_ground(white);

    while (1) {
        int keys = *key_ptr;

        if (state == 0) {
            clear_screen(black);
            clear_text_screen();
            video_text(30, 15, "PRESSIONE KEY2");
            video_text(27, 18, "PARA INICIAR O JOGO");

            if (keys & 0x2) {
                state = 1;
                trex_y = ground;
                cactus_x = 320;
                jump = 0;
                velocity = 0;
                score = 0;

                // reset passarinho
                bird_x = -100;
                bird_timer = 0;
                bird_active = 0;
                bird_direction = 0;
                current_bird_sprite = bird_left;
            }
        }
        else if (state == 1) {
            score += 1;

            if ((keys & 0x2) && jump == 0) {
                jump = 1;
                velocity = -10;
            }

            if (jump) {
                trex_y += velocity;
                velocity += gravity;
                if (trex_y >= ground) {
                    trex_y = ground;
                    jump = 0;
                }
            }

            // Cacto
            cactus_x -= 6;
            if (cactus_x < -10) cactus_x = 320;

            // Colisão com cacto
            if (check_collision(trex_x, trex_y, trex_w, trex_h,
                                cactus_x, cactus_y, cactus_w, cactus_h)) {
                state = 2;
            }

            // Frame do T-Rex
            frame_counter++;
            if (frame_counter >= 10) {
                frame_counter = 0;
                current_sprite = (current_sprite == right) ? left : right;
                current_bird_sprite = (current_bird_sprite == bird_left) ? bird_right : bird_left;
            }

            // Passarinho
            bird_timer++;
            if (!bird_active && bird_timer > 100 && (rand() % 100) < 5) {
                bird_active = 1;
                bird_timer = 0;
                bird_x = 320;
                bird_direction = 1 - bird_direction;
            }

            if (bird_active) {
                bird_x -= 8;
                if (bird_x < -bird_w) {
                    bird_active = 0;
                }

                if (check_collision(trex_x, trex_y, trex_w, trex_h,
                                    bird_x, bird_y, bird_w, bird_h)) {
                    state = 2;
                }
            }

            // Desenho
            clear_screen(black);
            clear_text_screen();
            draw_sprite(trex_x, trex_y, current_sprite, 8, 12, SCALE);
            draw_sprite(cactus_x, cactus_y, cactus_sprite, 5, 6, SCALE);
            if (bird_active) {
                draw_sprite(bird_x, bird_y, current_bird_sprite, 8, 8, SCALE);
            }
            draw_ground(white);

            char txt[30];
            sprintf(txt, "Score: %d", score);
            video_text(33, 20, txt);

            volatile int d;
            for (d = 0; d < 70000; d++);
        }
        else if (state == 2) {
            clear_screen(black);
            clear_text_screen();
            video_text(34, 14, "GAME OVER");
            char txt[30];
            sprintf(txt, "Score: %d", score);
            video_text(33, 16, txt);
            video_text(25, 18, "PRESSIONE KEY2 PARA REINICIAR");

            if (keys & 0x2) {
                state = 0;
            }
        }
    }
}

// Limpa a tela com a cor dada
void clear_screen(short color) {
    video_box(0, 0, STANDARD_X - 1, STANDARD_Y - 1, color);
}

void clear_text_screen(void) {
    int row;
    for (row = 0; row < 30; row++)
        video_text(0, row, "                                                                                ");
}

void draw_sprite(int x, int y, int *sprite, int w, int h, int scale) {
    int i, j, si, sj;
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int pixel_ptr;
    int x_factor = 1 << (res_offset + col_offset);
    int y_factor = 1 << res_offset;
    short color = resample_rgb(get_data_bits(*(int *)(RGB_RESAMPLER_BASE) & 0x3F), WHITE);

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            if (sprite[(h - 1 - i) * w + j]) {  // <== INVERTE A LINHA
                for (si = 0; si < scale; si++) {
                    for (sj = 0; sj < scale; sj++) {
                        int px = x + (j * scale) + sj;
                        int py = y - (i * scale) - si; // <== MUDA O SINAL PARA CIMA
                        px = px / x_factor;
                        py = py / y_factor;
                        pixel_ptr = pixel_buf_ptr + (py << (10 - res_offset - col_offset)) + (px << 1);
                        *(short *)pixel_ptr = color;
                    }
                }
            }
        }
    }
}

void draw_ground(short color) {
    int ground_y = 178 + (8 * SCALE); // y do chão
    video_box(0, ground_y, STANDARD_X - 1, ground_y + 1, color); // linha de 2 pixels
}

// Verifica colisão entre dois retângulos
int check_collision(int x1, int y1, int w1, int h1,
                    int x2, int y2, int w2, int h2) {
    if (x1 < x2 + w2 && x1 + w1 > x2 &&
        y1 < y2 + h2 && y1 + h1 > y2) {
        return 1;
    }
    return 0;
}

void video_text(int x, int y, char *text_ptr) {
    int offset = (y << 7) + x;
    volatile char *character_buffer = (char *)FPGA_CHAR_BASE;
    while (*text_ptr)
        *(character_buffer + offset++) = *text_ptr++;
}

void video_box(int x1, int y1, int x2, int y2, short color) {
    int row, col;
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << res_offset;

    x1 /= x_factor; x2 /= x_factor;
    y1 /= y_factor; y2 /= y_factor;

    for (row = y1; row <= y2; row++) {
        for (col = x1; col <= x2; col++) {
            int pixel_ptr = pixel_buf_ptr +
                            (row << (10 - res_offset - col_offset)) + (col << 1);
            *(short *)pixel_ptr = color;
        }
    }
}

int resample_rgb(int bits, int color) {
    if (bits == 8) {
        color = (((color >> 16) & 0xE0) |
                 ((color >> 11) & 0x1C) |
                 ((color >> 6)  & 0x03));
        return (short)((color << 8) | color);
    } else if (bits == 16) {
        return (short)((((color >> 8) & 0xF800) |
                        ((color >> 5) & 0x07E0) |
                        ((color >> 3) & 0x001F)));
    }
    return (short)color;
}

int get_data_bits(int mode) {
    switch (mode) {
        case 0x0: return 1;
        case 0x7: case 0x11: case 0x31: return 8;
        case 0x12: return 9;
        case 0x14: case 0x33: return 16;
        case 0x17: return 24;
        case 0x19: return 30;
        case 0x32: return 12;
        case 0x37: return 32;
        case 0x39: return 40;
        default: return 16;
    }
}