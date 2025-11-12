#include "display_task.h"

#include "../core/common.h"
#include "../core/semaphore_guard.h"

#include "display_layouts.h"

// We only include the library if the display is enabled.
#if ENABLE_DISPLAY
#include <TFT_eSPI.h> 
#endif

// The constructor still runs, but its contents are skipped
DisplayTask::DisplayTask(SplitflapTask& splitflap_task, const uint8_t task_core) : Task<DisplayTask>("Display", 6000, 1, task_core), splitflap_task_(splitflap_task), semaphore_(xSemaphoreCreateMutex()) {
    assert(semaphore_ != NULL);
    xSemaphoreGive(semaphore_);
}

DisplayTask::~DisplayTask() {
  if (semaphore_ != NULL) {
    vSemaphoreDelete(semaphore_);
  }
}


static const int32_t X_OFFSET = 10;
static const int32_t Y_OFFSET = 10;

// The run() function still exists, but its contents are skipped
void DisplayTask::run() {
    // --- WRAP THE ENTIRE CONTENTS ---
    #if ENABLE_DISPLAY
    tft_.begin();
    tft_.invertDisplay(1);
    tft_.setRotation(1);

    tft_.setTextFont(0);
    tft_.setTextColor(0xFFFF, TFT_BLACK);

    tft_.fillScreen(TFT_BLACK);

    // Automatically scale display based on DISPLAY_COLUMNS (see display_layouts.h)
    int32_t module_width = 20;
    int32_t module_height = 26;
    uint8_t module_text_size = 3;

    uint8_t rows = ((NUM_MODULES + DISPLAY_COLUMNS - 1) / DISPLAY_COLUMNS);

    if (DISPLAY_COLUMNS > 16 || rows > 6) {
        module_width = 7;
        module_height = 10;
        module_text_size = 1;
    } else if (DISPLAY_COLUMNS > 10 || rows > 4) {
        module_width = 14;
        module_height = 18;
        module_text_size = 2;
    }

    tft_.fillRect(X_OFFSET, Y_OFFSET, DISPLAY_COLUMNS * (module_width + 1) + 1, rows * (module_height + 1) + 1, 0x2104);

    uint8_t module_row, module_col;
    int32_t module_x, module_y;
    SplitflapState last_state = {};
    String last_messages[countof(messages_)] = {};
    while(1) {
        SplitflapState state = splitflap_task_.getState();
        if (state != last_state) {
            tft_.setTextSize(module_text_size);
            for (uint8_t i = 0; i < NUM_MODULES; i++) {
                SplitflapModuleState& s = state.modules[i];
                if (s == last_state.modules[i]) {
                    continue;
                }

                uint16_t background = 0x0000;
                uint16_t foreground = 0xFFFF;

                bool blink = (millis() / 400) % 2;

                char c;
                switch (s.state) {
                    case NORMAL:
                        c = flaps[s.flap_index];
                        if (s.moving) {
                            // use a dimmer color when moving
                            foreground = 0x6b4d;
                        }
                        break;
                    case PANIC:
                        c = '~';
                        background = blink ? 0xD000 : 0;
                        break;
                    case STATE_DISABLED:
                        c = '*';
                        break;
                    case LOOK_FOR_HOME:
                        c = '?';
                        background = blink ? 0x6018 : 0;
                        break;
                    case SENSOR_ERROR:
                        c = ' ';
                        background = blink ? 0xD461 : 0;
                        break;
                    default:
                        c = ' ';
                        break;
                }
                getLayoutPosition(i, &module_row, &module_col);

                // Add 1 to width/height as a separator line between modules
                module_x = X_OFFSET + 1 + module_col * (module_width + 1);
                module_y = Y_OFFSET + 1 + module_row * (module_height + 1);

                tft_.setTextColor(foreground, background);
                tft_.fillRect(module_x, module_y, module_width, module_height, background);
                tft_.setCursor(module_x + 1, module_y + 2);
                tft_.printf("%c", c);
            }
            last_state = state;
        }

        const int message_height = 10;
        const int message_text_size = 1;
        bool redraw_messages = false;
        {
            SemaphoreGuard lock(semaphore_);
            for (uint8_t i = 0; i < countof(messages_); i++) {
                if (messages_[i] != last_messages[i]) {
                    redraw_messages = true;
                    last_messages[i] = messages_[i];
                }
            }
        }
        if (redraw_messages) {
            tft_.setTextSize(message_text_size);
            tft_.setTextColor(TFT_WHITE, TFT_BLACK);
            tft_.fillRect(0, tft_.height() - message_height * countof(messages_), tft_.width(), message_height * countof(messages_), TFT_BLACK);
            for (uint8_t i = 0; i < countof(messages_); i++) {
                int y = tft_.height() - message_height * (countof(messages_) - i);
                tft_.drawString(last_messages[i], 2, y);
            }
        }

        delay(10);
    }
    #endif // --- END OF WRAP ---
}

// The setMessage() function still exists, but its contents are skipped
void DisplayTask::setMessage(uint8_t i, String message) {
    // --- WRAP THE CONTENTS ---
    #if ENABLE_DISPLAY
    SemaphoreGuard lock(semaphore_);
    assert(i < countof(messages_));
    messages_[i] = message;
    #endif
}