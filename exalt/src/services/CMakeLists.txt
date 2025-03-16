if(CONFIG_EXALT_BLINK_SERVICE)
  target_sources(app PRIVATE blink_service.c)
endif()

if(CONFIG_EXALT_BLE_SERVICE)
  target_sources(app PRIVATE ble_service.c)
endif()