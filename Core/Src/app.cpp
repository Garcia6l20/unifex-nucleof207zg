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

#include <usb.hpp>

extern "C" {
#include <main.h>
}

namespace stm32 {
class gpio {
public:
	GPIO_TypeDef *port_;
	uint16_t pin_;

	void toogle() const noexcept {
		HAL_GPIO_TogglePin(port_, pin_);
	}

	void operator=(bool on) const noexcept {
		HAL_GPIO_WritePin(port_, pin_, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}

	operator bool() const noexcept {
		return HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET;
	}
};
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

    auto led_toggler = [&scheduler](stm32::gpio const&gpio, auto delay) -> task<void> {
		while (true) {
			co_await schedule_at(scheduler, now(scheduler) + delay);
			gpio.toogle();
		}
    };

    auto usb = stm32::usb{};

    auto green_led = stm32::gpio{LD1_GPIO_Port, LD1_Pin};
    auto blue_led = stm32::gpio{LD2_GPIO_Port, LD2_Pin};
    auto red_led = stm32::gpio{LD3_GPIO_Port, LD3_Pin};
	auto user_btn = stm32::gpio{USER_Btn_GPIO_Port, USER_Btn_Pin};

    sync_wait(when_all(
    	[&]() -> task<void> {
    		while(true) {
    			auto data = co_await usb.receive();

				// Within IRQ

				std::string str{data.data(), data.size()}; // copy to local stack
				co_await schedule(scheduler); // schedule for main-loop processing

				// Within main loop

				usb.write(str);
    		}
    	}(),
		[&]() -> task<void> {
    		while (true) {
    			green_led = bool(user_btn);
    			co_await schedule_at(scheduler, now(scheduler) + 250ms);
    		}
    	}(),
		led_toggler(blue_led, 1s),
		led_toggler(red_led, 2s),
		[&]() -> task<void> {

			ctx.run();

			co_return;
		}()));

	return 0;

}

