/*
 * usb.cpp
 *
 *  Created on: Mar 21, 2022
 *      Author: fespresta1
 */


#include <usb.hpp>

#include <usbd_cdc_if.h>

namespace stm32 {
void usb::write(std::string_view data) {
	CDC_Transmit_FS(reinterpret_cast<uint8_t*>(const_cast<char*>(data.data())), data.size());
}
}

extern "C" void USB_Notify(uint8_t *data, size_t size) {
	stm32::usb::notify(data, size);
}
