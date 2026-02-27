#include "ren/Logging.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <ren/Exceptions.hpp>

namespace ren
{
	LogModule* Logging::ps_logModule = nullptr;
	bool Logging::p_forceFlush = false;

	void Logging::init(LogModule* _module)
	{
		if (ps_logModule) {
			delete ps_logModule;
		}
		ps_logModule = _module;

		time_t timestamp;
		time(&timestamp);

		Logging::log("[LOGGING INIT] %s", ctime(&timestamp));
		exceptionsLogFn = &Logging::log;
	}

	void Logging::log(const char* _format, ...)
	{
		va_list args;
		va_start(args, _format);
		char buffer[PRINTBUFFER_SIZE];

		if (ps_logModule) {
			std::vsnprintf(buffer, sizeof(buffer) - 1, _format, args);
			std::string output = buffer;
			ps_logModule->print(output);
			if (p_forceFlush) {
				ps_logModule->flush();
			}
		}
		
		va_end(args);
	}

	void Logging::log(const std::string& _string)
	{
		if (ps_logModule) {
			ps_logModule->print(_string);
		}
	}

	void Logging::forceFlush(bool _state)
	{
		p_forceFlush = _state;
	}

	void Logging::quit()
	{
		exceptionsLogFn = nullptr;

		time_t timestamp;
		time(&timestamp);
		Logging::log("[LOGGING QUIT] %s\n", ctime(&timestamp));

		if (ps_logModule) {
			ps_logModule->flush();
			delete ps_logModule;
			ps_logModule = nullptr;
		}
	}

	LogfileModule::LogfileModule(const char* _filename) : 
		p_logfile(_filename, std::ios::app), p_buffer() 
	{
		if (!p_logfile.is_open()) {
			throwException<std::runtime_error>(
				"%s(): Could not initialize the logfile \"%s\"!", __FUNCTION__,
				_filename);
		}
	}

	LogfileModule::~LogfileModule()
	{
		flush();
		p_logfile.close();
	}

	void LogfileModule::print(const std::string& _output)
	{
		p_buffer += _output;
		if (p_buffer.size() >= BUFFER_SIZE) {
			flush();
		}
	}

	void LogfileModule::flush()
	{
		p_logfile << p_buffer;
		p_logfile.flush();
		p_buffer.clear();
	}

	LogModule::~LogModule()
	{
	}

}