/** @file io_operation_base.hpp
 *
 * @date Mar 24, 2022
 * @author Sylvain Garcia
 */

#pragma once

#include <unifex/manual_lifetime.hpp>
#include <unifex/get_stop_token.hpp>
#include <unifex/receiver_concepts.hpp>

namespace stm32 {

template<class Operation>
concept StartableIoOperation = requires (Operation &op) {
	{op.start_io()};
};

template<class Operation>
concept StoppableIoOperation = requires (Operation &op) {
	{op.stop_io()};
};

template <template<class...> typename Operation, typename Receiver>
class io_operation_base {

    struct cancel_callback {
    	io_operation_base &op_;

        void operator()() noexcept { op_.request_stop(); }
    };

    unifex::manual_lifetime<typename unifex::stop_token_type_t<Receiver>::template callback_type<cancel_callback>>
        stop_callback_{};

    static constexpr bool is_stop_ever_possible = !unifex::is_stop_never_possible_v<unifex::stop_token_type_t<Receiver>>;

	Receiver receiver_;
    bool can_be_cancelled_;

protected:

    io_operation_base(Receiver &&r) noexcept:
		receiver_{(Receiver &&)r},
		can_be_cancelled_{unifex::get_stop_token(receiver_).stop_possible()} {
	}

	template <typename...Args>
	void set_value(Args&&...values) && {
		if constexpr (is_stop_ever_possible) {
			stop_callback_.destruct();
		}
		unifex::set_value(std::move(*this).receiver_, std::forward<Args>(values)...);
	}

public:
	void start() noexcept {
		if constexpr (is_stop_ever_possible) {
			stop_callback_.construct(unifex::get_stop_token(receiver_), cancel_callback{*this});
		}
		if constexpr (StartableIoOperation<Operation<Receiver>>) {
			static_cast<Operation<Receiver>*>(this)->start_io();
		}
	}

	void request_stop() noexcept {
		if constexpr (is_stop_ever_possible) {
			stop_callback_.destruct();
		}
		if constexpr (StoppableIoOperation<Operation<Receiver>>) {
			static_cast<Operation<Receiver>*>(this)->stop_io();
		}
	}
};
}
