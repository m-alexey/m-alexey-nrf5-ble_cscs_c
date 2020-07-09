#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_CSCS_C)
#include "ble_cscs_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_gattc.h"

#define NRF_LOG_MODULE_NAME ble_cscs_c
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define CSCM_FLAG_WHEEL_PRESENT  (0x01 << 0)           /**< Bit mask used to extract the presence of Wheel Revolution Data. */
#define CSCM_FLAG_CRANK_PRESENT  (0x01 << 1)           /**< Bit mask used to extract the presence of Crank Revolution Data. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

static void gatt_error_handler(uint32_t   nrf_error,
                               void     * p_ctx,
                               uint16_t   conn_handle)
{
    ble_cscs_c_t * p_ble_cscs_c = (ble_cscs_c_t *)p_ctx;

    NRF_LOG_DEBUG("A GATT Client error has occurred on conn_handle: 0X%X", conn_handle);

    if (p_ble_cscs_c->error_handler != NULL)
    {
        p_ble_cscs_c->error_handler(nrf_error);
    }
}

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function uses the Handle Value Notification received from the SoftDevice
 *            and checks whether it is a notification of the Cycling Speed and Cadence measurement from
 *            the peer. If it is, this function decodes the Cycling Speed measurement and sends it
 *            to the application.
 *
 * @param[in] p_ble_cscs_c Pointer to the Cycling Speed and Cadence Client structure.
 * @param[in] p_ble_evt    Pointer to the BLE event received.
 */
static void on_hvx(ble_cscs_c_t * p_ble_cscs_c, const ble_evt_t * p_ble_evt)
{
    const ble_gattc_evt_hvx_t * p_notif = &p_ble_evt->evt.gattc_evt.params.hvx;

    // Check if the event is on the link for this instance
    if (p_ble_cscs_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }

    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_cscs_c->peer_db.cscs_handle)
    {
        uint32_t         index = 0;
        ble_cscs_c_evt_t ble_cscs_c_evt;

        ble_cscs_c_evt.params.csc.is_wheel_rev_data_present = p_notif->data[index] & CSCM_FLAG_WHEEL_PRESENT;
        ble_cscs_c_evt.params.csc.is_crank_rev_data_present = p_notif->data[index] & CSCM_FLAG_CRANK_PRESENT;
        index++;

        if(ble_cscs_c_evt.params.csc.is_wheel_rev_data_present)
        {
            ble_cscs_c_evt.params.csc.cumulative_wheel_revs = uint32_decode(&p_notif->data[index]);
            index += sizeof(uint32_t);
            ble_cscs_c_evt.params.csc.last_wheel_event_time = uint16_decode(&p_notif->data[index]);
            index += sizeof(uint16_t);
        }
        if(ble_cscs_c_evt.params.csc.is_crank_rev_data_present)
        {
            ble_cscs_c_evt.params.csc.cumulative_crank_revs = uint16_decode(&p_notif->data[index]);
            index += sizeof(uint16_t);
            ble_cscs_c_evt.params.csc.last_crank_event_time = uint16_decode(&p_notif->data[index]);
            index += sizeof(uint16_t);
        }

        ble_cscs_c_evt.evt_type    = BLE_CSCS_C_EVT_CSM_NOTIFICATION;
        ble_cscs_c_evt.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
        p_ble_cscs_c->evt_handler(p_ble_cscs_c, &ble_cscs_c_evt);
    }
}

/**@brief     Function for handling events from the Database Discovery module.
 *
 * @details   This function handles an event from the Database Discovery module, and determines
 *            whether it relates to the discovery of Cycling Speed and Cadence service at the peer. If it does, the function
 *            calls the application's event handler to indicate that the Cycling Speed and Cadence
 *            service was discovered at the peer. The function also populates the event with service-related
 * 			  information before providing it to the application.
 *
 * @param[in] p_evt Pointer to the event received from the Database Discovery module.
 *
 */
void ble_cscs_on_db_disc_evt(ble_cscs_c_t * p_ble_cscs_c, const ble_db_discovery_evt_t * p_evt)
{
        // Check if the Cycling Speed and Cadence Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_CYCLING_SPEED_AND_CADENCE &&
        p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_BLE)
    {
        ble_cscs_c_evt_t evt;

        for (uint32_t i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_CSC_MEASUREMENT_CHAR:
                {
                    evt.params.cscs_db.cscs_cccd_handle =
                        p_evt->params.discovered_db.charateristics[i].cccd_handle;
                    evt.params.cscs_db.cscs_handle      =
                        p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                    break;
                }
                case BLE_UUID_CSC_FEATURE_CHAR:
                {
                    evt.params.cscs_db.cscs_feature_handle =
                        p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                    break;
                }
                case BLE_UUID_SENSOR_LOCATION_CHAR:
                {
                    evt.params.cscs_db.cscs_sensloc_handle =
                        p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                    break;
                }
            }
        }
        evt.evt_type = BLE_CSCS_C_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;
        p_ble_cscs_c->evt_handler(p_ble_cscs_c, &evt);
    }
}

uint32_t ble_cscs_c_init(ble_cscs_c_t * p_ble_cscs_c, ble_cscs_c_init_t * p_ble_cscs_c_init)
{
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c);
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c_init);

    ble_uuid_t cscs_uuid;

    cscs_uuid.type = BLE_UUID_TYPE_BLE;
    cscs_uuid.uuid = BLE_UUID_CYCLING_SPEED_AND_CADENCE;

    p_ble_cscs_c->evt_handler              = p_ble_cscs_c_init->evt_handler;
    p_ble_cscs_c->error_handler            = p_ble_cscs_c_init->error_handler;
    p_ble_cscs_c->conn_handle              = BLE_CONN_HANDLE_INVALID;
    p_ble_cscs_c->peer_db.cscs_cccd_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_cscs_c->peer_db.cscs_handle      = BLE_GATT_HANDLE_INVALID;
    p_ble_cscs_c->p_gatt_queue             = p_ble_cscs_c_init->p_gatt_queue;

    return ble_db_discovery_evt_register(&cscs_uuid);
}

uint32_t ble_cscs_c_handles_assign(ble_cscs_c_t *    p_ble_cscs_c,
                                   uint16_t          conn_handle,
                                   ble_cscs_c_db_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c);
    p_ble_cscs_c->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_cscs_c->peer_db = *p_peer_handles;
    }

    return nrf_ble_gq_conn_handle_register(p_ble_cscs_c->p_gatt_queue, conn_handle);
}

/**@brief     Function for handling Disconnected event received from the SoftDevice.
 *
 * @details   This function check whether the disconnect event is happening on the link
 *            associated with the current instance of the module. If the event is happening, the function sets the instance's
 *            conn_handle to invalid.
 *
 * @param[in] p_ble_rscs_c Pointer to the RSC Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_disconnected(ble_cscs_c_t * p_ble_cscs_c, const ble_evt_t * p_ble_evt)
{
    if (p_ble_cscs_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_ble_cscs_c->conn_handle              = BLE_CONN_HANDLE_INVALID;
        p_ble_cscs_c->peer_db.cscs_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_ble_cscs_c->peer_db.cscs_handle      = BLE_GATT_HANDLE_INVALID;
    }
}

void ble_cscs_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_cscs_c_evt_t evt;
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_cscs_c_t * p_ble_cscs_c = (ble_cscs_c_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_cscs_c, p_ble_evt);
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_ble_cscs_c, p_ble_evt);
            break;
        default:
            break;
    }
}

/**@brief Function for creating a message for writing to the CCCD.
 */
static uint32_t cccd_configure(ble_cscs_c_t * p_ble_cscs_c, bool enable)
{
    NRF_LOG_DEBUG("Configuring CCCD. CCCD Handle = %d, Connection Handle = %d",
                  p_ble_cscs_c->peer_db.cscs_cccd_handle,
                  p_ble_cscs_c->conn_handle);

    uint8_t          cccd[WRITE_MESSAGE_LENGTH];
    uint16_t         cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
    nrf_ble_gq_req_t cscs_c_req;

    cccd[0] = LSB_16(cccd_val);
    cccd[1] = MSB_16(cccd_val);

    memset(&cscs_c_req, 0, sizeof(cscs_c_req));
    cscs_c_req.type                        = NRF_BLE_GQ_REQ_GATTC_WRITE;
    cscs_c_req.error_handler.cb            = gatt_error_handler;
    cscs_c_req.error_handler.p_ctx         = p_ble_cscs_c;
    cscs_c_req.params.gattc_write.handle   = p_ble_cscs_c->peer_db.cscs_cccd_handle;
    cscs_c_req.params.gattc_write.len      = WRITE_MESSAGE_LENGTH;
    cscs_c_req.params.gattc_write.p_value  = cccd;
    cscs_c_req.params.gattc_write.offset   = 0;
    cscs_c_req.params.gattc_write.write_op = BLE_GATT_OP_WRITE_REQ;
    cscs_c_req.params.gattc_write.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;

    return nrf_ble_gq_item_add(p_ble_cscs_c->p_gatt_queue, &cscs_c_req, p_ble_cscs_c->conn_handle);
}

uint32_t ble_cscs_c_csm_notif_enable(ble_cscs_c_t * p_ble_cscs_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c);

    if (p_ble_cscs_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return cccd_configure(p_ble_cscs_c, true);
}


/** @}
 *  @endcond
 */
#endif // NRF_MODULE_ENABLED(BLE_CSCS_C)