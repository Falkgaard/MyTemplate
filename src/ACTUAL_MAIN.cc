// Signature:
	//    Author:  Falkgaard
	// Copyright:  2020-2021

// Pre-Processor Defines:
	// General:
		#define RW 
		#define RO const
	// Vulkan-Hpp:
		#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
		#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
	// GLFW:
		#define GLFW_INCLUDE_VULKAN
	// Logger module:
		#ifdef LOG
			#define MakeLogHandle( FALK_LOG_handleName, FALK_LOG_context ) \
				auto FALK_LOG_handleName = LogHandle { FALK_LOG_context }
			#define Log( FALK_LOG_handleName, ... ) \
				FALK_LOG_handleName (__VA_ARGS__)
		#else
			#define MakeLogHandle( FALK_LOG_handleName, FALK_LOG_context ) do{} while(0)
			#define Log( FALK_LOG_handleName, ... )                        do{} while(0) 
		#endif

// Header Includes:
	// C
		#include <cstdlib>
		#include <cassert>
		#include <cstdint>
		#include <cstdio>
		#include <ctime>
	// C++
		#include <stdexcept>
		#include <functional>
		#include <vector>
		#include <set>
		#include <optional>
		#include <algorithm>
		#include <limits>
		//#include <string_view>
	// GLFW & Vulkan
		#include <GLFW/glfw3.h>
		#include <vulkan/vulkan.hpp>
	// {fmt}
		#include <fmt/core.h>
		#include <fmt/format.h>

// Global Constexpr Constants:
	auto constexpr g_isDebugging
	{
		#ifdef DEBUG
			true
		#else
			false
		#endif
	} ;
	
	auto constexpr g_isVerbose
	{
		#ifdef VERBOSE
			g_isDebugging
		#else
			false
		#endif
	} ;
	
	char const constexpr g_version[]
	{
		"Falkengine v0.1.0"
	} ;
	
	auto constexpr g_maxFramesInFlight
	{
		3
	} ;
	
// Using Aliases:
	using U8  = uint8_t;
	using U16 = uint16_t;
	using U32 = uint32_t;
	using U64 = uint64_t;
	
	using I8  = int8_t;
	using I16 = int16_t;
	using I32 = int32_t;
	using I64 = int64_t;
	
	using F32 = float;
	using F64 = double;
	
	using Byte = char;
	
	template <typename...Args>
	using Vec = std::vector<Args...>;
	
	template <typename...Args>
	using UPtr = std::unique_ptr<Args...>;
	
	using CStr = char const *;

// ANSI Text Formatting Module:
	struct FG // foreground color
	{
		U8 code = 255;
	} ;
	
	[[nodiscard]] auto to_string( FG RO fg) -> std::string
	{
		return std::string("38;5;") + std::to_string(fg.code);
	} //
	
	struct BG // background color
	{
		U8 code = 238;
	} ;
		
	[[nodiscard]] auto to_string( BG RO bg ) noexcept -> std::string
	{
		return std::string("48;5;") + std::to_string(bg.code);
	} //
	
	struct ModMask
	{
		U8 flags;
	} ;
	
	namespace Mod // modifier constants
	{
		ModMask constexpr none       { 0      };
		ModMask constexpr bold       { 1      };
		ModMask constexpr faint      { 1 << 1 };
		ModMask constexpr italic     { 1 << 2 };
		ModMask constexpr underline  { 1 << 3 };
		ModMask constexpr fast_blink { 1 << 4 };
		ModMask constexpr slow_blink { 1 << 5 };
		ModMask constexpr clear      { 1 << 7 }; // 1 << 6 is unused
	} //
	
	[[nodiscard]] auto to_string( ModMask RO mod ) noexcept -> std::string
	{
		std::string result {};
		if ( mod.flags & Mod::bold       .flags ) result += "1;";
		if ( mod.flags & Mod::faint      .flags ) result += "2;";
		if ( mod.flags & Mod::italic     .flags ) result += "3;";
		if ( mod.flags & Mod::underline  .flags ) result += "4;";
		if ( mod.flags & Mod::slow_blink .flags ) result += "5;";
		if ( mod.flags & Mod::fast_blink .flags ) result += "6;";
		return result;
	} //
	
	[[nodiscard]] auto constexpr operator& ( ModMask RO lhs, ModMask RO rhs ) noexcept -> ModMask
	{
		return (lhs.flags | rhs.flags) & Mod::clear.flags ?
			ModMask{ Mod::clear.flags } : ModMask{ static_cast<U8>(lhs.flags | rhs.flags) };
	} //
	
	// Style Decorator Functions:
		[[nodiscard]] auto ANSI( std::string RO &text, FG RO fg, BG RO bg, ModMask RO mm ) noexcept -> std::string
		{
			return std::string("\033[") + to_string(mm) + to_string(fg) + ';' + to_string(bg) + 'm' + text + "\033[0m";
		} //
		
		//[[nodiscard]] inline auto ANSI( std::string RO &text, FG RO fg ) noexcept -> std::string
		//{
		//	return ANSI( text, fg, BG{}, ModMask{} ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, FG RO fg, BG RO bg ) noexcept -> std::string
		//{
		//	return ANSI( text, fg, bg, ModMask{} ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, FG RO fg, ModMask RO mm ) noexcept -> std::string
		//{
		//	return ANSI( text, fg, BG{}, mm ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, BG RO bg ) noexcept -> std::string
		//{
		//	return ANSI( text, FG{}, bg, ModMask{} ); 
		//} //
		//[[nodiscard]] inline auto ANSI( std::string RO &text, BG RO bg, FG RO fg ) noexcept -> std::string
		//{
		//	return ANSI( text, fg, bg, ModMask{} ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, BG RO bg, ModMask RO mm ) noexcept -> std::string
		//{
		//	return ANSI( text, FG{}, bg, mm ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, ModMask RO mm ) noexcept -> std::string
		//{
		//	return ANSI( text, FG{}, BG{}, mm ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, ModMask RO mm, FG RO fg ) noexcept -> std::string
		//{
		//	return ANSI( text, fg, BG{}, mm ); 
		//} //
		//
		//[[nodiscard]] inline auto ANSI( std::string RO &text, ModMask RO mm, BG RO bg ) noexcept -> std::string
		//{
		//	return ANSI( text, FG{}, bg, mm ); 
		//} //
	
	struct Style
	{
		// data members:
			FG      fg {};
			BG      bg {};
			ModMask mm {};
		
		[[nodiscard]] auto apply( std::string RO &text ) const noexcept -> std::string
		{
			return ANSI( text, this->fg, this->bg, this->mm );
		} //
		
		[[nodiscard]] auto apply( CStr RO text ) const noexcept -> std::string
		{
			return ANSI( std::string(text), this->fg, this->bg, this->mm );
		} //
		
		[[nodiscard]] auto constexpr operator&( ModMask RO otherMod ) const noexcept -> Style
		{
			return { this->fg, this->bg, this->mm & otherMod };
		} //
		
		// end-of-struct
	} ;
	
	[[nodiscard]] auto ANSI( std::string RO &text, Style RO &style ) noexcept -> std::string
	{
		return ANSI( text, style.fg, style.bg, style.mm );
	} //
	
	struct Theme
	{
		Style  critical,
		       error,
		       warning,
		       important,
		       plain,
		       unimportant,
		       header1,
		       header2,
		       header3,
		       debug,
		       selected,
		       list_positive,
		       list_neutral,
		       list_negative,
		       timestamp,
		       tag,
		       source;
	} ;
	
	namespace CC // color codes
	{
		[[maybe_unused]] U8 constexpr blue_light   {  81 };
		[[maybe_unused]] U8 constexpr blue         {  33 };
		[[maybe_unused]] U8 constexpr blue_dark    {  26 };
		[[maybe_unused]] U8 constexpr red_light    { 196 };
		[[maybe_unused]] U8 constexpr red          { 124 };
		[[maybe_unused]] U8 constexpr red_dark     {  88 };
		[[maybe_unused]] U8 constexpr green_light  {  83 };
		[[maybe_unused]] U8 constexpr green        {  35 };
		[[maybe_unused]] U8 constexpr green_dark   {  29 };
		[[maybe_unused]] U8 constexpr yellow_light { 229 };
		[[maybe_unused]] U8 constexpr yellow       { 227 };
		[[maybe_unused]] U8 constexpr yellow_dark  { 178 };
		[[maybe_unused]] U8 constexpr purple_light { 177 };
		[[maybe_unused]] U8 constexpr purple       { 127 };
		[[maybe_unused]] U8 constexpr purple_dark  {  53 };
		[[maybe_unused]] U8 constexpr orange_light { 222 };
		[[maybe_unused]] U8 constexpr orange       { 215 };
		[[maybe_unused]] U8 constexpr orange_dark  { 202 };
		[[maybe_unused]] U8 constexpr pink_light   { 224 };
		[[maybe_unused]] U8 constexpr pink         { 212 };
		[[maybe_unused]] U8 constexpr pink_dark    { 200 };
		[[maybe_unused]] U8 constexpr cyan_light   { 159 };
		[[maybe_unused]] U8 constexpr cyan         {  87 };
		[[maybe_unused]] U8 constexpr cyan_dark    {  44 };
		[[maybe_unused]] U8 constexpr grey_light   { 251 };
		[[maybe_unused]] U8 constexpr grey         { 246 };
		[[maybe_unused]] U8 constexpr grey_dark    { 243 };
		[[maybe_unused]] U8 constexpr brown_light  { 137 };
		[[maybe_unused]] U8 constexpr brown        { 130 };
		[[maybe_unused]] U8 constexpr brown_dark   {  94 };
		[[maybe_unused]] U8 constexpr white        { 255 };
		[[maybe_unused]] U8 constexpr black        {   0 };
	} //
	
	namespace Themes
	{
		[[maybe_unused]] auto constexpr Default = Theme
		{
			.critical      = { FG{ CC::white       },  BG{ CC::red   },  Mod::bold & Mod::underline },
			.error         = { FG{ CC::red_light   },  BG{           },  Mod::bold                  },
			.warning       = { FG{ CC::yellow      },  BG{           },  Mod::bold                  },
			.important     = { FG{ CC::grey_light  },  BG{           },  Mod::none                  },
			.plain         = { FG{ CC::grey        },  BG{           },  Mod::none                  },
			.unimportant   = { FG{ CC::grey_dark   },  BG{           },  Mod::none                  },
			.header1       = { FG{ CC::white       },  BG{           },  Mod::bold & Mod::underline },
			.header2       = { FG{ CC::white       },  BG{           },  Mod::bold                  },
			.header3       = { FG{ CC::grey_light  },  BG{           },  Mod::bold                  },
			.debug         = { FG{ CC::blue_light  },  BG{           },  Mod::none                  },
			.selected      = { FG{ CC::green_light },  BG{ CC::black },  Mod::bold                  },
			.list_positive = { FG{ CC::green_light },  BG{           },  Mod::bold                  },
			.list_neutral  = { FG{ CC::grey        },  BG{           },  Mod::none                  },
			.list_negative = { FG{ CC::red_dark    },  BG{           },  Mod::bold                  },
			.timestamp     = { FG{ CC::grey        },  BG{           },  Mod::none                  },
			.tag           = { FG{ CC::white       },  BG{           },  Mod::bold & Mod::underline },
			.source        = { FG{ CC::grey_light  },  BG{           },  Mod::bold                  },
		} ;
	} //

// Logging Module:
	enum LogPriority: U8
	{
		any           =   0,
		//------------------
		insignificant =  20,
		low           =  60,
		medium        = 100,
		high          = 140,
		important     = 180,
		critical      = 220,
	} ;
	
	enum LogCategory: U8
	{
		none     = 0b0000'0000,
		//---------------------
		untagged =      1 << 0,
		debug    =      1 << 1,
		info     =      1 << 2,
		file     =      1 << 3,
		network  =      1 << 4,
		core     =      1 << 5,
		warning  =      1 << 6,
		error    =      1 << 7,
		//---------------------
		all      = 0b1111'1111,
	} ;
	
	[[nodiscard]] auto constexpr operator&( LogCategory RO lhs, LogCategory RO rhs ) noexcept -> LogCategory
	{
		return static_cast<LogCategory>(
			static_cast<std::underlying_type_t<LogCategory>>(lhs)
				bitand
			static_cast<std::underlying_type_t<LogCategory>>(rhs)
		);
	} //
	
	[[nodiscard]] auto constexpr operator|( LogCategory RO lhs, LogCategory RO rhs ) noexcept -> LogCategory
	{
		return static_cast<LogCategory>(
			static_cast<std::underlying_type_t<LogCategory>>(lhs)
				bitor
			static_cast<std::underlying_type_t<LogCategory>>(rhs)
		);
	} //
	
	[[nodiscard]] static auto toString( LogCategory classification ) noexcept -> std::string
	{
		auto result      = std::string { "[" };
		auto maybe_affix = [&,counter=0U]( LogCategory RO mask, CStr RO str ) mutable
		{
			if ( not (classification & mask) )
				return;
			if ( counter++ > 0 )
				result += '|';
			result += str;
		}; //
		maybe_affix( LogCategory::untagged, "Untagged" );
		maybe_affix( LogCategory::debug,    "Debug"    );
		maybe_affix( LogCategory::info,     "Info"     );
		maybe_affix( LogCategory::file,     "File"     );
		maybe_affix( LogCategory::network,  "Net"      );
		maybe_affix( LogCategory::core,     "Core"     );
		maybe_affix( LogCategory::warning,  "Warning"  );
		maybe_affix( LogCategory::error,    "Error"    );
		return result += ']';
	} //
	
	class LogHandle; // forward declaration
	class LogContext
	{
		public:
			auto & isLoggingTime( bool RO isToggled ) noexcept
			{
				m_isLoggingTime = isToggled;
				return *this;
			} //
			
			auto isLoggingTime() const noexcept -> bool
			{
				return m_isLoggingTime;
			} //
			
			auto & indentMaxDepth( U8 RO newValue ) noexcept
			{
				m_indentMaxDepth = newValue;
				return *this;
			} //
			
			auto indentMaxDepth() const noexcept -> U8
			{
				return m_indentMaxDepth;
			} //
			
			auto & isIndentingDepth( bool RO isToggled ) noexcept
			{
				m_isIndentingDepth = isToggled;
				return *this;
			} //
			
			auto isIndentingDepth() const noexcept -> bool
			{
				return m_isIndentingDepth;
			} //
			
			auto & minPriority( LogPriority RO newValue ) noexcept
			{
				m_minPriority = newValue;
				return *this;
			} //
			
			auto minPriority() const noexcept
			{
					return m_minPriority;
			} //
			
			auto & filterMask( LogCategory RO newValue ) noexcept
			{
				m_filterMask = newValue;
				return *this;
			} //
			
			auto filterMask() const noexcept
			{
				return m_filterMask;
			} //
			
			auto theme( Theme RO newValue ) noexcept
			{
				m_theme = newValue;
				return *this;
			} //
			
			auto theme() const noexcept -> Theme RO &
			{
				return m_theme;
			} //
			
			auto destination( FILE RW * RO newValue ) noexcept
			{
				m_destination = newValue;
				return *this;
			} //
			
			auto destination() const noexcept
			{
				return m_destination;
			} //
			
			auto & indentSymbol( char RO newValue ) noexcept
			{
				m_indentFmtStr[3] = newValue;
				return *this;
			} //
			
			auto indentSymbol() const noexcept -> char
			{
				return m_indentFmtStr[3];
			} //
			
			auto & indentWidth( U8 RO newValue ) noexcept
			{
				m_indentWidth = newValue;
				return *this;
			} //
			
			auto indentWidth() const noexcept -> U8
			{
				return m_indentWidth;
			} //
			
			auto indentDepth() const noexcept -> U8
			{
				return m_indentDepth;
			} //
			
		private:
			friend class LogHandle;
			
			void incrementDepth() noexcept
			{
				if ( m_indentDepth < 255 )
					++m_indentDepth;
			} //
			
			void decrementDepth() noexcept
			{
				if ( m_indentDepth > 0 )
					--m_indentDepth;
			} //
			
			auto indentWidthAtCurrentDepth() const noexcept -> int
			{
				return std::min( m_indentDepth, m_indentMaxDepth ) * m_indentWidth; // TODO: low priority, clamp?
			} //
			
			// class data members:
				// TODO: make filter optional and have untagged = 0?
				bool         m_isLoggingTime    {             true };
				FILE        *m_destination      {           stdout };
				bool         m_isIndentingDepth {             true };
				U8           m_indentDepth      {                0 };
				U8           m_indentMaxDepth   {                5 };
				char         m_indentFmtStr[10] {     " {:\t>{}} " };
				U8           m_indentWidth      {                1 };
				LogPriority  m_minPriority      { LogPriority::any };
				LogCategory  m_filterMask       { LogCategory::all };
				Theme        m_theme            {  Themes::Default };
	} ;
	
	class LogHandle
	{
		public:
			LogHandle() = delete;
			
			LogHandle( LogHandle RW && ) noexcept = default;
			
			LogHandle( LogHandle RW &handle ):
					m_context       ( handle.m_context ),
					m_affectedCount ( true )
			{
				m_context->incrementDepth();
			} 
				
			LogHandle( LogContext RW &context ):
				m_context       ( &context ),
				m_affectedCount ( false )
			{}
			
			~LogHandle() noexcept
			{
				if ( m_affectedCount )
					m_context->decrementDepth();
			} //
			
			LogHandle & operator=( LogHandle RO &other ) noexcept
			{
				if ( this != &other ) {
					m_context       = other.m_context;
					m_affectedCount = true;
					m_context->incrementDepth();
				}
				return *this;
			} //
			
			LogHandle & operator=( LogHandle RW && ) noexcept = default;
			void operator()( //... )
				std::optional<std::string> RO  maybeSource,   // TODO: string_view
				std::string                RO &message,       // TODO: string_view
				LogPriority                RO  priority       = LogPriority::medium,
				LogCategory                RO  classification = LogCategory::untagged
			)
				noexcept
			{
				if ( shouldLog( priority, classification ) ) {
					maybePrintTimestamp();
					maybeIndent();
					printTag( classification );
					maybePrintSource( maybeSource, priority, classification );
					printMessage( message, priority, classification );
				}
			} //
			
			void operator()( //... )
				std::optional<std::string> RO  maybeSource,   // TODO: string_view
				std::string                RO &message,       // TODO: string_view
				LogCategory                RO  classification,
				LogPriority                RO  priority       = LogPriority::medium
			)
				noexcept
			{
				(*this)( maybeSource, message, priority, classification );
			} //
			
			void operator()( //... )
				std::string RO &message,       // TODO: string_view
				LogPriority RO  priority       = LogPriority::medium,
				LogCategory RO  classification = LogCategory::untagged
			)
				noexcept
			{
				(*this)( {}, message, priority, classification );
			} //
			
			void operator()( //... )
				std::string RO &message,       // TODO: string_view
				LogCategory RO  classification,
				LogPriority RO  priority       = LogPriority::medium
			)
				noexcept
			{
				(*this)( {}, message, priority, classification );
			} //
			
		private:
			[[nodiscard]] auto shouldLog( //... )
				LogPriority RO priority,
				LogCategory RO classification
			)
				const noexcept -> bool
			{
				auto RO isTooLowPriority = priority < m_context->minPriority();
				if ( isTooLowPriority ) return false;
				auto RO isIrrelevantCategory = not (classification & m_context->filterMask());
				if ( isIrrelevantCategory ) return false;
				else return true;
			}
			
			void maybePrintTimestamp() noexcept
			{
				if ( m_context->isLoggingTime() ) {
					auto RO timestamp = [] {
						auto RW  result    =  std::array<char,11> {}; // buffer for [HH:MM:SS]\0
						auto RO  time      =  std::time(nullptr);
						auto RO &localTime = *std::localtime(&time);
						std::sprintf(
							result.data(),
							"[%.2d:%.2d:%.2d]",
							//1900+lt.tm_year, lt.tm_mon, lt.tm_mday, // DEPRECATED: just [hh:mm:ss] should suffice
							localTime.tm_hour, localTime.tm_min, localTime.tm_sec
						);
						return result;
					}();
					fmt::print(
						m_context->destination(),
						" {}",
						m_context->theme().timestamp.apply( timestamp.data() )
					);
				}
			} //
			
			void maybeIndent() noexcept
			{
				if ( m_context->isIndentingDepth() )
					fmt::print(
						m_context->destination(),
						m_context->m_indentFmtStr,
						"",
						m_context->indentWidthAtCurrentDepth()
					);
			} //
			
			void printTag( LogCategory RO classification ) noexcept
			{
				fmt::print(
					m_context->destination(),
					"{} ",
					m_context->theme().tag.apply( toString(classification) )
				);
			} //
			
			void maybePrintSource( //... )
				std::optional<std::string> RO &maybeSource,
				LogPriority                RO  priority,
				LogCategory                RO  classification
			)
				noexcept
			{
				if ( maybeSource ) {
					auto RO sourceStyle = entryStyle(priority,classification) & (Mod::underline & Mod::bold);
					fmt::print(
						m_context->destination(),
						"{}: ",
						sourceStyle.apply( maybeSource.value() )
					);
				}
			} //
			
			void printMessage( //... )
				std::string RO &message,
				LogPriority RO  priority,
				LogCategory RO  classification
			)
				noexcept
			{
				auto RO messageStyle = entryStyle( priority, classification );
				fmt::print(
					m_context->destination(),
					"{} \n",
					messageStyle.apply( message )
				);
			} //
			
			[[nodiscard]] auto entryStyle( // ...
				LogPriority RO priority,
				LogCategory RO classification
			)
				const noexcept -> Style
			{
				if ( classification & LogCategory::error ) {
					if (priority > LogPriority::high )
						return m_context->theme().critical;
					else
						return m_context->theme().error;
				}
				else if ( classification & LogCategory::warning )
					return m_context->theme().warning;
				else if ( classification & LogCategory::debug )
					return m_context->theme().debug;
				else if ( priority < LogPriority::medium )
					return m_context->theme().unimportant;
				else if ( priority > LogPriority::medium )
					return m_context->theme().important;
				else
					return m_context->theme().plain;
			} //
			
		// class data members:
			LogContext *m_context;
			bool        m_affectedCount;
	} ;

class Exception
{
	public:
		//Exception( vk::SystemError RO &error ): _msg( error.what() ) {}
		Exception( std::exception RO &error        ): m_msg( error.what() ) {}
		Exception( std::string    RO &errorMessage ): m_msg( errorMessage ) {}
		auto const &what() const noexcept { return m_msg; }
	
	private:
		std::string m_msg { "N/A" };
} ;

class AppContext
{
	public:
		LogContext logContext;
} ;

namespace glfw
{
	class Instance
	{
		public:
			Instance( AppContext &appContext ):
				m_pAppContext( &appContext )
			{
				MakeLogHandle( logHandle, m_pAppContext->logContext );
				Log( logHandle,
					"glfw::Instance::Instance()", "Constructing GLFW instance...",
					LogPriority::insignificant, LogCategory::info
				) ;
				glfwInit();
			} //
			
			~Instance() noexcept
			{
				MakeLogHandle( logHandle, m_pAppContext->logContext );
				Log( logHandle,
					"glfw::Instance::Instance()", "Destructing GLFW instance...",
					LogPriority::insignificant, LogCategory::info
				) ;
				glfwTerminate();
			} //
		
		private:
			[[maybe_unused]] AppContext *m_pAppContext;
	}; //
	
	class Window {
		public:
			Window( AppContext &appContext, I32 RO width, I32 RO height, CStr RO title = "N/A" ):
				m_pAppContext( &appContext )
			{
				MakeLogHandle( logHandle, m_pAppContext->logContext );
				Log( logHandle,
					"Window::Window()", "Constructing GLFW window...",
					LogPriority::insignificant, LogCategory::info
				) ;
				
				glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
				glfwWindowHint( GLFW_RESIZABLE,  GLFW_FALSE  ); // TODO later
				m_window = glfwCreateWindow( width, height, title, nullptr, nullptr );
				
				Log( logHandle,
					"Window::Window()", "GLFW Window successfully constructed!",
					LogPriority::insignificant, LogCategory::info
				) ;
			} //
			
			~Window() noexcept
			{
				if ( m_window ) {
					glfwDestroyWindow( m_window );
					MakeLogHandle( logHandle, m_pAppContext->logContext );
					Log( logHandle,
						"Window::~Window()", "Destructing GLFW Window...",
						LogPriority::insignificant, LogCategory::info
					) ;
				}
			} //
			
			void rename( CStr RO title )
			{
				glfwSetWindowTitle( m_window, title );
			} //
			
			void resize( I32 RO width, I32 RO height )
			{
				glfwSetWindowSize( m_window, width, height );
			} //
			
			[[nodiscard]] GLFWwindow RW * operator()() noexcept
			{
				return m_window;
			} //
			
			[[nodiscard]] GLFWwindow RO * operator()() const noexcept
			{
				return m_window;
			} //
				
		private:
			[[maybe_unused]] AppContext *m_pAppContext;
			GLFWwindow                  *m_window;
		} ;
} //

namespace vulkan
{
	class Instance
	{
		public:
			Instance(
				AppContext   &appContext,
				Vec<CStr> RO &extensions,
				Vec<CStr> RO &layers
			):
				m_pAppContext { &appContext }
			{
				MakeLogHandle( logHandle, appContext.logContext );
				Log(
					logHandle,
					"vulkan::Instance::Instance()", "Creating Vulkan instance...",
					LogPriority::insignificant, LogCategory::info
				);
				
				// TODO: populate or globalize
				auto RO appInfo = VkApplicationInfo {
					.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
					.pNext              = nullptr,
					.pApplicationName   = "Falkengine Test",
					.applicationVersion = VK_MAKE_VERSION( 1,0,0 ),
					.pEngineName        = "Falkengine",
					.engineVersion      = VK_MAKE_VERSION( 1,0,0 ),
					.apiVersion         = VK_API_VERSION_1_1,
				};
				
				auto RO createInfo = VkInstanceCreateInfo {
					.sType                   =  VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					.pNext                   =  nullptr,
					.flags                   =  0,
					.pApplicationInfo        = &appInfo,
					.enabledLayerCount       =  static_cast<U32>( layers.size() ),
					.ppEnabledLayerNames     =  layers.data(),
					.enabledExtensionCount   =  static_cast<U32>( extensions.size() ),
					.ppEnabledExtensionNames =  extensions.data(),
				};
				
				if ( vkCreateInstance( &createInfo, nullptr, &m_instance ) != VK_SUCCESS )
					throw std::runtime_error( "Failed to create Vulkan instance!" );
				else Log(
					logHandle,
					"vulkan::Instance::Instance()", "Vulkan instance successfully created!",
					LogPriority::insignificant, LogCategory::info
				);
			} //
			
			~Instance() noexcept
			{
				vkDestroyInstance( m_instance, nullptr );
			} //
			
			[[nodiscard]] VkInstance RW & operator()() noexcept
			{
				return m_instance;
			} //
			
			[[nodiscard]] VkInstance RO & operator()() const noexcept
			{
				return m_instance;
			} //
		
		private:
			[[maybe_unused]] AppContext *m_pAppContext;
			VkInstance                   m_instance;
	} ;
} //

namespace // anonymous
{
	[[nodiscard]] auto createDebugUtilsMessengerEXT(
		VkInstance                             instance,
		VkDebugUtilsMessengerCreateInfoEXT RO *createInfo,
		VkAllocationCallbacks              RO *allocator,
		VkDebugUtilsMessengerEXT               *debugMessenger
	)
		-> VkResult // TODO: wrap debug util messenger in RAII
	{
		auto f = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" )
		);
		if ( f )
			return f( instance, createInfo, allocator, debugMessenger );
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	} //
	
	void destroyDebugUtilsMessengerEXT(
		VkInstance                instance,
		VkDebugUtilsMessengerEXT  debugMessenger,
		VkAllocationCallbacks RO *allocator)
	{
		auto f = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" )
		);
		if ( f != nullptr )
			f( instance, debugMessenger, allocator );
	} //
} //

/* class Swapchain
{
	public:
	
	Swapchain( vk::Device logicalDevice ):
		m_logicalDevice ( logicalDevice )
	{}
	
	~Swapchain() noexcept
	{
		for ( auto &framebuffer : m_framebuffers )
			vk::Frame
			vk::destroyFramebuffer(
				m_logicalDevice,
				framebuffer,
				nullptr
			);
		
		vk::freeCommandBuffers(
			m_logicalDevice,
			m_commandPool,
			static_cast<U32>( m_commandBuffers.size() ),
			m_commandBuffers.data()
		);
		
		vk::destroyPipeline(
			m_logicalDevice,
			m_pipeline,
			nullptr
		);
		
		vk::destroyPipelineLayout(
			m_logicalDevice,
			m_pipelineLayout,
			nullptr
		);
		
		vk::destroyRenderPass(
			m_logicalDevice,
			m_renderPass,
			nullptr
		);
		
		for ( auto imageView : m_imageViews )
			vk::destroyImageView(
				m_logicalDevice,
				imageView,
				nullptr
			);
		
		vk::destroySwapchainKHR(
			m_logicalDevice,
			m_swapchain,
			nullptr
		);
	}
	
	private:
	
	VkDevice                    m_logicalDevice;
	std::vector<VkFramebuffer>  m_framebuffers;
} ; */

class App
{
	public:
		App( AppContext &appContext ):
			m_pAppContext  { &appContext },
			m_glfwInstance {  appContext },
			m_window       {  appContext, 800, 600, "Falkengine Test" }
		{
			MakeLogHandle( logHandle, m_pAppContext->logContext );
			Log( logHandle,
				"App::App()", "Constructing App...",
				LogPriority::insignificant, LogCategory::info
			) ;
			
			auto RO applicationInfo = vk::ApplicationInfo {
				.pApplicationName    = "Falkengine Test",
				.applicationVersion  =  VK_MAKE_VERSION(0,1,0),
				.pEngineName         = "Falkengine",
				.engineVersion       =  VK_MAKE_VERSION(0,1,0),
				.apiVersion          =  VK_API_VERSION_1_1
			} ;
			
			m_vkInstance = vk::createInstanceUnique(
				vk::InstanceCreateInfo { .pApplicationInfo = &applicationInfo }
			) ;
			
			Log( logHandle,
				"App::App()", "App created!",
				LogPriority::insignificant, LogCategory::info
			) ;
		} //
		
	private:
		[[maybe_unused]] AppContext *m_pAppContext;
		glfw::Instance               m_glfwInstance;
		glfw::Window                 m_window;
		vk::UniqueInstance           m_vkInstance;
} ;

auto main() -> int
{
	auto appContext = AppContext {
		.logContext = LogContext {}
			.destination      (           stdout )
			.isIndentingDepth (             true )
			.indentWidth      (                1 )
			.indentSymbol     (              '+' )
			.indentMaxDepth   (               10 )
			.isLoggingTime    (             true )
			.minPriority      ( LogPriority::any )
			.filterMask       ( LogCategory::all )
	} ;
	MakeLogHandle( logHandle, appContext.logContext );
	Log( logHandle,
		"main()", "Logger created.",
		LogPriority::insignificant, LogCategory::info
	) ;
	
	try {
		[[maybe_unused]] auto app = App { appContext };
	}
	catch( Exception RO &e ) {
		Log( logHandle, "main()", e.what(), LogPriority::critical, LogCategory::error );
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
} //

// EOF
