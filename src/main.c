#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <SDL_main.h>
#include <SDL.h>
#include <SDL_audio.h>

#define WIDTH             640
#define HEIGHT            480

#define PADDLE_SPEED     0.1f
#define BALL_X_VELOCITY  0.1f
#define BALL_Y_VELOCITY 0.05f

// global because
SDL_Surface* s_ball;
SDL_Surface* s_left_paddle;
SDL_Surface* s_right_paddle;
SDL_Surface* s_background;
SDL_Surface* s_crown;
SDL_Surface* s_nums[11];

typedef struct {
    SDL_Surface* surface;
    SDL_FRect rect;
    float speed;
    float x_velocity, y_velocity;
} Ball;
Ball ball;

typedef struct {
    SDL_Surface* surface;
    SDL_FRect rect;
} Paddle;
Paddle left_paddle, right_paddle;

typedef struct {
    SDL_FRect render_rec;
    uint8_t score;
} Score;
Score left_score, right_score;

typedef struct {
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    uint8_t* audio_buffer;
    uint32_t buffer_length;
} Sound;
Sound hit_paddle, hit_screen, win_theme;

SDL_Surface* load_surface(const char* path, SDL_Surface* screen_surface) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    SDL_Surface* optimized_surface = NULL;
    if (surface == NULL)
        printf("Couldn't load image \"%s\"; Error: %s", path, SDL_GetError());
    else {
        /* Convert bitmap from 24-bit to 32-bit so the conversion
        doesn't have to be done every time the surface is blitted */
        optimized_surface = SDL_ConvertSurface(surface, screen_surface->format, 0);
        if (optimized_surface == NULL)
            printf("Couldn't optimize image \"%s\", Error: %s", path, SDL_GetError());
    }
    SDL_FreeSurface(surface);
    return optimized_surface;
}

SDL_Surface* load_surfaces(SDL_Surface* screen_surface) {
    s_ball = load_surface(".\\assets\\ball.bmp", screen_surface);
    s_left_paddle = load_surface(".\\assets\\paddle.bmp", screen_surface);
    s_right_paddle = load_surface(".\\assets\\paddle.bmp", screen_surface);
    s_background = load_surface(".\\assets\\background.bmp", screen_surface);
    s_crown = load_surface(".\\assets\\crown.bmp", screen_surface);
    s_nums[0] = load_surface(".\\assets\\0.bmp", screen_surface);
    s_nums[1] = load_surface(".\\assets\\1.bmp", screen_surface);
    s_nums[2] = load_surface(".\\assets\\2.bmp", screen_surface);
    s_nums[3] = load_surface(".\\assets\\3.bmp", screen_surface);
    s_nums[4] = load_surface(".\\assets\\4.bmp", screen_surface);
    s_nums[5] = load_surface(".\\assets\\5.bmp", screen_surface);
    s_nums[6] = load_surface(".\\assets\\6.bmp", screen_surface);
    s_nums[7] = load_surface(".\\assets\\7.bmp", screen_surface);
    s_nums[8] = load_surface(".\\assets\\8.bmp", screen_surface);
    s_nums[9] = load_surface(".\\assets\\9.bmp", screen_surface);
    s_nums[10] = load_surface(".\\assets\\9.bmp", screen_surface);
    // assign these here bc why not?
    ball.surface = s_ball;
    left_paddle.surface = s_left_paddle;
    right_paddle.surface = s_right_paddle;
}

void free_surfaces() {
    SDL_FreeSurface(s_ball);
    SDL_FreeSurface(s_left_paddle);
    SDL_FreeSurface(s_right_paddle);
    SDL_FreeSurface(s_background);
    for (int i = 0; i < 10; i++)
        SDL_FreeSurface(s_nums[i]);
}

void load_audio_device(Sound* sound) {
    printf("using audio device \"%s\"\n", SDL_GetAudioDeviceName(1, 0));
    sound->device = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(1, 0), 0, &sound->spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (sound->device == NULL)
        printf("Couldn't load audio device for sound \"%s\"; Error: %s", SDL_GetError());
}

void load_sound(const char* path, Sound* sound) {
    if (SDL_LoadWAV(path, &sound->spec, &sound->audio_buffer, &sound->buffer_length) == NULL)
        printf("Couldn't load sound \"%s\"; Error: %s", path, SDL_GetError());
}

void load_sounds() {
    load_sound(".\\assets\\hit_paddle.wav", &hit_paddle);
    load_sound(".\\assets\\hit_screen.wav", &hit_screen);
    load_sound(".\\assets\\win_theme.wav", &win_theme);
    load_audio_device(&hit_paddle);
    load_audio_device(&hit_screen);
    load_audio_device(&win_theme);
}

void free_sounds() {
    SDL_FreeWAV(hit_paddle.audio_buffer);
    SDL_FreeWAV(hit_screen.audio_buffer);
    SDL_FreeWAV(win_theme.audio_buffer);
    SDL_CloseAudioDevice(hit_paddle.device);
    SDL_CloseAudioDevice(hit_screen.device);
    SDL_CloseAudioDevice(win_theme.device);
}

void play_sound(Sound* sound) {
    if (SDL_QueueAudio(sound->device, sound->audio_buffer, sound->buffer_length) > 0)
        printf("Couldn't queue sound \"%s\"; Error: %s", SDL_GetError());
    SDL_PauseAudioDevice(sound->device, 0);
    printf("played sound\n");
}

void stop_sound(Sound* sound) {
    SDL_PauseAudioDevice(sound->device, 1);
}

void reflect_ball_from_screen() {
    ball.y_velocity = -ball.y_velocity;
    play_sound(&hit_screen);
}

void reflect_ball_from_paddle(uint8_t* keyboard_state) {
    ball.x_velocity = -ball.x_velocity;
    // Augment y-velocity based on paddle movement
    if (keyboard_state[SDL_SCANCODE_W] || keyboard_state[SDL_SCANCODE_UP])
        ball.y_velocity = ball.y_velocity ? -ball.y_velocity : BALL_Y_VELOCITY;
    else if (keyboard_state[SDL_SCANCODE_S] || keyboard_state[SDL_SCANCODE_DOWN])
        ball.y_velocity = ball.y_velocity ? -ball.y_velocity : BALL_Y_VELOCITY;
    play_sound(&hit_paddle);
}

void resolve_ball_collisions(bool* ball_is_reset, uint8_t* keyboard_state) {
    SDL_FRect dummy;
    if (SDL_IntersectFRect(&ball.rect, &left_paddle.rect, &dummy)) {
        reflect_ball_from_paddle(keyboard_state);
    }
    if (SDL_IntersectFRect(&ball.rect, &right_paddle.rect, &dummy)) {
        reflect_ball_from_paddle(keyboard_state);
    }
    // Reflect off of top and bottom of the screen
    if (ball.rect.y > HEIGHT - ball.rect.h / 2.0f || ball.rect.y < ball.rect.h / 2.0f)
        reflect_ball_from_screen();
}

void resolve_ball_off_screen(bool* ball_is_reset) {
    if (ball.rect.x > WIDTH - ball.rect.w / 2.0f) {
        update_score(1);
        reset_ball(ball_is_reset);
    }
    if (ball.rect.x < ball.rect.w / 2.0f) {
        update_score(2);
        reset_ball(ball_is_reset);
    }
}

// left = 0
void set_off_ball(int direction) {
    ball.speed = 1;
    ball.x_velocity = direction == 0 ? -BALL_X_VELOCITY : BALL_X_VELOCITY;
    play_sound(&hit_screen);
}

// Center ball and reset its velocity
void reset_ball(bool* ball_is_reset) {
    *ball_is_reset = true;
    ball.x_velocity = ball.y_velocity = 0;
    ball.speed = 0;
    ball.rect.x = WIDTH / 2.0f;
    ball.rect.y = HEIGHT / 2.0f;
    ball.rect.w = ball.rect.h = 15.0f;
}

void move_ball(bool* ball_is_reset, uint8_t* keyboard_state) {
    ball.rect.x += ball.x_velocity * ball.speed;
    ball.rect.y += ball.y_velocity * ball.speed;
    resolve_ball_collisions(ball_is_reset, keyboard_state);
}

void reset_paddles() {
    left_paddle.rect.y = right_paddle.rect.y = HEIGHT / 2.0f;
}

void move_paddles(uint8_t* keyboard_state, float delta) {
    // Player 1 Up
    if (keyboard_state[SDL_SCANCODE_W]) {
        left_paddle.rect.y -= (left_paddle.rect.y > left_paddle.rect.h / 2.0f) ? PADDLE_SPEED : 0;
    }
    // Player 1 Down
    else if (keyboard_state[SDL_SCANCODE_S]) {
        left_paddle.rect.y += (left_paddle.rect.y < HEIGHT - left_paddle.rect.h / 2.0f) ? PADDLE_SPEED : 0;
    }
    // Player 2 Up
    if (keyboard_state[SDL_SCANCODE_UP]) {
        right_paddle.rect.y -= (right_paddle.rect.y > right_paddle.rect.h / 2.0f) ? PADDLE_SPEED : 0;
    }
    // Player 2 Down
    else if (keyboard_state[SDL_SCANCODE_DOWN]) {
        right_paddle.rect.y += (right_paddle.rect.y < HEIGHT - right_paddle.rect.h / 2.0f) ? PADDLE_SPEED : 0;
    }
}

void reset_score() {
    left_score.score = right_score.score = 0;
}

void update_score(int player) {
    if (left_score.score > 9 || right_score.score > 9)
        return;
    if (player == 1)
        left_score.score++;
    else if (player == 2)
        right_score.score++;
}

void draw_centered_scaled_surface(SDL_Surface* src_surface, SDL_Surface* dst_surface, SDL_FRect src_rect) {
    // Converting from float to int for rendering
    SDL_Rect centered_rect = {.w = src_rect.w, .h = src_rect.h, .x = src_rect.x, .y = src_rect.y};
    centered_rect.x -= src_rect.w / 2.0f;
    centered_rect.y -= src_rect.h / 2.0f;
    SDL_BlitScaled(src_surface, NULL, dst_surface, &centered_rect);
}

void draw_ball(SDL_Surface* screen_surface) {
    draw_centered_scaled_surface(ball.surface, screen_surface, ball.rect);
}

void draw_paddles(SDL_Surface* screen_surface) {
    draw_centered_scaled_surface(left_paddle.surface, screen_surface, left_paddle.rect);
    draw_centered_scaled_surface(right_paddle.surface, screen_surface, right_paddle.rect);
}

void draw_score(int player, SDL_Surface* screen_surface) {
    if (player == 1) {
        draw_centered_scaled_surface(s_nums[left_score.score], screen_surface, left_score.render_rec);
    }
    else if (player == 2) {
        draw_centered_scaled_surface(s_nums[right_score.score], screen_surface, right_score.render_rec);
    }
}

void draw_background(SDL_Surface* screen_surface) {
    SDL_BlitSurface(s_background, NULL, screen_surface, NULL);
}

void draw_crown(int player, SDL_Surface* screen_surface) {
    SDL_FRect crown_rect = (SDL_FRect){.w = 50.0f, .h = 50.0f, .y = HEIGHT / 2.0f};
    crown_rect.x = player == 1 ? WIDTH / 2.0f - WIDTH / 4.0f : WIDTH / 2.0f + WIDTH / 4.0f;
    draw_centered_scaled_surface(s_crown, screen_surface, crown_rect);
}

// Initialize SDL subsystems
SDL_Window* init_sdl() {
    SDL_Window* win = NULL;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("Error! %s\n", SDL_GetError());
        return NULL;
    }
    win = SDL_CreateWindow("SDL Pong", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN);

    return win;
}

int main(int argc, char* argv[]) {
    SDL_Window* win = init_sdl();
    if (!win)
        return 1;

    SDL_Surface* screen_surface = SDL_GetWindowSurface(win);
    load_surfaces(screen_surface);

    load_sounds();

    left_paddle.rect = (SDL_FRect){.w = 4.0f, .h = 70.0f, .x = 5.0f, .y = HEIGHT / 2.0f};
    right_paddle.rect = (SDL_FRect){.w = 4.0f, .h = 70.0f, .x = WIDTH - 5.0f, .y = HEIGHT / 2.0f};

    left_score.render_rec = (SDL_FRect){.w = 46.0f, .h = 58.0f, .x = WIDTH / 2.0f - 50.0f, .y = 40.0f};
    right_score.render_rec = (SDL_FRect){.w = 46.0f, .h = 58.0f, .x = WIDTH / 2.0f + 50.0f, .y = 40.0f};
    
    // Main loop
    bool exit = false;
    bool game_ended = false;
    int only_play_win_theme_once_counter = 0;
    SDL_Event event;
    uint8_t* keyboard_state;

    // Initialize ball
    bool ball_is_reset;
    reset_ball(&ball_is_reset);
    reset_score();

    uint32_t last_update = SDL_GetTicks();
    while (!exit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    exit = true;
                    break;
            }
        }
        // Poll inputs at the start of each frame
        keyboard_state = SDL_GetKeyboardState(NULL);
        if (game_ended && keyboard_state[SDL_SCANCODE_SPACE]) {
            game_ended = false;
            reset_score();
            reset_paddles();
            reset_ball(&ball_is_reset);
            only_play_win_theme_once_counter = 0;
        }
        if (!game_ended) {
            // Calculate physics, resolve collisions, update score, etc.
            if (ball_is_reset && keyboard_state[SDL_SCANCODE_RETURN]) {
                // choose a random direction to set off
                srand(time(NULL));
                set_off_ball(rand() % 2);
                ball_is_reset = false;
            }

            uint32_t update = SDL_GetTicks();
            float delta = (update - last_update) / 100.0f;

            move_paddles(keyboard_state, delta);
            move_ball(&ball_is_reset, keyboard_state);
            resolve_ball_off_screen(&ball_is_reset);
            
            last_update = update;
        }
        // Render
        if (left_score.score > 9 || right_score.score > 9) {
            game_ended = true;
            if (only_play_win_theme_once_counter++ == 1)
                play_sound(&win_theme);
        }
        draw_background(screen_surface);
        if (!game_ended) {
            draw_paddles(screen_surface);
            draw_ball(screen_surface);
        }
        else
            draw_crown(left_score.score > right_score.score ? 1 : 2, screen_surface);
        draw_score(1, screen_surface);
        draw_score(2, screen_surface);

        SDL_UpdateWindowSurface(win);
    }
    free_surfaces();
    free_sounds();
    return 0;
}