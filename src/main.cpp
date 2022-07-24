#include <Arduino.h>

#include <TFT_eSPI.h>
#include <lvgl.h>
#include <BleKeyboard/BleKeyboard.h>
#include "FT62XXTouchScreen.h"

// Display buffer
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf[DISPLAY_WIDTH * 10];

/// Number of shortcut columns
#define BTN_X_COUNT 3
/// Number of shortcut rows
#define BTN_Y_COUNT 3
/// Number of tabs
#define BTN_TAB_COUNT BTN_Y_COUNT
/// Shortcut length + 1 byte for terminator
#define SHORTCUT_LENGTH 5

// Additional keys not defined in BleKeyboard.h
const uint8_t KEY_KP1 = 0xE1; // Keypad 1 and End
const uint8_t KEY_KP2 = 0xE2; // Keypad 2 and Down Arrow
const uint8_t KEY_KP3 = 0xE3; // Keypad 3 and PageDn
const uint8_t KEY_KP4 = 0xE4; // Keypad 4 and Left Arrow
const uint8_t KEY_KP5 = 0xE5; // Keypad 5
const uint8_t KEY_KP6 = 0xE6; // Keypad 6 and Right Arrow
const uint8_t KEY_KP7 = 0xE7; // Keypad 7 and Home
const uint8_t KEY_KP8 = 0xE8; // Keypad 8 and Up Arrow
const uint8_t KEY_KP9 = 0xE9; // Keypad 9 and Page Up
const uint8_t KEY_KP0 = 0xEA; // Keypad 0 and Insert
const uint8_t KEY_KP_MINUS = 0xDE; // Keypad -
const uint8_t KEY_KP_PLUS = 0xDF; // Keypad + 

/// Button with image and shortcut
typedef struct {
    lv_obj_t *btn;
    lv_obj_t *img;
    const lv_img_dsc_t *img_desc;
    uint8_t shortcut[SHORTCUT_LENGTH];
} btn_item_t;

// Navigation panel images
extern "C" const lv_img_dsc_t macro;
extern "C" const lv_img_dsc_t jogging;
extern "C" const lv_img_dsc_t run;

// Macros tab images
extern "C" const lv_img_dsc_t m1;
extern "C" const lv_img_dsc_t m2;
extern "C" const lv_img_dsc_t m3;
extern "C" const lv_img_dsc_t m4;
extern "C" const lv_img_dsc_t m5;
extern "C" const lv_img_dsc_t m6;
extern "C" const lv_img_dsc_t m7;
extern "C" const lv_img_dsc_t m8;
extern "C" const lv_img_dsc_t m9;

// Jogging tab images
extern "C" const lv_img_dsc_t step_plus;
extern "C" const lv_img_dsc_t y_plus;
extern "C" const lv_img_dsc_t z_plus;
extern "C" const lv_img_dsc_t x_minus;
extern "C" const lv_img_dsc_t end;
extern "C" const lv_img_dsc_t x_plus;
extern "C" const lv_img_dsc_t step_minus;
extern "C" const lv_img_dsc_t y_minus;
extern "C" const lv_img_dsc_t z_minus;

// Run tab images
extern "C" const lv_img_dsc_t home;
extern "C" const lv_img_dsc_t feed_plus_10;
extern "C" const lv_img_dsc_t feed_plus_1;
extern "C" const lv_img_dsc_t zero;
extern "C" const lv_img_dsc_t feed_minus_10;
extern "C" const lv_img_dsc_t feed_minus_1;
extern "C" const lv_img_dsc_t cycle_start;
extern "C" const lv_img_dsc_t feed_hold;
extern "C" const lv_img_dsc_t stop;

/// Navigation buttons
static btn_item_t btn_nav_array[BTN_TAB_COUNT] = {
    {0, 0, &macro, 0},
    {0, 0, &jogging, 0},
    {0, 0, &run, 0}
};

/// Shortcut buttons
static btn_item_t btn_array[BTN_TAB_COUNT][BTN_Y_COUNT][BTN_X_COUNT] = {
    // Macro tab
    {
        { // Row 1
            {0, 0, &m1, {KEY_F1, 0}},
            {0, 0, &m2, {KEY_F2, 0}},
            {0, 0, &m3, {KEY_F3, 0}}
        },
        { // Row 2
            {0, 0, &m4, {KEY_F4, 0}},
            {0, 0, &m5, {KEY_F5, 0}},
            {0, 0, &m6, {KEY_F6, 0}}
        },
        { // Row 3
            {0, 0, &m7, {KEY_F7, 0}},
            {0, 0, &m8, {KEY_F8, 0}},
            {0, 0, &m9, {KEY_F9, 0}}
        }
    },
    // Jogging tab
    {
        { // Row 1
            {0, 0, &step_plus, {KEY_LEFT_CTRL, 'p', 0}},
            {0, 0, &y_plus, {KEY_UP_ARROW, 0}},
            {0, 0, &z_plus, {KEY_PAGE_UP, 0}}
        },
        { // Row 2
            {0, 0, &x_minus, {KEY_LEFT_ARROW, 0}},
            {0, 0, &end, {KEY_END, 0}},
            {0, 0, &x_plus, {KEY_RIGHT_ARROW, 0}}
        },
        { // Row 3
            {0, 0, &step_minus, {KEY_LEFT_CTRL, 'm', 0}},
            {0, 0, &y_minus, {KEY_DOWN_ARROW, 0}},
            {0, 0, &z_minus, {KEY_PAGE_DOWN, 0}}
        }
    },
    // Run tab
    {
        { // Row 1
            {0, 0, &home, {KEY_KP0, 0}},
            {0, 0, &feed_plus_10, {KEY_KP1, 0}},
            {0, 0, &feed_plus_1, {KEY_KP2, 0}}
        },
        { // Row 2
            {0, 0, &zero, {KEY_KP3, 0}},
            {0, 0, &feed_minus_10, {KEY_KP4, 0}},
            {0, 0, &feed_minus_1, {KEY_KP5, 0}}
        },
        { // Row 3
            {0, 0, &cycle_start, {KEY_KP6, 0}},
            {0, 0, &feed_hold, {KEY_KP7, 0}},
            {0, 0, &stop, {KEY_KP8, 0}}
        }
    }
};

lv_obj_t *screenMain;

static lv_style_t btn_style_default;
static lv_style_t btn_style_default_pressed;

static lv_style_t btn_style_nav;
static lv_style_t btn_style_nav_pressed;

// Backlight PWM
#define BL_PWM_CHANNEL       0
#define BL_PWM_FREQUENCY     5000
#define BL_PWM_RESOLUTION    8

// Three components: keyboard, display and touchscreen
BleKeyboard keyboard;
TFT_eSPI display = TFT_eSPI(DISPLAY_WIDTH, DISPLAY_HEIGHT);
FT62XXTouchScreen touch = FT62XXTouchScreen(DISPLAY_HEIGHT, PIN_SDA, PIN_SCL);

#define DEFAULT_TAB 1

// Selected tab
static int selectedTab;

static void load_tab(int new_tab) {
    if (selectedTab == new_tab)
        return;

    // Switch navigation buttons
    lv_obj_clear_state(btn_nav_array[selectedTab].btn, LV_STATE_USER_1);
    lv_obj_add_state(btn_nav_array[new_tab].btn, LV_STATE_USER_1);

    // Hide old and show new tab
    for (int y = 0; y < BTN_Y_COUNT; y++) {
        for (int x = 0; x < BTN_X_COUNT; x++) {
            btn_item_t* item = &btn_array[selectedTab][y][x];
            lv_obj_add_flag(item->btn, LV_OBJ_FLAG_HIDDEN);
            item = &btn_array[new_tab][y][x];
            lv_obj_clear_flag(item->btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
    selectedTab = new_tab;
}

btn_item_t * find_button(lv_obj_t *obj) {
    for (int y = 0; y < BTN_Y_COUNT; y++) {
        for (int x = 0; x < BTN_X_COUNT; x++) {
            btn_item_t *ret = &btn_array[selectedTab][y][x];
            if (obj == ret->btn)
                return ret;
        }
    }
    return NULL;
}

static void event_handler_btn(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    if (e->code == LV_EVENT_PRESSED)
    {
        if (obj == btn_nav_array[0].btn) {
            load_tab(0);
            return;
        }
        if (obj == btn_nav_array[1].btn) {
            load_tab(1);
            return;
        }
        if (obj == btn_nav_array[2].btn) {
            load_tab(2);
            return;
        }
    }

    if (false == keyboard.isConnected())
        return;

    if (e->code != LV_EVENT_CLICKED)
    {
        return;
    }

    btn_item_t *button = find_button(obj);
    if (button == NULL)
        return;
    
    for (int i = 0; i < SHORTCUT_LENGTH; i++) {
        if (button->shortcut[i] != 0)
            keyboard.press(button->shortcut[i]);
        else
            break;
    }
    for (int i = SHORTCUT_LENGTH - 1; i >= 0; i--) {
        if (button->shortcut[i] == 0)
            continue;

        keyboard.release(button->shortcut[i]);
    }
    keyboard.releaseAll();
}

static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    static uint16_t lastx = 0;
    static uint16_t lasty = 0;

    TouchPoint touchPos = touch.read();
    if (touchPos.touched)
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchPos.xPos;
        data->point.y = touchPos.yPos;
        lastx = touchPos.xPos;
        lasty = touchPos.yPos;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
        data->point.x = lastx;
        data->point.y = lasty;
    }
}

static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    display.startWrite();
    display.setAddrWindow(area->x1, area->y1, w, h);
    display.pushColors(&color_p->full, w * h, true);
    display.endWrite();

    lv_disp_flush_ready(disp);
}

static void init_default_style(bool nav, lv_style_t* style, lv_palette_t color) {
    lv_style_init(style);

    if (nav) {
        lv_style_set_radius(style, 0);
    } else {
        lv_style_set_radius(style, 7);
    }

    /*Set a background color and a radius*/
    lv_style_set_bg_color(style, lv_palette_main(color));
    lv_style_set_line_width(style, 0);

    lv_style_set_border_color(style, lv_palette_main(color));
    lv_style_set_border_width(style, 2);

    /*Only its pointer is saved so must static, global or dynamically allocated */
    static const lv_style_prop_t trans_props[] = {
                                                LV_STYLE_BORDER_WIDTH, LV_STYLE_BORDER_COLOR, LV_STYLE_SHADOW_COLOR, LV_STYLE_SHADOW_WIDTH,
                                                LV_STYLE_BG_OPA, LV_STYLE_PROP_INV, /*End marker*/
    };

    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(&trans, trans_props, lv_anim_path_ease_out, 300, 0, NULL);

    lv_style_set_transition(style, &trans);
}

static void init_pressed_style(bool nav, lv_style_t* style, lv_palette_t color) {
    lv_style_init(style);

    if (nav) {
        lv_style_set_radius(style, 0);
    } else {
        lv_style_set_radius(style, 7);
    }

    lv_style_set_bg_color(style, lv_palette_main(color));
    lv_style_set_border_color(style, lv_palette_lighten(color, 4));
    lv_style_set_border_width(style, 7);
    lv_style_set_bg_opa(style, LV_OPA_50);
    lv_style_set_shadow_width(style, 55);
    lv_style_set_shadow_color(style, lv_palette_main(color));
}

void setup()
{
    static lv_disp_drv_t disp_drv;
    static lv_indev_drv_t indev_drv;

    Serial.begin(115200);
    keyboard.setName("CNC keypad");
    keyboard.begin();
    lv_init();
    Serial.println("Starting BLE work!");

    ledcSetup(BL_PWM_CHANNEL, BL_PWM_FREQUENCY, BL_PWM_RESOLUTION);
    ledcAttachPin(DISPLAY_BL, BL_PWM_CHANNEL);

    // Enable TFT
    display.begin();
    display.setRotation(1);

    // Enable backlight
    ledcWrite(BL_PWM_CHANNEL, 30);

    // Start TouchScreen
    touch.begin();

    // Display Buffer
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, sizeof(buf)/sizeof(lv_color_t));

    // Init display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // Init touch
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);

    // Screen Object
    screenMain = lv_obj_create(NULL);

    static lv_coord_t col_dsc[3];
    static lv_coord_t row_dsc[BTN_Y_COUNT + 1];

    col_dsc[0] = LV_GRID_FR(2);
    col_dsc[1] = LV_GRID_FR(7);
    col_dsc[2] = LV_GRID_TEMPLATE_LAST;

    for (int i = 0; i < BTN_Y_COUNT; i++) {
        row_dsc[i] = LV_GRID_FR(1);
    }
    row_dsc[BTN_Y_COUNT] = LV_GRID_TEMPLATE_LAST;

    static lv_coord_t col_dsc2[BTN_X_COUNT + 1];
    static lv_coord_t row_dsc2[BTN_Y_COUNT + 1];

    for (int i = 0; i < BTN_X_COUNT; i++) {
        col_dsc2[ i] = LV_GRID_FR(1);
    }
    for (int i = 0; i < BTN_Y_COUNT; i++) {
        row_dsc2[i] = LV_GRID_FR(1);
    }
    col_dsc2[BTN_X_COUNT] = LV_GRID_TEMPLATE_LAST;
    row_dsc2[BTN_Y_COUNT] = LV_GRID_TEMPLATE_LAST;

    init_default_style(false, &btn_style_default, LV_PALETTE_BLUE);
    init_pressed_style(false, &btn_style_default_pressed, LV_PALETTE_BLUE);
    init_default_style(true, &btn_style_nav, LV_PALETTE_RED);
    init_pressed_style(true, &btn_style_nav_pressed, LV_PALETTE_RED);

    static lv_style_t bg_style;
    lv_style_init(&bg_style);
    lv_style_set_bg_color(&bg_style, lv_color_black());
    lv_style_set_border_width(&bg_style, 0);

    lv_obj_t* contnav = lv_obj_create(screenMain);
    lv_obj_set_style_grid_column_dsc_array(contnav, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(contnav, row_dsc, 0);
    lv_obj_set_size(contnav, 480, 320);
    lv_obj_center(contnav);
    lv_obj_set_layout(contnav, LV_LAYOUT_GRID);
    lv_obj_add_style(contnav, &bg_style, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(contnav, 0, 0);

    lv_obj_t* cont = lv_obj_create(contnav);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc2, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc2, 0);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_add_style(cont, &bg_style, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_ver(cont, 0, 0);
    lv_obj_set_style_pad_right(cont, 0, 0);
    lv_obj_set_grid_cell(cont, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 3);

    // Initialize navigation buttons
    for (int y = 0; y < BTN_Y_COUNT; y++) {
        btn_nav_array[y].btn = lv_btn_create(contnav);
        lv_obj_add_style(btn_nav_array[y].btn, &btn_style_nav, 0);
        lv_obj_add_style(btn_nav_array[y].btn, &btn_style_nav_pressed, LV_STATE_FOCUSED | LV_STATE_PRESSED);
        lv_obj_add_style(btn_nav_array[y].btn, &btn_style_nav_pressed, LV_STATE_USER_1);

        lv_obj_add_event_cb(btn_nav_array[y].btn, event_handler_btn, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(btn_nav_array[y].btn, event_handler_btn, LV_EVENT_PRESSED, NULL);
        btn_nav_array[y].img = lv_img_create(btn_nav_array[y].btn);
        lv_img_set_src(btn_nav_array[y].img, btn_nav_array[y].img_desc);
        lv_obj_center(btn_nav_array[y].img);
        lv_obj_set_grid_cell(btn_nav_array[y].btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, y, 1);
    }

    // Initialize all shortcut buttons
    for (int z = 0; z < BTN_Y_COUNT; z++) {
        for (int y = 0; y < BTN_Y_COUNT; y++) {
            for (int x = 0; x < BTN_X_COUNT; x++) {
                btn_item_t *item = &btn_array[z][y][x];
                item->btn = lv_btn_create(cont);
                lv_obj_add_flag(item->btn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_style(item->btn, &btn_style_default, 0);
                lv_obj_add_style(item->btn, &btn_style_default_pressed, LV_STATE_FOCUSED | LV_STATE_PRESSED);
                lv_obj_add_event_cb(item->btn, event_handler_btn, LV_EVENT_CLICKED, NULL);
                lv_obj_add_event_cb(item->btn, event_handler_btn, LV_EVENT_PRESSED, NULL);
                item->img = lv_img_create(item->btn);
                lv_img_set_src(item->img, item->img_desc);
                lv_obj_center(item->img);
                lv_obj_set_grid_cell(item->btn, LV_GRID_ALIGN_STRETCH, x , 1, LV_GRID_ALIGN_STRETCH, y, 1);
            }
        }
    }

    lv_obj_clear_state(btn_nav_array[0].btn, LV_STATE_ANY);
    selectedTab = 1 - DEFAULT_TAB;
    load_tab(DEFAULT_TAB);

    // Screen load
    lv_scr_load(screenMain);
}

void loop()
{
    static int last_ms = millis();
    int ms = millis();
    lv_tick_inc(ms - last_ms);
    last_ms = ms;
    lv_timer_handler();
}