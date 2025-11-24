#include "lvgl.h"
#include "spdlog/spdlog.h"
#include <iostream>

int main() {
    lv_init();

    // Create a simple subject
    lv_subject_t subject;
    lv_subject_init_int(&subject, 0);

    int callback_count = 0;

    // Add an observer
    auto observer_cb = [](lv_observer_t* observer, lv_subject_t* subj) {
        int* count_ptr = static_cast<int*>(lv_observer_get_user_data(observer));
        (*count_ptr)++;
        spdlog::info("Observer fired! Count is now: {}", *count_ptr);
    };

    spdlog::info("Adding observer to subject...");
    lv_subject_add_observer(&subject, observer_cb, &callback_count);
    spdlog::info("Observer added. callback_count = {}", callback_count);

    spdlog::info("Setting subject value to 42...");
    lv_subject_set_int(&subject, 42);
    spdlog::info("Value set. callback_count = {}", callback_count);

    spdlog::info("Final callback_count: {}", callback_count);

    if (callback_count == 1) {
        std::cout << "PASS: Observer fired exactly once\n";
        return 0;
    } else {
        std::cout << "FAIL: Observer fired " << callback_count << " times instead of 1\n";
        return 1;
    }
}
