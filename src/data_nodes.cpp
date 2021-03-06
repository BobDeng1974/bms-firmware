/* LibreSolar Battery Management System firmware
 * Copyright (c) 2016-2018 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "data_nodes.h"

#ifdef __ZEPHYR__
#include <zephyr.h>
#include <drivers/hwinfo.h>
#include <sys/crc.h>
#endif

#include "pcb.h"
#include "config.h"

#include "thingset.h"
#include "bms.h"

#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

const char manufacturer[] = "Libre Solar";
const char device_type[] = DEVICE_TYPE;
const char hardware_version[] = HARDWARE_VERSION;
const char firmware_version[] = "0.1";

static char device_id[9];

static char auth_password[11];

// struct to define ThingSet array node
ArrayInfo cell_voltages_arr = {
    bms_status.cell_voltages, NUM_CELLS_MAX, NUM_CELLS_MAX, TS_T_FLOAT32
};

bool pub_serial_enable = false;
bool pub_can_enable = false;

/**
 * Data Objects
 *
 * IDs from 0x00 to 0x17 consume only 1 byte, so they are reserved for output data
 * objects communicated very often (to lower the data rate for LoRa and CAN)
 *
 * Normal priority data objects (consuming 2 or more bytes) start from IDs > 23 = 0x17
 */
static DataNode data_nodes[] = {

    // DEVICE INFORMATION /////////////////////////////////////////////////////
    // using IDs >= 0x18

    TS_NODE_PATH(ID_INFO, "info", 0, NULL),

    TS_NODE_STRING(0x19, "DeviceID", device_id, sizeof(device_id),
        ID_INFO, TS_ANY_R | TS_MKR_W, PUB_NVM),

    TS_NODE_STRING(0x1A, "Manufacturer", manufacturer, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_NODE_STRING(0x1B, "DeviceType", device_type, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_NODE_STRING(0x1C, "HardwareVersion", hardware_version, 0,
        ID_INFO, TS_ANY_R, 0),

    TS_NODE_STRING(0x1D, "FirmwareVersion", firmware_version, 0,
        ID_INFO, TS_ANY_R, 0),

    // CONFIGURATION //////////////////////////////////////////////////////////
    // using IDs >= 0x30 except for high priority data objects

    TS_NODE_PATH(ID_CONF, "conf", 0, &data_nodes_update_conf),

    // general battery settings

    TS_NODE_FLOAT(0x31, "BatNom_Ah", &bms_conf.nominal_capacity_Ah, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // current limits

    TS_NODE_FLOAT(0x40, "PcbDisSC_A", &bms_conf.dis_sc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT32(0x41, "PcbDisSC_us", &bms_conf.dis_sc_delay_us,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x42, "BatDisLim_A", &bms_conf.dis_oc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT32(0x43, "BatDisLimDelay_ms", &bms_conf.dis_oc_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x44, "BatChgLim_A", &bms_conf.chg_oc_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT32(0x45, "BatChgLimDelay_ms", &bms_conf.chg_oc_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // temperature limits

    TS_NODE_FLOAT(0x48, "DisUpLim_degC", &bms_conf.dis_ot_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x49, "DisLowLim_degC", &bms_conf.dis_ut_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x4A, "ChgUpLim_degC", &bms_conf.chg_ot_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x4B, "ChgLowLim_degC", &bms_conf.chg_ut_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x4C, "TempLimHyst_degC", &bms_conf.t_limit_hyst, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // voltage limits

    TS_NODE_FLOAT(0x50, "CellUpLim_V", &bms_conf.cell_ov_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x51, "CellUpLimReset_V", &bms_conf.cell_ov_reset, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT32(0x52, "CellUpLimDelay_ms", &bms_conf.cell_ov_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x53, "CellLowLim_V", &bms_conf.cell_uv_limit, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x54, "CellLowLimReset_V", &bms_conf.cell_uv_reset, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT32(0x55, "CellLowLimDelay_ms", &bms_conf.cell_uv_delay_ms,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // balancing

    TS_NODE_BOOL(0x58, "AutoBalEn", &bms_conf.auto_balancing_enabled,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x59, "BalCellDiff_V", &bms_conf.bal_cell_voltage_diff, 3,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x5A, "BalCellLowLim_V", &bms_conf.bal_cell_voltage_min, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_UINT16(0x5B, "BalIdleDelay_s", &bms_conf.bal_idle_delay,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x5C, "BalIdleTh_A", &bms_conf.bal_idle_current, 1,
        ID_CONF, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // INPUT DATA /////////////////////////////////////////////////////////////
    // using IDs >= 0x60

    TS_NODE_PATH(ID_INPUT, "input", 0, NULL),

    //TS_NODE_BOOL(0x61, "ChgEn", &bms.chg_enabled_target,
    //    ID_INPUT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    //TS_NODE_BOOL(0x62, "DisEn", &bms.dis_enabled_target,
    //    ID_INPUT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // OUTPUT DATA ////////////////////////////////////////////////////////////
    // using IDs >= 0x70 except for high priority data objects

    TS_NODE_PATH(ID_OUTPUT, "output", 0, NULL),

    TS_NODE_FLOAT(0x71, "Bat_V", &bms_status.pack_voltage, 2,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_FLOAT(0x72, "Bat_A", &bms_status.pack_current, 2,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_FLOAT(0x73, "Bat_degC", &bms_status.bat_temp_avg, 1,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_FLOAT(0x74, "IC_degC", &bms_status.ic_temp, 1,
        ID_OUTPUT, TS_ANY_R, 0),

    //TS_NODE_FLOAT(0x75, "MCU_degC", &mcu_temp, 1,
    //    ID_OUTPUT, TS_ANY_R, 0),

#if defined(PIN_ADC_TEMP_FETS) || defined(MOSFET_TEMP_SENSOR)
    TS_NODE_FLOAT(0x76, "MOSFETs_degC", &bms_status.mosfet_temp, 1,
        ID_OUTPUT, TS_ANY_R, 0),
#endif

    TS_NODE_UINT16(0x7C, "SOC_pct", &bms_status.soc,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_UINT32(0x7E, "ErrorFlags", &bms_status.error_flags,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_UINT16(0x7F, "BmsState", &bms_status.state,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_ARRAY(0x80, "Cells_V", &cell_voltages_arr, 3,
        ID_OUTPUT, TS_ANY_R, 0),

    TS_NODE_FLOAT(0x9A, "CellAvg_V", &bms_status.cell_voltage_avg, 3,
        ID_OUTPUT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x9B, "CellMin_V", &bms_status.cell_voltage_min, 3,
        ID_OUTPUT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    TS_NODE_FLOAT(0x9C, "CellMax_V", &bms_status.cell_voltage_max, 3,
        ID_OUTPUT, TS_ANY_R | TS_ANY_W, PUB_NVM),

    // RECORDED DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xA0

    TS_NODE_PATH(ID_REC, "rec", 0, NULL),

    // CALIBRATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xD0

    TS_NODE_PATH(ID_CAL, "cal", 0, NULL),

    // FUNCTION CALLS (EXEC) //////////////////////////////////////////////////
    // using IDs >= 0xE0

    TS_NODE_PATH(ID_EXEC, "exec", 0, NULL),

    TS_NODE_EXEC(0xE1, "shutdown", &bms_shutdown, ID_EXEC, TS_ANY_RW),
    //TS_NODE_EXEC(0xE2, "reset", &reset_device, ID_EXEC, TS_ANY_RW),
    //TS_NODE_EXEC(0xE3, "bootloader-stm", &start_stm32_bootloader, ID_EXEC, TS_ANY_RW),
    //TS_NODE_EXEC(0xE4, "save-settings", &eeprom_store_data, ID_EXEC, TS_ANY_RW),

    TS_NODE_EXEC(0xEE, "auth", &thingset_auth, 0, TS_ANY_RW),
    TS_NODE_STRING(0xEF, "Password", auth_password, sizeof(auth_password), 0xEE, TS_ANY_RW, 0),

    // PUBLICATION DATA ///////////////////////////////////////////////////////
    // using IDs >= 0xF0

    TS_NODE_PATH(ID_PUB, "pub", 0, NULL),

    TS_NODE_PATH(0xF1, "serial", ID_PUB, NULL),
    TS_NODE_BOOL(0xF2, "Enable", &pub_serial_enable, 0xF1, TS_ANY_RW, 0),
    TS_NODE_PUBSUB(0xF3, "IDs", PUB_SER, 0xF1, TS_ANY_RW, 0),

    TS_NODE_PATH(0xF5, "can", ID_PUB, NULL),
    TS_NODE_BOOL(0xF6, "Enable", &pub_can_enable, 0xF5, TS_ANY_RW, 0),
    TS_NODE_PUBSUB(0xF7, "IDs", PUB_CAN, 0xF5, TS_ANY_RW, 0),
};

ThingSet ts(data_nodes, sizeof(data_nodes)/sizeof(DataNode));

void data_nodes_update_conf()
{
    // ToDo: Store changes in EEPROM
}

void data_nodes_init()
{
#ifdef __ZEPHYR__
    uint8_t buf[12];
    hwinfo_get_device_id(buf, sizeof(buf));

    uint64_t id64 = crc32_ieee(buf, sizeof(buf));
    id64 += ((uint64_t)CONFIG_LIBRE_SOLAR_TYPE_ID) << 32;

    uint64_to_base32(id64, device_id, sizeof(device_id), alphabet_crockford);
#endif

    // ToDo: Read data from EEPROM
}

void thingset_auth()
{
    static const char pass_exp[] = CONFIG_THINGSET_EXPERT_PASSWORD;
    static const char pass_mkr[] = CONFIG_THINGSET_MAKER_PASSWORD;

    if (strlen(pass_exp) == strlen(auth_password) &&
        strncmp(auth_password, pass_exp, strlen(pass_exp)) == 0)
    {
        printf("Authenticated as expert user\n");
        ts.set_authentication(TS_EXP_MASK | TS_USR_MASK);
    }
    else if (strlen(pass_mkr) == strlen(auth_password) &&
        strncmp(auth_password, pass_mkr, strlen(pass_mkr)) == 0)
    {
        printf("Authenticated as maker\n");
        ts.set_authentication(TS_MKR_MASK | TS_USR_MASK);
    }
    else {
        printf("Reset authentication\n");
        ts.set_authentication(TS_USR_MASK);
    }
}

void uint64_to_base32(uint64_t in, char *out, size_t size, const char *alphabet)
{
    // 13 is the maximum number of characters needed to encode 64-bit variable to base32
    int len = (size > 13) ? 13 : size;

    // find out actual length of output string
    for (int i = 0; i < len; i++) {
        if ((in >> (i * 5)) == 0) {
            len = i;
            break;
        }
    }

    for (int i = 0; i < len; i++) {
        out[len-i-1] = alphabet[(in >> (i * 5)) % 32];
    }
    out[len] = '\0';
}
