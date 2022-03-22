/** @file eth.hpp
 *
 * @date Mar 21, 2022
 * @author Sylvain Garcia
 */

#pragma once

#include <cstdlib>
#include <cstddef>
#include <cstring>

#include <exception>
#include <span>

extern "C" {
#include <stm32f2xx_hal.h>
}

namespace stm32 {

template <size_t tx_buffer_count = 1u, size_t rx_buffer_count = 2u>
class eth {
	inline static eth *instance_ = nullptr;

	ETH_HandleTypeDef *handle_;

	ETH_DMADescTypeDef rx_descs_[rx_buffer_count];
	uint8_t rx_buffers_[rx_buffer_count][ETH_RX_BUF_SIZE];

	ETH_DMADescTypeDef tx_descs_[tx_buffer_count];
	uint8_t tx_buffers_[tx_buffer_count][ETH_TX_BUF_SIZE];

	void on_rx_complete() noexcept {
		if (rx_notify_ && rx_notify_self_) {
			rx_notify_(rx_notify_self_,
					std::as_bytes(std::span{
						rx_buffers_[handle_->RxFrameInfos.buffer],
						handle_->RxFrameInfos.length
					}));
		}
	}

	static void _on_rx_complete(ETH_HandleTypeDef *) {
		if (instance_) {
			instance_->on_rx_complete();
		}
	}

	void on_tx_complete() noexcept {
		if (tx_notify_ && tx_notify_self_) {
			tx_notify_(tx_notify_self_);
		}
	}

	static void _on_tx_complete(ETH_HandleTypeDef *) {
		if (instance_) {
			instance_->on_tx_complete();
		}
	}
public:
	eth(ETH_HandleTypeDef *handle) :
		handle_{handle} {

		if (instance_) {
			throw std::exception();
		}
		instance_ = this;

		if (HAL_ETH_DMARxDescListInit(handle_, rx_descs_, &rx_buffers_[0u][0u], rx_buffer_count) != HAL_OK) {
			throw std::exception();
		}
		if (HAL_ETH_DMATxDescListInit(handle_, tx_descs_, &tx_buffers_[0u][0u], tx_buffer_count) != HAL_OK) {
			throw std::exception();
		}

		if (HAL_ETH_RegisterCallback(handle_, HAL_ETH_RX_COMPLETE_CB_ID, &eth::_on_rx_complete) != HAL_OK) {
			throw std::exception();
		}
		if (HAL_ETH_RegisterCallback(handle_, HAL_ETH_TX_COMPLETE_CB_ID, &eth::_on_tx_complete) != HAL_OK) {
			throw std::exception();
		}

		if (HAL_ETH_Start(handle_) != HAL_OK) {
			throw std::exception();
		}

		HAL_NVIC_EnableIRQ(ETH_IRQn);


	}

	inline static void (*tx_notify_)(void *self) = nullptr;
	inline static void *tx_notify_self_ = nullptr;

	inline static void (*rx_notify_)(void *self, std::span<const std::byte> data) = nullptr;
	inline static void *rx_notify_self_ = nullptr;

	struct tx_sender {

		template <typename Receiver>
		struct operation {
			Receiver receiver_;
			tx_sender &sender_;

			template <typename Receiver2>
			operation(tx_sender &sender, Receiver2 &&r): receiver_{(Receiver2 &&)r}, sender_{sender} {
				sender.driver_.tx_notify_self_ = this;
				sender.driver_.tx_notify_ = &operation::on_complete;
			}

			static void on_complete(void *self) {
				auto me = static_cast<operation*>(self);
				me->sender_.driver_.tx_notify_self_ = nullptr;
				me->sender_.driver_.tx_notify_ = nullptr;
				unifex::set_value(static_cast<Receiver&&>(me->receiver_), size_t(me->sender_.data_.size()));
			}

			void start() noexcept {
				std::memcpy(sender_.driver_.tx_buffers_[0u], sender_.data_.data(), sender_.data_.size());
				if (HAL_ETH_TransmitFrame(sender_.driver_.handle_, sender_.data_.size()) != HAL_OK) {
					unifex::set_error(static_cast<Receiver&&>(receiver_), std::make_error_code(std::errc::invalid_argument));
				}
			}
		};

		template <
			template <typename...> class Variant,
			template <typename...> class Tuple>
		using value_types = Variant<Tuple<size_t>>;

		template <template <typename...> class Variant>
		using error_types = Variant<std::error_code, std::exception_ptr>;

		static constexpr bool sends_done = true;

		eth &driver_;
		std::span<const std::byte> data_;

		tx_sender(eth &driver, std::span<const std::byte> data) :
			driver_{driver}, data_{data} {
		}

		template <typename Receiver>
		operation<std::remove_cvref_t<Receiver>> connect(Receiver&& r) && {
		  return operation<std::remove_cvref_t<Receiver>>{*this, (Receiver &&) r};
		}
	};
	template <typename Receiver>
	friend struct tx_sender::operation;

	template <typename T, size_t Ex>
	auto send(std::span<T, Ex> data) {
		return tx_sender{*this, std::as_bytes(data)};
	}


};

}
