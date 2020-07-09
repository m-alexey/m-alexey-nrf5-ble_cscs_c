/**
 * Copyright (c) 2012 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef BLE_CSCS_C_H__
#define BLE_CSCS_C_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "nrf_ble_gq.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief   Macro for defining a ble_�scs_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_CSCS_C_DEF(_name)                                                                       \
static ble_cscs_c_t _name;                                                                          \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_CSCS_C_BLE_OBSERVER_PRIO,                                                  \
                     ble_cscs_c_on_ble_evt, &_name)

/** @brief Macro for defining multiple ble_�scs_c instances.
 *
 * @param   _name   Name of the array of instances.
 * @param   _cnt    Number of instances to define.
 * @hideinitializer
 */
#define BLE_CSCS_C_ARRAY_DEF(_name, _cnt)                 \
static ble_cscs_c_t _name[_cnt];                          \
NRF_SDH_BLE_OBSERVERS(_name ## _obs,                      \
                      BLE_CSCS_C_BLE_OBSERVER_PRIO,       \
                      ble_cscs_c_on_ble_evt, &_name, _cnt)

/**@brief   Structure containing the handles related to the Cycling Speed and Cadence Service found on the peer. */
typedef struct
{
    uint16_t cscs_cccd_handle;                /**< Handle of the CCCD of the Cycling Speed and Cadence characteristic. */
    uint16_t cscs_handle;                     /**< Handle of the Cycling Speed and Cadence characteristic as provided by the SoftDevice. */
    uint16_t cscs_feature_handle;             /**< Handle of the Cycling Speed and Cadence feature characteristic as provided by the SoftDevice. */
    uint16_t cscs_sensloc_handle;             /**< Handle of the Cycling Speed and Cadence sensor loacation characteristic as provided by the SoftDevice. */
} ble_cscs_c_db_t;

/**@brief   CSCS Client event type. */
typedef enum
{
    BLE_CSCS_C_EVT_DISCOVERY_COMPLETE = 1,  /**< Event indicating that the Cycling Speed and Cadence Service has been discovered at the peer. */
    BLE_CSCS_C_EVT_CSM_NOTIFICATION         /**< Event indicating that a notification of the Cycling Speed and Cadence Measurement characteristic has been received from the peer. */
} ble_cscs_c_evt_type_t;

/**@brief   Structure containing the Running Speed and Cadence measurement received from the peer. */
typedef struct
{
    bool        is_wheel_rev_data_present;  /**< True if Wheel Revolution Data is present in the measurement. */
    bool        is_crank_rev_data_present;  /**< True if Crank Revolution Data is present in the measurement. */
    uint32_t    cumulative_wheel_revs;      /**< Cumulative Wheel Revolutions. */
    uint16_t    last_wheel_event_time;      /**< Last Wheel Event Time. */
    uint16_t    cumulative_crank_revs;      /**< Cumulative Crank Revolutions. */
    uint16_t    last_crank_event_time;      /**< Last Crank Event Time. */
} ble_cscs_c_meas_t;

/**@brief   Cycling Speed and Cadence Event structure. */
typedef struct
{
    ble_cscs_c_evt_type_t evt_type;  /**< Type of the event. */
    uint16_t  conn_handle;           /**< Connection handle on which the cscs_c event  occured.*/
    union
    {
        ble_cscs_c_db_t    cscs_db;           /**< Cycling Speed and Cadence Service related handles found on the peer device. This is filled if the evt_type is @ref BLE_CSCS_C_EVT_DISCOVERY_COMPLETE.*/
        ble_cscs_c_meas_t  csc;               /**< Cycling Speed and Cadence measurement received. This is filled if the evt_type is @ref BLE_CSCS_C_EVT_CSM_NOTIFICATION. */
    } params;
} ble_cscs_c_evt_t;

// Forward declaration of the ble_cscs_c_t type.
typedef struct ble_cscs_c_s ble_cscs_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that is to be provided by the application
 *          of this module in order to receive events.
 */
typedef void (* ble_cscs_c_evt_handler_t) (ble_cscs_c_t * p_ble_cscs_c, ble_cscs_c_evt_t * p_evt);

/**@brief   Cycling Speed and Cadence client structure. */
struct ble_cscs_c_s
{
    uint16_t                 conn_handle;   /**< Connection handle as provided by the SoftDevice. */
    ble_cscs_c_db_t          peer_db;       /**< Handles related to CSCS on the peer*/
    ble_cscs_c_evt_handler_t evt_handler;   /**< Application event handler to be called when there is an event related to the Cunning Speed and Cadence service. */
    ble_srv_error_handler_t  error_handler; /**< Function to be called in case of an error. */
    nrf_ble_gq_t           * p_gatt_queue;  /**< Pointer to BLE GATT Queue instance. */
};

/**@brief   Cycling Speed and Cadence client initialization structure. */
typedef struct
{
    ble_cscs_c_evt_handler_t evt_handler;   /**< Event handler to be called by the Cycling Speed and Cadence Client module whenever there is an event related to the Cycling Speed and Cadence Service. */
    ble_srv_error_handler_t  error_handler; /**< Function to be called in case of an error. */
    nrf_ble_gq_t           * p_gatt_queue;  /**< Pointer to BLE GATT Queue instance. */
} ble_cscs_c_init_t;


/**@brief      Function for initializing the Cycling Speed and Cadence Service Client module.
 *
 * @details    This function will initialize the module and set up Database Discovery to discover
 *             the Cycling Speed and Cadence Service. After calling this function, call @ref ble_db_discovery_start
 *             to start discovery once a link with a peer has been established.
 *
 * @param[out] p_ble_cscs_c      Pointer to the CSC Service Client structure.
 * @param[in]  p_ble_cscs_c_init Pointer to the CSC Service initialization structure containing
 *                               the initialization information.
 *
 * @retval     NRF_SUCCESS      Operation success.
 * @retval     NRF_ERROR_NULL   A parameter is NULL.
 * @retval     err_code       	Otherwise, this function propagates the error code returned by @ref ble_db_discovery_evt_register.
 */
uint32_t ble_cscs_c_init(ble_cscs_c_t * p_ble_cscs_c, ble_cscs_c_init_t * p_ble_cscs_c_init);

/**@brief   Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack that are of interest to the Cycling Speed and Cadence
 *          Service Client.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 * @param[in]   p_context   Cycling Speed and Cadence Service Client structure.
 */
void ble_cscs_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief   Function for requesting the peer to start sending notification of Heart Rate
 *          Measurement.
 *
 * @details This function enables notification of the Cycling Speed and Cadence Measurement at the peer
 *          by writing to the CCCD of the Cycling Speed and Cadence Measurement characteristic.
 *
 * @param   p_ble_cscs_c Pointer to the Cycling Speed and Cadence Client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice is requested to write to the CCCD of the peer.
 * @retval	err_code	Otherwise, this function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t ble_cscs_c_csm_notif_enable(ble_cscs_c_t * p_ble_cscs_c);

/**@brief   Function for handling events from the Database Discovery module.
 *
 * @details Call this function when you get a callback event from the Database Discovery module.
 *          This function handles an event from the Database Discovery module, and determines
 *          whether it relates to the discovery of Running Speed and Cadence Service at the peer.
 *          If it does, the function calls the application's event handler to indicate that the CSC Service was
 * 			discovered at the peer. The function also populates the event with service-related
 *          information before providing it to the application.
 *
 * @param     p_ble_cscs_c Pointer to the Cycling Speed and Cadence Service Client structure.
 * @param[in] p_evt Pointer to the event received from the Database Discovery module.
 */
void ble_cscs_on_db_disc_evt(ble_cscs_c_t * p_ble_cscs_c, ble_db_discovery_evt_t const * p_evt);

/**@brief   Function for assigning handles to this instance of cscs_c.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module. The connection handle and attribute handles are
 *          provided from the discovery event @ref BLE_CSCS_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in]   p_ble_cscs_c    Pointer to the CSC client structure instance for associating the link.
 * @param[in]   conn_handle     Connection handle to associated with the given CSCS Client Instance.
 * @param[in]   p_peer_handles  Attribute handles on the CSCS server that you want this CSCS client
 *                              to interact with.
 */
uint32_t ble_cscs_c_handles_assign(ble_cscs_c_t    * p_ble_cscs_c,
                                   uint16_t          conn_handle,
                                   ble_cscs_c_db_t * p_peer_handles);

#ifdef __cplusplus
}
#endif

#endif // BLE_CSCS_C_H__