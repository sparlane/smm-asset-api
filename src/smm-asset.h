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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * An opaque object for accessing the smm
 */
typedef struct smm_connection_s *smm_connection;

/**
 * An opaque object that represents an asset on the smm
 */
typedef struct smm_asset_s *smm_asset;

/**
 * A list of opaque objects that represent assets on the smm
 */
typedef struct smm_asset_s **smm_assets;


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
 * Possible commands for an asset
 */
typedef enum {
    SMM_COMMAND_NONE, /*!< No restriction on current operation */
    SMM_COMMAND_CIRCLE, /*!< Circle/Hold at current position */
    SMM_COMMAND_RTL, /*!< Return to launch site */
    SMM_COMMAND_GOTO, /*!< Goto to the specified position */
    SMM_COMMAND_CONTINUE, /*!< Previous command revoked, resume own navigation */
    SMM_COMMAND_UNKNOWN, /*!< The command from the server is not known */
} smm_asset_command;

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

/**
 * Get all the assets that this user account has access to
 *
 * @param connection the smm_connection object to get the assets from
 * @param assets Where to store the assets
 * @param assets_count Where to store how many assets there are
 *
 * @return true if assets were successfully retrieved (even if there are none), false if there was an error
 */
bool smm_asset_get_assets(smm_connection connection, smm_assets *assets, size_t *assets_count);

/**
 * Free a set of assets
 *
 * @param assets the assets to free
 * @param assets_count how many assets to free
 */
void smm_asset_free_assets(smm_assets assets, size_t assets_count);

/**
 * Get the name of the specified asset
 *
 * @param asset the Asset
 *
 * @return The name of the asset, or NULL if asset is invalid
 */
const char *smm_asset_name(smm_asset asset);

/**
 * Get the type of the specified asset
 *
 * @param asset the Asset
 *
 * @return The type of the asset, or NULL if asset is invalid
 */
const char *smm_asset_type(smm_asset asset);

/**
 * Report the current position of the asset to the server
 *
 * @param asset the Asset
 * @param latitude The current latitude in degrees
 * @param longitude The current longitude in degrees
 * @param altitude The current altitude in meters
 * @param bearing the current course over ground in degrees true
 * @param fix the accurancy of the current fix (0=unknown, 2=2d only, 3 = 3d fix)
 *
 * @return true if the position was reported to the server
 */
bool smm_asset_report_position(smm_asset asset, double latitude, double longitude, unsigned int altitude, uint16_t bearing, uint8_t fix);

/**
 * Get the last command we saw from the server
 * the command is set in response to a position report,
 * normally this is checked after @ref smm_asset_report_position
 *
 * @param asset the Asset
 *
 * @return The command that currently applies to the asset
 */
smm_asset_command smm_asset_last_command(smm_asset asset);

/**
 * The position associated with a goto command
 *
 * @param asset the Asset
 * @param lat A place to store the latitude
 * @param lon A place to store the longitude
 *
 * @return true if the current command is goto and the fields were set
 */
bool smm_asset_last_goto_pos(smm_asset asset, double *lat, double *lon);

