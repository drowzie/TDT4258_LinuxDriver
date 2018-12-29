void init_framebuffer();
void refresh_screen();

struct player_1 {
    int pos_x: 5;
    int pos_y: 95;
    int width: 10;
    int height: 50;
} player_1;

struct player_2 {
    int pos_x: 305;
    int pos_y: 95;
    int width: 10;
    int height: 50;
} player_2;

struct ball {
    int side_length: 10;
    int pos_x: 155;
    int pos_y: 115;
} ball;
