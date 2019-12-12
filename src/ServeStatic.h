#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

class ServeStatic : public AsyncWebHandler
{
	using File = fs::File;
	using FS = fs::FS;

private:
	bool _getFile(AsyncWebServerRequest *request);
	bool _fileExists(AsyncWebServerRequest *request, const String &path);
	uint8_t _countBits(const uint8_t value) const;

protected:
	FS _fs;
	String _uri;
	String _path;
	String _default_file;
	String _cache_control;
	String _last_modified;
	AwsTemplateProcessor _callback;
	bool _isDir;
	bool _gzipFirst;
	uint8_t _gzipStats;

public:
	ServeStatic(const char *uri, FS &fs, const char *path);
	virtual bool canHandle(AsyncWebServerRequest *request) override final;
	virtual void handleRequest(AsyncWebServerRequest *request) override final;
	ServeStatic &setIsDir(bool isDir);
	ServeStatic &setDefaultFile(const char *filename);
	ServeStatic &setCacheControl(const char *cache_control);
	ServeStatic &setLastModified(const char *last_modified);
	ServeStatic &setTemplateProcessor(AwsTemplateProcessor newCallback)
	{
		_callback = newCallback;
		return *this;
	}
};