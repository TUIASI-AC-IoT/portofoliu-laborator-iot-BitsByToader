/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sl_bt_api.h"
#include "sl_main_init.h"
#include "app_assert.h"
#include "app.h"
#include "app_log.h" // Adăugat pentru funcția app_log()

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Application Init.
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

// Application Process Action.
void app_process_action(void)
{
  if (app_is_process_required()) {
    /////////////////////////////////////////////////////////////////////////////
    // Put your additional application code here!                              //
    // This is will run each time app_proceed() is called.                     //
    // Do not call blocking functions from here!                               //
    /////////////////////////////////////////////////////////////////////////////
  }
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the default weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    case sl_bt_evt_system_boot_id:
      app_log("Sistemul a pornit. Se initializeaza radioul...\r\n");

      // --- LOGICA ORIGINALA DE ADVERTISING (PĂSTRATĂ) ---
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, 160, 0, 0);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);

      // --- TODO: START SCANARE PENTRU IBEACON ---
      // Setam parametrii de scanare (PHY de 1M, interval si fereastra default)
      sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_phy_1m, 200, 200);
      app_assert_status(sc);

      // Pornim scanarea pasiva
      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m, sl_bt_scanner_discover_generic);
      app_assert_status(sc);

      app_log("Scanarea a fost pornita!\r\n");
      break;

    // -------------------------------
    // Evenimentul generat la receptia unui pachet tip Advertise
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
    {
      // Extragem datele din payload, lungimea sa si puterea semnalului receptionat (RSSI)
      uint8_t *payload = evt->data.evt_scanner_legacy_advertisement_report.data.data;
      uint8_t payload_len = evt->data.evt_scanner_legacy_advertisement_report.data.len;
      int8_t rssi = evt->data.evt_scanner_legacy_advertisement_report.rssi;

      // Iteram prin payload pentru a gasi structura de tip Manufacturer Specific Data (0xFF)
      uint8_t i = 0;
      while (i < payload_len) {
        uint8_t ad_len = payload[i];
        if (ad_len == 0) break; // Am ajuns la final

        uint8_t ad_type = payload[i + 1];

        // Daca tipul este 0xFF (Manufacturer Specific Data)
        if (ad_type == 0xFF) {
          // Verificam identificatorul Apple (0x4C 0x00), Tipul iBeacon (0x02) si Lungimea (0x15)
          if (ad_len >= 26 &&
              payload[i + 2] == 0x4C &&
              payload[i + 3] == 0x00 &&
              payload[i + 4] == 0x02 &&
              payload[i + 5] == 0x15) {

            // Verificam daca UUID-ul se potriveste cu aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
            uint8_t target_uuid[16] = {0xaa, 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xcc, 0xcc,
                                       0xdd, 0xdd, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee};

            bool match = true;
            for (int j = 0; j < 16; j++) {
              if (payload[i + 6 + j] != target_uuid[j]) {
                match = false;
                break;
              }
            }

            if (match) {
              // Extragem restul datelor - atentie, iBeacon trimite in format Big-Endian
              uint16_t major = (payload[i + 22] << 8) | payload[i + 23];
              uint16_t minor = (payload[i + 24] << 8) | payload[i + 25];
              int8_t tx_power = (int8_t)payload[i + 26];

              // Verificam distanta (mai mica de 1m)
              // Daca valoarea curenta e mai mare ca puterea setata de referinta (ex. -50 > -65)
              if (rssi > tx_power) {
                // Stabilim numele programului master
                char *program = "Necunoscut";
                if (major == 0) program = "CI";
                else if (major == 1) program = "SSC";

                // Afisam mesajul cerut (am pus Colegul X pentru ca aplicatia nu are de unde stii numele tau direct)
                app_log("Salut, colegul numarul %d de la programul %s!\r\n", minor, program);
              }
            }
          }
        }
        // Sarim la urmatoarea structura de date (AD Data) adunand lungimea curenta + 1 byte de lungime
        i += ad_len + 1;
      }
      break;
    }

    // -------------------------------
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    case sl_bt_evt_connection_closed_id:
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    default:
      break;
  }
}
