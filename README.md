# nrf5-ble_cscs_c
Cycling Speed and Cadence Service Client SDK 17.0.0.0

Add to your sdk_config.h, placement choose yourself.
```c
// <q> BLE_CSCS_C_ENABLED  - ble_cscs - Cycling Speed and Cadence Service

#ifndef BLE_CSCS_C_ENABLED
#define BLE_CSCS_C_ENABLED 1
#endif

// <o> BLE_CSCS_C_BLE_OBSERVER_PRIO  
// <i> Priority with which BLE events are dispatched to the Cycling Speed and Cadence Service.

#ifndef BLE_CSCS_C_BLE_OBSERVER_PRIO
#define BLE_CSCS_C_BLE_OBSERVER_PRIO 2
#endif
```
for an example look at ble_central\ble_app_rscs_c
