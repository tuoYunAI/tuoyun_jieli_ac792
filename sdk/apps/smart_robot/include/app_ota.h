
#ifndef __OTA_H__
#define __OTA_H__

/**
 * @brief  Register device information to the server at startup
 * @param  mac: Device MAC address
 * @return 0: Registration successful
 *     Other: Registration failed
 * @note 
 */
int register_device(char *mac);

#endif