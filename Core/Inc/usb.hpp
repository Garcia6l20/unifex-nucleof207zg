/** @file usb.hpp
 *
 * @date Mar 21, 2022
 * @author Sylvain Garcia
 */

#pragma once

#include <unifex/receiver_concepts.hpp>

#include <system_error>

namespace stm32 {
class usb {

public:

	inline static void (*notify_)(void *self, uint8_t *data, size_t size) = nullptr;
	inline static void *notify_self_ = nullptr;

	struct rx_sender {

	    template <typename Receiver>
		struct operation {
	    	Receiver receiver_;
	    	rx_sender &sender_;

		  	template <typename Receiver2>
			operation(rx_sender &sender, Receiver2 &&r): receiver_{(Receiver2 &&)r}, sender_{sender} {
				sender.driver_.notify_self_ = this;
				sender.driver_.notify_ = &operation::on_complete;
		  	}

			static void on_complete(void *self, uint8_t *data, size_t size) {
				auto me = static_cast<operation*>(self);
				me->sender_.driver_.notify_self_ = nullptr;
				me->sender_.driver_.notify_ = nullptr;
				unifex::set_value(static_cast<Receiver&&>(me->receiver_), std::string_view{reinterpret_cast<char*>(data), size});
			}

		    void start() noexcept {}
		};

	    template <
	        template <typename...> class Variant,
	        template <typename...> class Tuple>
	    using value_types = Variant<Tuple<std::string_view>>;

	    template <template <typename...> class Variant>
	    using error_types = Variant<std::error_code, std::exception_ptr>;

	    static constexpr bool sends_done = true;

	    usb &driver_;

	    rx_sender(usb &driver) :
	    	driver_{driver} {
	    }

	    template <typename Receiver>
	    operation<std::remove_cvref_t<Receiver>> connect(Receiver&& r) && {
	      return operation<std::remove_cvref_t<Receiver>>{*this, (Receiver &&) r};
	    }
	};

	auto receive() {
		return rx_sender{*this};
	}

	void write(std::string_view data);

	static void notify(uint8_t *data, size_t size) {
		if (notify_ && notify_self_) {
			notify_(notify_self_, data, size);
		}
	}
};
}
