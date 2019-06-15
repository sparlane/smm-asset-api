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

#include <jansson.h>

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

	smm_connection_login (conn);

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
		free (connection->csrfmiddlewaretoken);
		curl_easy_cleanup (connection->curl);
	}
	free (connection);
}

smm_asset
smm_asset_create (smm_connection conn, const char *name, const char *type, int asset_id, int asset_type_id)
{
	smm_asset asset = calloc (1, sizeof (struct smm_asset_s));
	asset->conn = conn;
	asset->name = strdup (name);
	asset->type = strdup (type);
	asset->asset_id = asset_id;
	asset->asset_type_id = asset_type_id;

	return asset;
}

bool
smm_asset_get_assets (smm_connection connection, smm_assets * assets, size_t * assets_count)
{
	struct buffer_s buf = { NULL, 0 };
	json_t *json_root;
	json_error_t json_error;

	struct smm_curl_res_s *res = smm_connection_curl_retrieve_url (connection, "/assets/mine/json/", NULL, to_buffer, &buf);
	if (res == NULL)
	{
		return false;
	}
	if (!(res->success && res->httpcode == 200))
	{
		/* Login and then try again */
		if (smm_connection_login (connection))
		{
			smm_curl_res_free (res);
			res = smm_connection_curl_retrieve_url (connection, "/assets/mine/json/", NULL, to_buffer, &buf);
			if (!res || !(res->success && res->httpcode == 200))
			{
				/* Still no good */
				smm_curl_res_free (res);
				return false;
			}
		}
	}
	smm_curl_res_free (res);

	/* Parse the assets */
	*assets_count = 0;
	*assets = NULL;

	json_root = json_loadb (buf.data, buf.bytes, 0, &json_error);
	if (json_root)
	{
		json_t *json_assets = json_object_get (json_root, "assets");
		if (json_assets)
		{
			size_t index;
			json_t *value;

			json_array_foreach (json_assets, index, value)
			{
				const char *key;
				json_t *val;
				int asset_id = -1;
				int asset_type_id = -1;
				const char *name = NULL;
				const char *type = NULL;
				json_object_foreach (value, key, val)
				{
					if (strcmp (key, "id") == 0)
					{
						asset_id = json_integer_value (val);
					}
					else if (strcmp (key, "type_id") == 0)
					{
						asset_type_id = json_integer_value (val);
					}
					else if (strcmp (key, "name") == 0)
					{
						name = json_string_value (val);
					}
					else if (strcmp (key, "type_name") == 0)
					{
						type = json_string_value (val);
					}
					else
					{
						if (json_is_integer (val))
						{
							printf ("%s = %lli\n", key, json_integer_value (val));
						}
						else if (json_is_string (val))
						{
							printf ("%s = %s\n", key, json_string_value (val));
						}
					}
				}
				*(assets_count) += 1;
				*assets = realloc (*assets, *assets_count * sizeof (smm_asset));
				(*assets)[(*assets_count) - 1] = smm_asset_create (connection, name, type, asset_id, asset_type_id);
			}
		}
		else
		{
			printf ("Didn't find assets\n");
		}
	}
	else
	{
		printf ("Error on line %i: %s\n", json_error.line, json_error.text);
	}

	json_decref (json_root);


	free (buf.data);
	return true;
}


void
smm_asset_free_asset (smm_asset asset)
{
	free (asset->name);
	free (asset->type);
	free (asset);
}

void
smm_asset_free_assets (smm_assets assets, size_t assets_count)
{
	for (size_t i = 0; i < assets_count; i++)
	{
		smm_asset_free_asset (assets[i]);
	}
	free (assets);
}

static int
smm_asset_get_asset_id (smm_asset asset)
{
	return asset->asset_id;
}

const char *
smm_asset_name (smm_asset asset)
{
	if (asset)
	{
		return asset->name;
	}
	return NULL;
}

const char *
smm_asset_type (smm_asset asset)
{
	if (asset)
	{
		return asset->type;
	}
	return NULL;
}

static bool
smm_asset_update_command (smm_asset asset, struct buffer_s *buf)
{
	json_t *json_root;
	json_error_t json_error;

	json_root = json_loadb (buf->data, buf->bytes, 0, &json_error);
	if (json_root)
	{
		const char *command = NULL;
		json_t *tmp;

		tmp = json_object_get (json_root, "action");
		if (tmp)
		{
			command = json_string_value (tmp);
		}

		if (strcmp (command, "GOTO") == 0)
		{
			/* Get lat and long as well */
			tmp = json_object_get (json_root, "latitude");
			if (tmp)
			{
				asset->last_command_lat = json_real_value (tmp);
			}
			tmp = json_object_get (json_root, "longitude");
			if (tmp)
			{
				asset->last_command_lon = json_real_value (tmp);
			}
			asset->last_command = SMM_COMMAND_GOTO;
		}
		else if (strcmp (command, "RON") == 0)
		{
			asset->last_command = SMM_COMMAND_CONTINUE;
		}
		else if (strcmp (command, "RTL") == 0)
		{
			asset->last_command = SMM_COMMAND_RTL;
		}
		else if (strcmp (command, "CIR") == 0)
		{
			asset->last_command = SMM_COMMAND_CIRCLE;
		}
		else
		{
			asset->last_command = SMM_COMMAND_UNKNOWN;
		}


		json_decref (json_root);
	}
	else
	{
		printf ("Error on line %i: %s\n", json_error.line, json_error.text);
		asset->last_command = SMM_COMMAND_UNKNOWN;
	}

	return true;
}


smm_asset_command
smm_asset_last_command (smm_asset asset)
{
	return asset->last_command;
}

bool
smm_asset_last_goto_pos (smm_asset asset, double *lat, double *lon)
{
	if (asset->last_command != SMM_COMMAND_GOTO)
	{
		return false;
	}
	if (lat == NULL || lon == NULL)
	{
		return false;
	}
	*lat = asset->last_command_lat;
	*lon = asset->last_command_lon;
	return true;
}


bool
smm_asset_report_position (smm_asset asset, double latitude, double longitude, unsigned int altitude, uint16_t bearing, uint8_t fix)
{
	struct buffer_s buf = { NULL, 0 };

	char *page = NULL;
	if (asprintf (&page, "/data/assets/%s/position/add/?lat=%lf&lon=%lf&alt=%u&bearing=%u&fix=%u", asset->name, latitude, longitude, altitude, bearing, fix)
	    < 0)
	{
		return false;
	}

	struct smm_curl_res_s *res = smm_connection_curl_retrieve_url (asset->conn, page, NULL, to_buffer, &buf);
	if (res == NULL)
	{
		free (page);
		return false;
	}
	if (!(res->success && res->httpcode == 200))
	{
		/* login and try again */
		smm_curl_res_free (res);
		smm_connection_login (asset->conn);
		res = smm_connection_curl_retrieve_url (asset->conn, page, NULL, to_buffer, &buf);
		if (!res || !(res->success && res->httpcode == 200))
		{
			smm_curl_res_free (res);
			free (page);
			return false;
		}
	}

	free (page);

	/* if json data was returned, update the current action */
	if (res->content_type != NULL && strcmp (res->content_type, "application/json") == 0)
	{
		smm_asset_update_command (asset, &buf);
	}
	else
	{
		if (buf.data && strncmp (buf.data, "Continue", buf.bytes) == 0)
		{
			asset->last_command = SMM_COMMAND_CONTINUE;
		}
		else
		{
			asset->last_command = SMM_COMMAND_NONE;
		}
	}

	free (buf.data);

	return false;
}
