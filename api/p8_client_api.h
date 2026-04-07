#pragma once

#include "ts_helpers.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

/// @brief callback function type to get high-resolution system timer frequency
/// @param ip_ctx [in] calling software context
/// @return frequency in hertz
typedef uint64_t (*l_p8_get_timer_frequency)(void *ip_ctx);

/// @brief callback function type to get high-resolution system timer value
/// @param ip_ctx [in] calling software context
/// @return timer value
/// WARNING: it is not recommended to take ANY LOCK inside function, performance penalties will be significant!
typedef uint64_t (*l_p8_get_timer_value)(void *ip_ctx);

/// @brief P8 configuration structure
struct s_p8_config
{
    const char *mp_json_config; //! Json config data (content), example: "examples/config.json"

    //! Fields provides the way to reimplement internal mechanism of timestamping, by default it is reccommended to
    //! provide nullptr as arguments. But if you want to syncronize different streams and clocks - it might be used.
    void                    *mp_ctx_timer;       //! Context for timer functions, nullptr accepted
    l_p8_get_timer_frequency ml_timer_frequency; //! callback to get system high-resolution timer frequency, or nullptr
    l_p8_get_timer_value     ml_timer_value;     //! callback to get system high-resolution timer value or nullptr
};

/// @brief P8 initialization function, should be called once at CPU startup
/// @param ip_config [in] configuration
/// @return true - P8 has been initialized, false - failure
bool p8_initialize(const struct s_p8_config *ip_config);

/// @brief
/// @return
bool p8_get_initialized();

/// @brief Function allows to flush (deliver) not  delivered/saved  P8  buffers  for  all opened clients and related
/// channels owned by process in CASE OF your app/proc. crash. This function do not call system  memory allocation
/// functions  only  writes to file/socket. Classical scenario: your application has been crushed, you catch the moment
/// of crush and call this function once. Has to be used if integrator implements own crash handling mechanism.
/// N.B.: DO NOT USE OTHER P8 FUNCTION AFTER CALLING OF THIS FUNCTION
void p8_exceptional_flush();

/// @brief function used to specify name for current thread, allows to have nice log/trace message formatting on Baical
/// server. Call the function from the newly created thread & call p8_trc_unregister_current_thread() right before
/// thread destroying
/// @param ip_name [in] thread name
/// @return true - successful registration, false - failure
bool p8_register_current_thread(const char *ip_name);

/// @brief function used to unregister thread name (called on thread exit)
void p8_unregister_current_thread();

#ifdef __cplusplus
/// @brief RAII guard that registers the current thread name on construction and unregisters it on
/// destruction. Designed for zero overhead beyond the two underlying C calls — no virtual
/// dispatch, no heap allocation, no exceptions.
class cp8_thread final
{
public:
    explicit cp8_thread(const char *ip_name) noexcept
    {
        p8_log_register_current_thread(ip_name);
    }

    ~cp8_thread() noexcept
    {
        p8_log_unregister_current_thread();
    }

    cp8_thread(const cp8_thread &)            = delete;
    cp8_thread &operator=(const cp8_thread &) = delete;
    cp8_thread(cp8_thread &&)                 = delete;
    cp8_thread &operator=(cp8_thread &&)      = delete;
};
#endif // __cplusplus

/// @brief Key-value attribute attached to a log, trace or metric record.
///
/// Both pointers must remain valid and point to null-terminated UTF-8 strings
/// for the entire lifetime of the record (i.e. until the log call returns).
/// Passing string literals is the recommended zero-allocation approach.
struct s_attr
{
    const char *mp_name;  ///< Attribute name,  e.g. "user_id". Must not be nullptr.
    const char *mp_value; ///< Attribute value, e.g. "42".      Must not be nullptr.
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         P8::Logging structures & functions                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enum describes different verbosity & logs levels
enum e_p8_level
{
    e_p8_trace0 = 0, //! minimum verbosity level, maximum information to be transmitted
    e_p8_trace1,     //!
    e_p8_trace2,     //!
    e_p8_trace3,     //!
    e_p8_debug0,     //! debug messages, info, up to critical
    e_p8_debug1,     //!
    e_p8_debug2,     //!
    e_p8_debug3,     //!
    e_p8_info0,      //! info messages, warning, up to critical
    e_p8_info1,      //!
    e_p8_info2,      //!
    e_p8_info3,      //!
    e_p8_warning0,   //! warning messages, error, up to critical
    e_p8_warning1,   //!
    e_p8_warning2,   //!
    e_p8_warning3,   //!
    e_p8_error0,     //! only error & critical messages
    e_p8_error1,     //!
    e_p8_error2,     //!
    e_p8_error3,     //!
    e_p8_critical0,  //! only critical messages
    e_p8_critical1,  //!
    e_p8_critical2,  //!
    e_p8_critical3,  //!

    e_p8_levels_count
};

/// @brief P8 Log module type, used to create different modules for logging. Each module has it's own name &
/// verbosity
typedef void *p_p8_module;

/// @brief P8 invalid module ID value
#define P8_MODULE_INVALID_ID nullptr

// Trace & ID to have parent,
// Logs + trace ID + attributes + more levels

#define P8TRC0(ip_module, ip_format_str, ...)                                                                         \
    p8_log_sent(e_p8_trace0, ip_module, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8DBG0(ip_module, ip_format_str, ...)                                                                         \
    p8_log_sent(e_p8_debug0, ip_module, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8INF0(ip_module, ip_format_str, ...)                                                                         \
    p8_log_sent(e_p8_info0, ip_module, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8WRN0(ip_module, ip_format_str, ...)                                                                         \
    p8_log_sent(e_p8_warning0, ip_module, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8ERR0(ip_module, ip_format_str, ...)                                                                         \
    p8_log_sent(e_p8_error0, ip_module, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)
#define P8CRT0(ip_module, ip_format_str, ...)                                                                         \
    p8_log_sent(e_p8_critical0, ip_module, (uint32_t)__LINE__, __FILE__, __FUNCTION__, ##__VA_ARGS__)

/// @brief set module log verbosity, logs with less priority will be rejected. You may set verbosity online from Baical
/// server. See documentation for details.
/// @param ip_module [in] module, if nullptr - default verbosity will be set for entire p8
/// @param ie_verbosity [in] verbosity level
void p8_log_set_verbosity(p_p8_module ip_module, enum e_p8_level ie_verbosity);

enum e_p8_level p8_log_get_verbosity(p_p8_module ip_module);

/// @brief function is used to register log module. Modules are used to group log messages by modules, use the same
///       verbosity level per module and to have nice formatting on Baical side - each log will be marked with module
///       name. If module with such name is already registered - existing handle will be returned
/// @param ip_name [in] module name
/// @param ie_verbosity [in] module verbosity
/// @param op_module [out] module handle
/// @return true - successful registration, false - failure
bool p8_log_register_module(const char *ip_name, enum e_p8_level ie_verbosity, p_p8_module *op_module);

/// @brief function is used to find log module by name. Modules are used to group log messages by modules, use the
/// same verbosity level per module and to have nice formatting on Baical side - each log will be marked with module
///       name. If module with such name is already registered - existing handle will be returned
/// @param ip_name [in] module name
/// @return module handle
p_p8_module p8_log_find_module(const char *ip_name);

/// @brief Function for internal usage, please use macros P8TRC, P8DBG, etc. instead.
/// @param ie_level [in] log level
/// @param ip_module [in] module
/// @param iu_trace_id [in] distributed trace identifier that correlates this log record; pass 0 when
///                         the log is not associated with any trace
/// @param iu_line [in] source code line index
/// @param ip_file [in] source code file name
/// @param ip_function [in] source code function name
/// @param ip_format [in] format string, for format specification please refer to documentation, all standard format
/// options are supported, and in addition few custom
/// @param iz_attrs [in] number of elements in ip_attrs array; pass 0 when no attributes are attached
/// @param ip_attrs [in] pointer to array of s_attr of length iz_attrs; both mp_name and mp_value inside each s_attr
///                      must not be nullptr and must remain valid until the call returns
/// @param ... [in] variable arguments list
/// @return true - success, false - failure
bool p8_log_sent(enum e_p8_level ie_level,
                 p_p8_module     ip_module,
                 uint64_t        iu_trace_id,
                 uint32_t        iu_line,
                 const char     *ip_file,
                 const char     *ip_function,
                 size_t          iz_attrs,
                 const s_attr   *ip_attrs,
                 const char     *ip_format,
                 ...);

/// @brief Function for internal usage, please use macros P8TRC, P8DBG, etc. instead.
/// @param ie_level [in] log level
/// @param ip_module [in] module
/// @param iu_trace_id [in] distributed trace identifier that correlates this log record; pass 0 when
///                         the log is not associated with any trace
/// @param iu_line [in] source code line index
/// @param ip_file [in] source code file name
/// @param ip_function [in] source code function name
/// @param iz_attrs [in] number of elements in ip_attrs array; pass 0 when no attributes are attached
/// @param ip_attrs [in] pointer to array of s_attr of length iz_attrs; both mp_name and mp_value inside each s_attr
///                      must not be nullptr and must remain valid until the call returns
/// @param ip_format [in] format string address, for format specification please refer to documentation, all standard
/// format options are supported, and in addition few custom
/// @param ip_va_list [in] variable arguments list
/// @return true - success, false - failure
bool p8_log_sent_emb(enum e_p8_level ie_level,
                     p_p8_module     ip_module,
                     uint64_t        iu_trace_id,
                     uint32_t        iu_line,
                     const char     *ip_file,
                     const char     *ip_function,
                     size_t          iz_attrs,
                     const s_attr   *ip_attrs,
                     const char    **ip_format,
                     va_list        *ip_va_list);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         P8::Trace structures & functions                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: add condition MACRO to switch off traces

/// @brief Start a trace
#define P8TRC_BEGIN(iu_parent_id, iz_attrs, ip_attrs, ip_fmt, ...)                                                    \
    p8_trc_begin((iu_parent_id),                                                                                      \
                 (uint32_t)__LINE__,                                                                                  \
                 __FILE__,                                                                                            \
                 __FUNCTION__,                                                                                        \
                 (iz_attrs),                                                                                          \
                 (ip_attrs),                                                                                          \
                 (ip_fmt),                                                                                            \
                 ##__VA_ARGS__)

/// @brief End a trace by its trace ID.
#define P8TRC_END(iu_trace_id) p8_trc_end(iu_trace_id)

/// @brief Start a trace span. For internal usage — prefer P8TRC_BEGIN / P8TRC_SCOPE macros.
/// @param iu_parent_trace_id [in] parent span ID for nested traces; pass 0 for a root span
/// @param iu_line            [in] source code line index
/// @param ip_file            [in] source code file name
/// @param ip_function        [in] source code function name
/// @param iz_attrs           [in] number of elements in ip_attrs; pass 0 when no attributes
/// @param ip_attrs           [in] array of s_attr of length iz_attrs; may be nullptr when iz_attrs == 0
/// @param ip_function_args   [in] printf-style format string describing call arguments; may be nullptr
/// @param ...                [in] arguments for ip_function_args format string
/// @return assigned trace ID (non-zero on success, 0 on failure)
uint64_t p8_trc_begin(uint64_t      iu_parent_trace_id,
                      uint32_t      iu_line,
                      const char   *ip_file,
                      const char   *ip_function,
                      size_t        iz_attrs,
                      const s_attr *ip_attrs,
                      const char   *ip_function_args,
                      ...);

/// @brief End a trace span. For internal usage — prefer P8TRC_SCOPE macro (RAII) or P8TRC_END.
/// @param iu_trace_id [in] trace ID returned by p8_trc_begin; passing 0 is a no-op
/// @return true - success, false - failure
bool p8_trc_end(uint64_t iu_trace_id);

#ifdef __cplusplus
    /// @brief Declare a named cp8_trace_guard with structured attributes.
    #define P8TRC_SCOPE(var_name, iu_parent_id, iz_attrs, ip_attrs, ip_fmt, ...)                                      \
        cp8_trace var_name((iu_parent_id),                                                                            \
                           (uint32_t)__LINE__,                                                                        \
                           __FILE__,                                                                                  \
                           __FUNCTION__,                                                                              \
                           (iz_attrs),                                                                                \
                           (ip_attrs),                                                                                \
                           (ip_fmt),                                                                                  \
                           ##__VA_ARGS__)

    /// @brief Declare a named cp8_trace_guard with structured attributes.
    #define P8TRC(var_name, iu_parent_id, iz_attrs, ip_attrs)                                                         \
        cp8_trace var_name((iu_parent_id), (uint32_t)__LINE__, __FILE__, __FUNCTION__, (iz_attrs), (ip_attrs), )

/// @brief RAII guard that calls p8_trc_begin on construction and p8_trc_end on destruction.
/// Stores the assigned trace ID so it can be forwarded to p8_log_sent for span correlation.
/// Designed for zero overhead beyond the two underlying C calls — no virtual dispatch,
/// no heap allocation, no exceptions.
class cp8_trace final
{
public:
    explicit cp8_trace(uint64_t      iu_parent_trace_id,
                       uint32_t      iu_line,
                       const char   *ip_file,
                       const char   *ip_function,
                       size_t        iz_attrs         = 0,
                       const s_attr *ip_attrs         = nullptr,
                       const char   *ip_function_args = nullptr) noexcept
        : mu_trace_id(
              p8_trc_begin(iu_parent_trace_id, iu_line, ip_file, ip_function, iz_attrs, ip_attrs, ip_function_args))
    {
    }

    ~cp8_trace() noexcept
    {
        p8_trc_end(mu_trace_id);
    }

    /// Returns the trace ID assigned by p8_trc_begin; pass it to p8_log_sent / P8INF0 etc.
    uint64_t get_trace_id() const noexcept
    {
        return mu_trace_id;
    }

    cp8_trace(const cp8_trace &)            = delete;
    cp8_trace &operator=(const cp8_trace &) = delete;
    cp8_trace(cp8_trace &&)                 = delete;
    cp8_trace &operator=(cp8_trace &&)      = delete;

private:
    uint64_t mu_trace_id;
};
#endif // __cplusplus

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         P8::Metrics structures & functions                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief P8 Metric ID type
typedef uint16_t u_p8_mtk_id;

/// @brief Invalid telemetry counter ID
#define P8_METRIC_INVALID_ID ((u_p8_mtk_id) ~((u_p8_mtk_id)0))

struct s_metric_attr
{
    void       *mp_user_context;
    const char *mp_name;
    const char *mp_description;
    const char *mp_unit;
    const char *mp_type;
    int64_t     mi_min;
    int64_t     mi_max;
    // callback
};

// types: (single val, mutlival) / observer + increment + on-off (cyclogram)

/// @brief function to register telemetry counter
/// @param ip_name [in] counter name
/// @param id_min [in] min counter value
/// @param id_alarm_min [in] value below which counter value will be interpreted as alarm signal on Baical's side
/// @param id_max [in] max counter value
/// @param id_alarm_max [in] value above which counter value will be interpreted as alarm signal on Baical's side
/// @param ib_on [in] default counter state (true - on, false - off), can be changed later in real-time from Baical
/// @param op_id [out] counter ID
/// @return true - success, false - failure
bool p8_tel_create_counter(const char *ip_name,
                           double      id_min,
                           вщгиду шв_ьштб

                           double       id_alarm_min,
                           double       id_max,
                           double       id_alarm_max,
                           bool         ib_on,
                           u_p8_mtk_id *op_id);

/// @brief function to sent counter sample
/// @param iu_id [in] counter ID
/// @param id_value [in] sample value
/// @return true - success, false - failure
bool p8_tel_sent_sample(u_p8_mtk_id iu_id, double id_value);

/// @brief function to find counter ID by name (case sensitive)
/// @param ip_name [in] counter name
/// @param op_id [out] counter ID
/// @return true - success, false - failure
bool p8_tel_find_counter(const char *ip_name, u_p8_mtk_id *op_id);

/// @brief function to enable counter
/// @param iu_id [in] counter ID
void p8_tel_manage_counter(u_p8_mtk_id iu_id, bool ib_enable);

/// @brief function to retrieve enable state of the counter
/// @param iu_id [in] counter ID
/// @return true - enabled, false - disabled
bool p8_tel_is_counter_enabled(u_p8_mtk_id iu_id);

/// @brief get counter's name
/// @param iu_id [in] counter ID
/// @return name
const char *p8_tel_get_counter_name(u_p8_mtk_id iu_id);
