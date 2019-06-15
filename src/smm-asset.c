/**
 * smm-asset.c, API functions for communicating with Search Management Map
 * to act as an Asset.
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
#include "smm-asset.h"
#include "smm-asset-internal.h"

#include <stdlib.h>
#include <string.h>

smm_connection
smm_asset_connect (const char *host, const char *user, const char *pass)
{
	smm_connection conn = calloc (1, sizeof (struct smm_connection_s));
	if (conn == NULL)
	{
		return NULL;
	}

	conn->host = strdup (host);
	conn->user = strdup (user);
	conn->pass = strdup (pass);

	return conn;
}

smm_connection_status
smm_asset_connection_get_state (smm_connection connection)
{
	if (connection == NULL)
	{
		return SMM_CONNECTION_UNKNOWN;
	}
	return connection->state;
}

void
smm_connection_close (smm_connection connection)
{
	if (connection != NULL)
	{
		free (connection->host);
		free (connection->user);
		free (connection->pass);
	}
	free (connection);
}
