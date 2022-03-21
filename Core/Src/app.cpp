/*
 * app.cpp
 *
 *  Created on: Mar 21, 2022
 *      Author: fespresta1
 */

#include <unifex/task.hpp>
#include <unifex/when_all.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/scheduler_concepts.hpp>

#include <unifex/stm32/stm32_bare_context.hpp>

extern "C" {
#include <main.h>
}

using unifex::task;
using unifex::when_all;
using unifex::schedule_at;
using unifex::schedule;
using unifex::now;
using unifex::sync_wait;

using namespace std::literals::chrono_literals;

extern "C" int application(void) {

	unifex::stm32_bare_context ctx{};
    auto scheduler = ctx.get_scheduler();

    auto led_toggler = [&scheduler](auto port, auto pin, auto delay) -> task<void> {
		while (true) {
			co_await schedule_at(scheduler, now(scheduler) + delay);
			HAL_GPIO_TogglePin(port, pin);
		}
		co_return;
    };

    sync_wait(when_all(
		led_toggler(LD1_GPIO_Port, LD1_Pin, 500ms),
		led_toggler(LD2_GPIO_Port, LD2_Pin, 1s),
		led_toggler(LD3_GPIO_Port, LD3_Pin, 2s),
		[&]() -> task<void> {

			ctx.run();

			co_return;
		}()));

	return 0;

}

