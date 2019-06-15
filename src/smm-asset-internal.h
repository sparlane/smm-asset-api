#pragma once

/**
 * smm-asset-internal.h, Internal structures and headers for libsmm-asset
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

#include <stdbool.h>

#include <curl/curl.h>

struct smm_connection_s {
	char *host;
	char *user;
	char *pass;
	smm_connection_status state;
	CURL *curl;
	char *csrfmiddlewaretoken;
};

struct smm_asset_s {
	smm_connection conn;
	char *name;
	char *type;
	int asset_id;
	int asset_type_id;
};

struct smm_curl_res_s {
	bool success;
	long httpcode;
	char *full_uri;
	char *redirect_url;
};

struct buffer_s {
    char *data;
    size_t bytes;
};

size_t to_buffer(char *ptr, size_t size, size_t nmemb, void *userdata);

void smm_curl_res_free (struct smm_curl_res_s *);
struct smm_curl_res_s *smm_connection_curl_retrieve_url (smm_connection conn, const char *path, const char *post_data, size_t (*write_func)(char *ptr, size_t size, size_t nmemb, void *userdata), void *write_data);
bool smm_connection_login (smm_connection connection);

smm_asset smm_asset_create(smm_connection connection, const char *name, const char *type, int asset_id, int asset_type_id);
void smm_asset_free_asset(smm_asset assets);
