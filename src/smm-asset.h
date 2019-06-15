#pragma once

/**
 * smm-asset.h, The Asset interface to Search Management Map
 *
 * Copyright 2019 Canterbury Air Patrol Incorporated
 *
 * This file is part of libsmm-asset
 *
 * libsmm-asset is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * An opaque object for accessing the smm
 */
typedef struct smm_connection_s *smm_connection;


/**
 * Possible current states for an smm_connection object
 */
typedef enum {
	SMM_CONNECTION_UNKNOWN, /*!< Unknown state or invalid object */
	SMM_CONNECTION_CONNECTED, /*!< Currently connected */
	SMM_CONNECTION_HOST_INVALID, /*!< Host URL invalid, i.e. not http(s):// or not a valid domain */
	SMM_CONNECTION_NO_HOST_CONNECTION, /*!< Unable to connect to host */
	SMM_CONNECTION_AUTHENTICATION_FAILURE, /*!< Unable to authenticate with host */
	SMM_CONNECTION_FAILURE, /*!< Unable to communicate, for another reason */
} smm_connection_status;

/**
 * Connect to the specified smm
 *
 * @param host the URI of the smm server (i.e. https://smm.example.com)
 * @param user the username to authenticate as
 * @param pass the password to authenticate with
 *
 * @return an smm_connection object, check the status with @ref smm_asset_connection_status
 */
smm_connection smm_asset_connect(const char *host, const char *user, const char *pass);

/**
 * Check the state of a connection
 *
 * @param connection the smm_connection object to check
 *
 * @return The current state of the connection
 */
smm_connection_status smm_asset_connection_get_state(smm_connection connection);


/**
 * Close a connection to smm and free associated resources
 *
 * @param connection the smm_connection object to close and free
 */
void smm_connection_close (smm_connection connection);
