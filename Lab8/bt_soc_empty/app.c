/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"

#include "em_cmu.h"
#include "em_gpio.h"

// Includeri necesare pentru configuratia GATT si logging
#include "gatt_db.h"
#include "app_log.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Flag folosit in logica aplicatiei pentru generarea de notificari la buton
bool button_io_notification_enabled = false;

void GPIO_ODD_IRQHandler(void)
{
  // Stergere flag intrerupere
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  // Semnalam stivei BLE un eveniment extern (pentru buton)
  sl_bt_external_signal(1);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  // Activare ramura clock periferic GPIO
  CMU_ClockEnable(cmuClock_GPIO, true);
  // Configurare GPIOA 04 ca iesire (LED)
  GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 1);
  // Configurare GPIOC 07 ca intrare (buton)
  GPIO_PinModeSet(gpioPortC, 7, gpioModeInputPullFilter, 1);
  // Configurare intrerupere pentru buton pe ambele fronturi
  GPIO_ExtIntConfig(gpioPortC, 7, 7, true, true, true);
  // Activare intrerupere
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);

}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Conexiune deschisa!\r\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("Conexiune inchisa. Se reia advertising...\r\n");

      // Resetam flag-ul notificarilor pe deconectare pentru siguranta
      button_io_notification_enabled = false;

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Tratarea scrierilor pe baza de date GATT de catre client
    case sl_bt_evt_gatt_server_attribute_value_id:
      if (gattdb_LED_IO == evt->data.evt_gatt_server_attribute_value.attribute) {
        uint8_t recv_val;
        size_t recv_len;

        // Citire valoare caracteristica LED in urma actualizarii
        sl_bt_gatt_server_read_attribute_value(gattdb_LED_IO,
                                               0,
                                               sizeof(recv_val),
                                               &recv_len,
                                               &recv_val);

        if (recv_val != 0) {
          GPIO_PinOutSet(gpioPortA, 4);
        } else {
          GPIO_PinOutClear(gpioPortA, 4);
        }

        // Log-ul specificat in cerinte
        app_log("LED = %d\r\n", recv_val);
      }
      break;

    // -------------------------------
    // Tratare eveniment activare/dezactivare notificare pentru caracteristica BUTTON
    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (gattdb_BTN_IO == evt->data.evt_gatt_server_characteristic_status.characteristic) {
        if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_notification) {
          app_log("Notificare activata pentru caracteristica BUTTON\r\n");
          button_io_notification_enabled = true;
        } else {
          app_log("Notificare dezactivata pentru caracteristica BUTTON\r\n");
          button_io_notification_enabled = false;
        }
      }
      break;

    // -------------------------------
    // Tratare semnal extern generat de intreruperea de pe buton (GPIO)
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals == 1) {
        uint8_t button_state;

        // Citire noua stare a butonului
        button_state = GPIO_PinInGet(gpioPortC, 7);

        // Actualizam valoarea locala in baza de date
        sl_bt_gatt_server_write_attribute_value(gattdb_BTN_IO,
                                                0,
                                                sizeof(button_state),
                                                &button_state);

        // Trimitem notificarea daca functia de notificare a fost activata de client
        if (button_io_notification_enabled) {
          sl_bt_gatt_server_notify_all(gattdb_BTN_IO,
                                       sizeof(button_state),
                                       &button_state);
        }
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
