/*
 * app.cpp
 *
 *  Created on: Mar 21, 2022
 *      Author: fespresta1
 */

#include <unifex/task.hpp>
#include <unifex/when_all.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/stop_when.hpp>
#include <unifex/scheduler_concepts.hpp>

#include <unifex/stm32/stm32_bare_context.hpp>

#include <usb.hpp>
#include <gpio.hpp>

#include <g6/router.hpp>

#include <cstring>

#include <memory_resource>


extern "C" {
#include <main.h>
}

using unifex::task;
using unifex::when_all;
using unifex::schedule_after;
using unifex::schedule_at;
using unifex::schedule;
using unifex::now;
using unifex::sync_wait;

using namespace std::literals::chrono_literals;

extern "C" int application(void) {

	unifex::stm32_bare_context ctx{};
    auto scheduler = ctx.get_scheduler();

    auto led_toggler = [&scheduler](stm32::gpio const&gpio, auto const &delay) -> task<void> {
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

	std::chrono::milliseconds red_delay = 500ms;

	g6::router::router commands_router{
	    g6::router::on<R"(echo (\w+)\r\n)">([](const std::string &value) -> std::string {
	        return value + "\r\n";
	    }),
		g6::router::on<R"(set-led (\w+)\r\n)">([&blue_led](const std::string &value) -> std::string {
	    	blue_led = value.starts_with("on");
			return "ok\r\n";
		}),
		g6::router::on<R"(red-delay (\d+)\r\n)">([&red_delay](int value) -> std::string {
	    	red_delay = std::chrono::milliseconds{value};
			return "ok\r\n";
		}),
	    g6::router::on<R"(.*)">([]() -> std::string {
	        return "no such command\r\n";
	    })};

    sync_wait(when_all(
    	[&]() -> task<void> {

			std::array<std::byte, 1024u> cmd_data;
			std::pmr::monotonic_buffer_resource pool{ cmd_data.data(), cmd_data.size() };
			std::pmr::string str{&pool};

    		while(true) {

    			auto data = co_await (usb.receive() | unifex::stop_when(schedule_after(scheduler, 1s)));

				// Within IRQ

				if (data.size()) {
					str = data; // copy to local stack
					co_await schedule(scheduler); // schedule for main-loop processing

					// Within main loop

					auto res = commands_router(str);
					usb.write(res);
				}
    		}
    	}(),
		[&]() -> task<void> {
    		while (true) {
    			green_led = bool(user_btn);
    			co_await schedule_at(scheduler, now(scheduler) + 250ms);
    		}
    	}(),
		led_toggler(red_led, red_delay),
		[&]() -> task<void> {

			ctx.run();

			co_return;
		}()));

	return 0;

}

