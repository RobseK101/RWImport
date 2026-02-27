#pragma once
#include <cstring>
#include <string>
#include <fstream>
#include <ren/Exceptions.hpp>

/// @file
/// @brief Logging utilities

namespace ren
{
	// Interface
	class LogModule
	{
	public:
		virtual ~LogModule();
		virtual void print(const std::string& _output) = 0;
		virtual void flush() = 0;
	};

	// "static" class
	class Logging
	{
	public:
		static constexpr size_t PRINTBUFFER_SIZE = 512;

		static void init(LogModule* _module);
		static void log(const char* _format, ...);
		static void log(const std::string& _string);
		static void forceFlush(bool _state);
		static void quit();

	private:
		static LogModule* ps_logModule;
		static bool p_forceFlush;
	};

	class LogfileModule : public LogModule
	{
	public:
		static constexpr size_t BUFFER_SIZE = 1024 * 4;

		LogfileModule() = delete;
		LogfileModule(const char* _filename);
		LogfileModule(const LogfileModule&) = delete;
		LogfileModule(LogfileModule&&) = delete;

		virtual ~LogfileModule() override;
		virtual void print(const std::string& _output) override;
		virtual void flush() override;

	private:
		std::string p_buffer;
		std::ofstream p_logfile;
	};
}