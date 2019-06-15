/**
 * smm-asset-curl.c, Use curl to communicate with the SMM server.
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

#include <tidy.h>
#include <tidybuffio.h>

void
smm_curl_res_free (struct smm_curl_res_s *res)
{
	if (res)
	{
		free (res->full_uri);
		free (res->redirect_url);
	}
}

static size_t
eat_data (char *ptr __attribute__ ((unused)), size_t size, size_t nmemb, void *userdata __attribute__ ((unused)))
{
	return size * nmemb;
}

size_t
to_buffer (char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t new_bytes = size * nmemb;
	struct buffer_s *buf = (struct buffer_s *) userdata;
	char *tmp = realloc (buf->data, buf->bytes + new_bytes + 1);
	if (tmp == NULL)
	{
		return 0;
	}
	buf->data = tmp;
	memcpy (&buf->data[buf->bytes], ptr, new_bytes);
	buf->bytes += new_bytes;
	buf->data[buf->bytes] = '\0';

	return new_bytes;
}

struct smm_curl_res_s *
smm_connection_curl_retrieve_url (smm_connection conn, const char *path, const char *post_data,
				  size_t (*write_func) (char *ptr, size_t size, size_t nmemb, void *userdata), void *write_data)
{
	CURL *curl;
	struct smm_curl_res_s *res = NULL;

	if (conn == NULL || path == NULL)
	{
		return NULL;
	}

	curl = conn->curl;
	if (curl == NULL)
	{
		curl = curl_easy_init ();
		conn->curl = curl;
	}

	res = (struct smm_curl_res_s *) calloc (1, sizeof (struct smm_curl_res_s));

	if (asprintf (&res->full_uri, "%s%s", conn->host, path) < 0)
	{
		free (res);
		return NULL;
	}

	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt (curl, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 0L);
	curl_easy_setopt (curl, CURLOPT_URL, res->full_uri);

	if (post_data)
	{
		curl_easy_setopt (curl, CURLOPT_REFERER, res->full_uri);
		curl_easy_setopt (curl, CURLOPT_POSTFIELDS, post_data);
		curl_easy_setopt (curl, CURLOPT_POST, 1);
	}
	else
	{
		curl_easy_setopt (curl, CURLOPT_REFERER, NULL);
		curl_easy_setopt (curl, CURLOPT_POSTFIELDS, NULL);
		curl_easy_setopt (curl, CURLOPT_POST, 0);
	}

	if (write_func)
	{
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_func);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, write_data);
	}
	else
	{
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, eat_data);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, NULL);
	}

	CURLcode cres = curl_easy_perform (curl);
	res->success = (cres == CURLE_OK);

	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &res->httpcode);
	switch (res->httpcode)
	{
		case 301:
		case 302:
		case 303:
		{
			char *redirect_url = NULL;
			if (curl_easy_getinfo (curl, CURLINFO_REDIRECT_URL, &redirect_url) == CURLE_OK)
			{
				res->redirect_url = strdup (redirect_url);
			}
		}
			break;
	}

	/* Clear anything we set in the curl object */
	curl_easy_setopt (curl, CURLOPT_URL, NULL);
	curl_easy_setopt (curl, CURLOPT_REFERER, NULL);
	curl_easy_setopt (curl, CURLOPT_POSTFIELDS, NULL);
	curl_easy_setopt (curl, CURLOPT_POST, 0);
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, NULL);

	return res;
}

static size_t
populate_tidy (char *ptr, size_t size, size_t nmemb, void *userdata)
{
	tidyBufAppend ((TidyBuffer *) userdata, ptr, size * nmemb);
	return size * nmemb;
}

static bool
_extract_csrfmiddlewaretoken (TidyDoc tdoc, TidyNode tnod, char **token)
{
	bool res = false;
	for (TidyNode child = tidyGetChild (tnod); child; child = tidyGetNext (child))
	{
		ctmbstr name = tidyNodeGetName (child);
		if (name)
		{
			if (strcmp (name, "input") == 0)
			{
				/* check the attributes */
				for (TidyAttr attr = tidyAttrFirst (child); attr; attr = tidyAttrNext (attr))
				{
					ctmbstr attrName = tidyAttrName (attr);
					if (strcmp (attrName, "name") == 0)
					{
						if (strcmp (tidyAttrValue (attr), "csrfmiddlewaretoken") == 0)
						{
							res = true;
						}
					}
					else if (strcmp (attrName, "value") == 0)
					{
						if (res)
						{
							*token = strdup (tidyAttrValue (attr));
							return true;
						}
					}
				}
			}
		}
		res = _extract_csrfmiddlewaretoken (tdoc, child, token);
		if (res)
		{
			return res;
		}
	}
	return res;
}



bool
smm_connection_login (smm_connection connection)
{
	bool res = false;
	TidyDoc tdoc;
	TidyBuffer docbuf = { 0 };

	tdoc = tidyCreate ();
	tidyOptSetBool (tdoc, TidyForceOutput, yes);
	tidyOptSetInt (tdoc, TidyWrapLen, 4096);
	tidyBufInit (&docbuf);

	/* Get the login page, so we can get the csrf cookie + token */
	struct smm_curl_res_s *res_get = smm_connection_curl_retrieve_url (connection, "/accounts/login/", NULL, populate_tidy, &docbuf);

	if (res_get && res_get->success && res_get->httpcode == 200)
	{
		tidyParseBuffer (tdoc, &docbuf);
		tidyCleanAndRepair (tdoc);

		/* find the input token with the csrfmiddlewaretoken */
		_extract_csrfmiddlewaretoken (tdoc, tidyGetRoot (tdoc), &connection->csrfmiddlewaretoken);

		if (connection->csrfmiddlewaretoken)
		{
			char *post_data = NULL;
			if (asprintf
			    (&post_data, "csrfmiddlewaretoken=%s&username=%s&password=%s", connection->csrfmiddlewaretoken, connection->user,
			     connection->pass) >= 0)
			{
				struct smm_curl_res_s *res_post = smm_connection_curl_retrieve_url (connection, "/accounts/login/", post_data, NULL, NULL);
				if (res_post && res_post->success && res_post->httpcode == 302)
				{
					res = true;
					connection->state = SMM_CONNECTION_CONNECTED;
				}
				else
				{
					connection->state = SMM_CONNECTION_AUTHENTICATION_FAILURE;
				}
				smm_curl_res_free (res_post);
				free (post_data);
			}
		}
	}
	else if (!res_get)
	{
		connection->state = SMM_CONNECTION_NO_HOST_CONNECTION;
	}
	smm_curl_res_free (res_get);

	tidyBufFree (&docbuf);
	tidyRelease (tdoc);

	return res;
}
