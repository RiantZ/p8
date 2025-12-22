#pragma once

#include "TsHelpers.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

/// @brief callback function type to get high-resolution system timer frequency
/// @param i_pCtx [in] calling software context
/// @return frequency in hertz
typedef uint64_t (*fnuP8getTimerFrequency)(void *i_pCtx);

/// @brief callback function type to get high-resolution system timer value
/// @param i_pCtx [in] calling software context
/// @return timer value
/// WARNING: it is not recommended to take ANY LOCK inside function, performance penalties will be significant!
typedef uint64_t (*fnuP8getTimerValue)(void *i_pCtx);

/// @brief P8 configuration structure
struct stP8config
{
    const char *pJsonConfig; //! Json config data (content), example: "examples/config.json"

    //! Fields provides the way to reimplement internal mechanism of timestamping, by default it is reccommended to
    //! provide nullptr as arguments. But if you want to syncronize different streams and clocks - it might be used.
    void                  *pCtxTimer;        //! Context for timer functions, nullptr accepted
    fnuP8getTimerFrequency cbTimerFrequency; //! callback to get system high-resolution timer frequency, or nullptr
    fnuP8getTimerValue     cbTimerValue;     //! callback to get system high-resolution timer value or nullptr
};

/// @brief P8 initialization function, should be called once at CPU startup
/// @param i_pConfig [in] configuration
/// @return true - P8 has been initialized, false - failure
bool P8Initialize(const struct stP8config *i_pConfig);

/// @brief
/// @return
bool P8GetInitialized();

/// @brief Function allows to flush (deliver) not  delivered/saved  P8  buffers  for  all opened clients and related
/// channels owned by process in CASE OF your app/proc. crash. This function do not call system  memory allocation
/// functions  only  writes to file/socket. Classical scenario: your application has been crushed, you catch the moment
/// of crush and call this function once. Has to be used if integrator implements own crash handling mechanism.
/// N.B.: DO NOT USE OTHER P8 FUNCTION AFTER CALLING OF THIS FUNCTION
void P8ExceptionalFlush();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         P8::Tracing structures & functions                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enum describes different verbosity & trace levels
enum eP8Level
{
    eP8Trace = 0, //! minimum verbosity level, maximum information to be transmitted
    eP8Debug,     //! debug messages, info, up to critical
    eP8Info,      //! info messages, warning, up to critical
    eP8Warning,   //! warning messages, error, up to critical
    eP8Error,     //! only error & critical messages
    eP8Critical,  //! only critical messages

    eP8LevelsCount
};

/// @brief P8 trace module type, used to create different modules for tracing. Each module has it's own name &
/// verbosity
typedef void *hP8Module;

/// @brief P8 invalid module ID value*/
#define P8_MODULE_INVALID_ID nullptr

#define P8TRC(i_hModule, i_pFormatStr, ...)                                                                           \
    P8TrcSent(eP8Trace, i_hModule, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8DBG(i_hModule, i_pFormatStr, ...)                                                                           \
    P8TrcSent(eP8Debug, i_hModule, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8INF(i_hModule, i_pFormatStr, ...)                                                                           \
    P8TrcSent(eP8Info, i_hModule, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8WRN(i_hModule, i_pFormatStr, ...)                                                                           \
    P8TrcSent(eP8Warning, i_hModule, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8ERR(i_hModule, i_pFormatStr, ...)                                                                           \
    P8TrcSent(eP8Error, i_hModule, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8CRT(i_hModule, i_pFormatStr, ...)                                                                           \
    P8TrcSent(eP8Critical, i_hModule, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)

/// @brief set trace verbosity, traces with less priority will be rejected. You may set verbosity online from Baical
/// server. See documentation for details.
/// @param i_hModule [in] module, if NULL - default verbosity will be set
/// @param i_eVerbosity [in] verbosity level
void P8TrcSetVerbosity(hP8Module i_hModule, enum eP8Level i_eVerbosity);

enum eP8Level P8TrcGetVerbosity(hP8Module i_hModule);

/// @brief function used to specify name for current thread, allows to have nice trace message formatting on Baical
/// server. Call the function from the newly created thread & call P8TrcUnregisterCurrentThread() right before  thread
/// destroying
/// @param i_pName [in] thread name
/// @return true - successful registration, false - failure
bool P8TrcRegisterCurrentThread(const char *i_pName);

/// @brief function used to unregister thread name (called on thread exit)
void P8TrcUnRegisterCurrentThread();

/// @brief function is used to register trace module. Modules are used to group trace messages by modules, use the same
///       verbosity level per module and to have nice formatting on Baical side - each trace will be marked with module
///       name. If module with such name is already registered - existing handle will be returned
/// @param i_pName [in] module name
/// @param i_eVerbosity [in] module verbosity
/// @param o_hModule [out] module handle
/// @return true - successful registration, false - failure
bool P8TrcRegisterModule(const char *i_pName, enum eP8Level i_eVerbosity, hP8Module *o_hModule);

/// @brief function is used to find trace module by name. Modules are used to group trace messages by modules, use the
/// name verbosity level per module and to have nice formatting on Baical side - each trace will be marked with module
///       name. If module with such name is already registered - existing handle will be returned
/// @param i_pName [in] module name
/// @param i_eVerbosity [in] module verbosity
/// @param o_hModule [out] module handle
/// @return true - successful registration, false - failure
hP8Module P8TrcFindModule(const char *i_pName);

/// @brief Function for internal usage, please use macros P8TRC, P8DBG, etc. instead.
/// @param i_eLevel [in] trace level
/// @param i_hModule [in] module
/// @param i_uLine [in] source code line index
/// @param i_pFunction [in] source code function name
/// @param i_pFormat [in] format string, for format specification please refer to documentation, all standard format
/// options are supported, and in addition few custom
/// @param ... [in] variable arguments list
/// @return true - success, false - failure
bool P8TrcSent(enum eP8Level i_eLevel,
               hP8Module     i_hModule,
               uint32_t      i_uLine,
               const char   *i_pFile,
               const char   *i_pFunction,
               const char   *i_pFormat,
               ...);

/// @brief Function for internal usage, please use macros P8TRC, P8DBG, etc. instead.
/// @param i_eLevel [in] trace level
/// @param i_hModule [in] module
/// @param i_uLine [in] source code line index
/// @param i_pFunction [in] source code function name
/// @param i_ppFormat [in] format string address, for format specification please refer to documentation, all standard
/// format options are supported, and in addition few custom
/// @param i_pVa_List [in] variable arguments list
/// @return true - success, false - failure
bool P8TrcSentEmb(enum eP8Level i_eLevel,
                  hP8Module     i_hModule,
                  uint32_t      i_uLine,
                  const char   *i_pFile,
                  const char   *i_pFunction,
                  const char  **i_ppFormat,
                  va_list      *i_pVa_List);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         P8::Telemetry structures & functions                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief P8 telemetry ID type
typedef uint16_t hP8TelId;

/// @brief Invalid telemetry counter ID
#define P8_TELEMETRY_INVALID_ID ((hP8TelId) ~((hP8TelId)0))

/// @brief function to register telemetry counter
/// @param i_pName [in] counter name
/// @param i_dbMin [in] min counter value
/// @param i_dbbAlarmMin [in] value below which counter value will be interpreted as alarm signal on Baical's side
/// @param i_dbMax [in] max counter value
/// @param i_dbAlarmMax [in] value above which counter value will be interpreted as alarm signal on Baical's side
/// @param i_bOn [in] default counter state (true - on, false - off), can be changed later in real-time from Baical
/// @param o_pID [out] counter ID
/// @return true - success, false - failure
bool P8TelCreateCounter(const char *i_pName,
                        double      i_dbMin,
                        double      i_dbbAlarmMin,
                        double      i_dbMax,
                        double      i_dbAlarmMax,
                        bool        i_bOn,
                        hP8TelId   *o_pID);

/// @brief function to sent counter sample
/// @param i_hID [in] counter ID
/// @param i_tValue [in] sample value
/// @return true - success, false - failure
bool P8TelSentSample(hP8TelId i_hID, double i_tValue);

/// @brief function to find counter ID by name (case sensitive)
/// @param i_pName [in] counter name
/// @param o_pID [out] counter ID
/// @return true - success, false - failure
bool P8TelFindCounter(const char *i_pName, hP8TelId *o_pID);

/// @brief function to enable counter
/// @param i_hID [in] counter ID
void P8TelManageCounter(hP8TelId i_hID, bool i_bEnable);

/// @brief function to retrieve enable state of the counter
/// @param i_hID [in] counter ID
/// @return true - enabled, false - disabled
bool P8TelIsCounterEnabled(hP8TelId i_hID);

/// @brief get counter's name
/// @param i_hID [in] counter ID
/// @return name
const char *P8TelGetCounterName(hP8TelId i_hID);
