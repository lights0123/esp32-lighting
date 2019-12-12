#include "ServeStatic.h"
ServeStatic::ServeStatic(const char *uri, FS &fs, const char *path)
	: _fs(fs), _uri(uri), _path(path), _default_file("index.htm"), _cache_control(""), _last_modified(""), _callback(nullptr)
{
	// Ensure leading '/'
	if (_uri.length() == 0 || _uri[0] != '/')
		_uri = "/" + _uri;
	if (_path.length() == 0 || _path[0] != '/')
		_path = "/" + _path;

	// If path ends with '/' we assume a hint that this is a directory to improve performance.
	// However - if it does not end with '/' we, can't assume a file, path can still be a directory.
	_isDir = _path[_path.length() - 1] == '/';

	// Remove the trailing '/' so we can handle default file
	// Notice that root will be "" not "/"
	if (_uri[_uri.length() - 1] == '/')
		_uri = _uri.substring(0, _uri.length() - 1);
	if (_path[_path.length() - 1] == '/')
		_path = _path.substring(0, _path.length() - 1);

	// Reset stats
	_gzipFirst = false;
	_gzipStats = 0xF8;
}

ServeStatic &ServeStatic::setIsDir(bool isDir)
{
	_isDir = isDir;
	return *this;
}

ServeStatic &ServeStatic::setDefaultFile(const char *filename)
{
	_default_file = String(filename);
	return *this;
}

ServeStatic &ServeStatic::setCacheControl(const char *cache_control)
{
	_cache_control = String(cache_control);
	return *this;
}

ServeStatic &ServeStatic::setLastModified(const char *last_modified)
{
	_last_modified = String(last_modified);
	return *this;
}

bool ServeStatic::canHandle(AsyncWebServerRequest *request)
{
	if (request->method() != HTTP_GET || !request->url().startsWith(_uri) || !request->isExpectedRequestedConnType(RCT_DEFAULT, RCT_HTTP))
	{
		return false;
	}
	if (_getFile(request))
	{
		// We interested in "If-Modified-Since" header to check if file was modified
		if (_last_modified.length())
			request->addInterestingHeader("If-Modified-Since");

		if (_cache_control.length())
			request->addInterestingHeader("If-None-Match");

		DEBUGF("[ServeStatic::canHandle] TRUE\n");
		return true;
	}

	return false;
}

bool ServeStatic::_getFile(AsyncWebServerRequest *request)
{
	// Remove the found uri
	String path = request->url().substring(_uri.length());

	// We can skip the file check and look for default if request is to the root of a directory or that request path ends with '/'
	bool canSkipFileCheck = (_isDir && path.length() == 0) || (path.length() && path[path.length() - 1] == '/');

	path = _path + path;

	// Do we have a file or .gz file
	if (!canSkipFileCheck && _fileExists(request, path))
		return true;

	// Can't handle if not default file
	if (_default_file.length() == 0)
		return false;

	// Try to add default file, ensure there is a trailing '/' ot the path.
	if (path.length() == 0 || path[path.length() - 1] != '/')
		path += "/";
	path += _default_file;

	return _fileExists(request, path);
}

#ifdef ESP32
#define FILE_IS_REAL(f) (f == true && !f.isDirectory())
#else
#define FILE_IS_REAL(f) (f == true)
#endif

bool ServeStatic::_fileExists(AsyncWebServerRequest *request, const String &path)
{
	bool fileFound = false;
	bool gzipFound = false;
	bool acceptsGzip = false;

	String gzip = path + ".gz";
	const String *selected = &path;
	if (request->hasHeader("Accept-Encoding"))
	{
		AsyncWebHeader *h = request->getHeader("Accept-Encoding");
		acceptsGzip = h->value().indexOf("gzip") != -1;
	}
	if (acceptsGzip)
	{
		request->_tempFile = _fs.open(gzip, "r");
		gzipFound = FILE_IS_REAL(request->_tempFile);
	}
	if (!gzipFound)
	{
		request->_tempFile = _fs.open(path, "r");
		fileFound = FILE_IS_REAL(request->_tempFile);
	}
	else
	{
		selected = &gzip;
	}

	bool found = fileFound || gzipFound;

	if (found)
	{
		// Extract the file name from the path and keep it in _tempObject
		size_t pathLen = selected->length();
		char *_tempPath = (char *)malloc(pathLen + 1);
		snprintf(_tempPath, pathLen + 1, "%s", selected->c_str());
		request->_tempObject = (void *)_tempPath;
	}

	return found;
}

uint8_t ServeStatic::_countBits(const uint8_t value) const
{
	uint8_t w = value;
	uint8_t n;
	for (n = 0; w != 0; n++)
		w &= w - 1;
	return n;
}
const char *getFileType(String path)
{
	if (path.endsWith(".gz") && path.indexOf('.') < path.length() - 3)
	{
		path = path.substring(0, path.length() - 3);
	}
	if (path.endsWith(".html"))
		return "text/html";
	if (path.endsWith(".htm"))
		return "text/html";
	if (path.endsWith(".css"))
		return "text/css";
	if (path.endsWith(".json"))
		return "application/json";
	if (path.endsWith(".js"))
		return "application/javascript";
	if (path.endsWith(".png"))
		return "image/png";
	if (path.endsWith(".gif"))
		return "image/gif";
	if (path.endsWith(".jpg"))
		return "image/jpeg";
	if (path.endsWith(".ico"))
		return "image/x-icon";
	if (path.endsWith(".svg"))
		return "image/svg+xml";
	if (path.endsWith(".eot"))
		return "font/eot";
	if (path.endsWith(".woff"))
		return "font/woff";
	if (path.endsWith(".woff2"))
		return "font/woff2";
	if (path.endsWith(".ttf"))
		return "font/ttf";
	if (path.endsWith(".xml"))
		return "text/xml";
	if (path.endsWith(".pdf"))
		return "application/pdf";
	if (path.endsWith(".zip"))
		return "application/zip";
	if (path.endsWith(".gz"))
		return "application/x-gzip";
	return "text/plain";
}
void ServeStatic::handleRequest(AsyncWebServerRequest *request)
{
	// Get the filename from request->_tempObject and free it
	String filename = String((char *)request->_tempObject);
	free(request->_tempObject);
	request->_tempObject = NULL;
	if ((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
		return request->requestAuthentication();

	if (request->_tempFile == true)
	{
		String etag = String(request->_tempFile.size());
		if (_last_modified.length() && _last_modified == request->header("If-Modified-Since"))
		{
			request->_tempFile.close();
			request->send(304); // Not modified
		}
		else if (_cache_control.length() && request->hasHeader("If-None-Match") && request->header("If-None-Match").equals(etag))
		{
			request->_tempFile.close();
			AsyncWebServerResponse *response = new AsyncBasicResponse(304); // Not modified
			response->addHeader("Cache-Control", _cache_control);
			response->addHeader("ETag", etag);
			request->send(response);
		}
		else
		{
			AsyncWebServerResponse *response = new AsyncFileResponse(request->_tempFile, "", getFileType(filename), false, _callback);
			if (_last_modified.length())
				response->addHeader("Last-Modified", _last_modified);
			if (_cache_control.length())
			{
				response->addHeader("Cache-Control", _cache_control);
				response->addHeader("ETag", etag);
			}
			request->send(response);
		}
	}
	else
	{
		request->send(404);
	}
}
