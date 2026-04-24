#include "p8_log.hpp"

#include <gtest/gtest.h>
#include <climits>

static const size_t MAX_ARGS = 32;

class c_log_format_test : public ::testing::Test
{
protected:
    struct sP7Trace_Arg mo_args[MAX_ARGS];

    void SetUp() override
    {
        memset(mo_args, 0, sizeof(mo_args));
    }

    size_t parse(const char *ip_fmt)
    {
        return cp8_log::parse_format_string(mo_args, MAX_ARGS, ip_fmt);
    }

    size_t parse_limited(const char *ip_fmt, size_t iz_max)
    {
        return cp8_log::parse_format_string(mo_args, iz_max, ip_fmt);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 1: basic integer specifiers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, int_d)
{
    EXPECT_EQ(parse("%d"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uint32_t));
}

TEST_F(c_log_format_test, int_i)
{
    EXPECT_EQ(parse("%i"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_o)
{
    EXPECT_EQ(parse("%o"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_u)
{
    EXPECT_EQ(parse("%u"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_x)
{
    EXPECT_EQ(parse("%x"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_upper_x)
{
    EXPECT_EQ(parse("%X"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_b)
{
    EXPECT_EQ(parse("%b"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_upper_b)
{
    EXPECT_EQ(parse("%B"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 2: integer with prefixes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, int_hd)
{
    EXPECT_EQ(parse("%hd"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT16);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uint16_t));
}

TEST_F(c_log_format_test, int_hhd)
{
    EXPECT_EQ(parse("%hhd"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT8);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uint8_t));
}

TEST_F(c_log_format_test, int_ld)
{
    EXPECT_EQ(parse("%ld"), 1u);
#if defined(G_OS_WINDOWS)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#elif defined(GTX64)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#endif
}

TEST_F(c_log_format_test, int_lld)
{
    EXPECT_EQ(parse("%lld"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uint64_t));
}

TEST_F(c_log_format_test, int_i64d)
{
    EXPECT_EQ(parse("%I64d"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
}

TEST_F(c_log_format_test, int_i32d)
{
    EXPECT_EQ(parse("%I32d"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, int_upper_id)
{
    EXPECT_EQ(parse("%Id"), 1u);
#ifdef GTX64
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#endif
}

TEST_F(c_log_format_test, int_zd)
{
    EXPECT_EQ(parse("%zd"), 1u);
#ifdef GTX64
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#endif
}

TEST_F(c_log_format_test, int_zu)
{
    EXPECT_EQ(parse("%zu"), 1u);
#ifdef GTX64
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#endif
}

TEST_F(c_log_format_test, int_td)
{
    EXPECT_EQ(parse("%td"), 1u);
#ifdef GTX64
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#endif
}

TEST_F(c_log_format_test, int_jd)
{
    EXPECT_EQ(parse("%jd"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INTMAX);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uintmax_t));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 3: floating point
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, float_f)
{
    EXPECT_EQ(parse("%f"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(double));
}

TEST_F(c_log_format_test, float_upper_f)
{
    EXPECT_EQ(parse("%F"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, float_e)
{
    EXPECT_EQ(parse("%e"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, float_upper_e)
{
    EXPECT_EQ(parse("%E"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, float_g)
{
    EXPECT_EQ(parse("%g"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, float_upper_g)
{
    EXPECT_EQ(parse("%G"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, float_a)
{
    EXPECT_EQ(parse("%a"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, float_upper_a)
{
    EXPECT_EQ(parse("%A"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, long_double_lf)
{
    EXPECT_EQ(parse("%Lf"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_LDOUBLE);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(long double));
}

TEST_F(c_log_format_test, long_double_le)
{
    EXPECT_EQ(parse("%Le"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_LDOUBLE);
}

TEST_F(c_log_format_test, long_double_lg)
{
    EXPECT_EQ(parse("%Lg"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_LDOUBLE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 4: strings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, string_s)
{
    EXPECT_EQ(parse("%s"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_USTR8);
    EXPECT_EQ(mo_args[0].mu_size, 0u);
}

TEST_F(c_log_format_test, string_hs)
{
    EXPECT_EQ(parse("%hs"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_STRA);
    EXPECT_EQ(mo_args[0].mu_size, 0u);
}

TEST_F(c_log_format_test, string_ls)
{
    EXPECT_EQ(parse("%ls"), 1u);
#if defined(G_OS_WINDOWS)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_USTR16);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_USTR32);
#endif
    EXPECT_EQ(mo_args[0].mu_size, 0u);
}

TEST_F(c_log_format_test, string_upper_s)
{
    EXPECT_EQ(parse("%S"), 1u);
#if defined(G_OS_WINDOWS)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_STRA);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_USTR32);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 5: characters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, char_c)
{
    EXPECT_EQ(parse("%c"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_CHAR);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(char));
}

TEST_F(c_log_format_test, char_hc)
{
    EXPECT_EQ(parse("%hc"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_CHAR);
}

TEST_F(c_log_format_test, char_lc)
{
    EXPECT_EQ(parse("%lc"), 1u);
#if defined(G_OS_WINDOWS)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_CHAR16);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_CHAR32);
#endif
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(wchar_t));
}

TEST_F(c_log_format_test, char_upper_c)
{
    EXPECT_EQ(parse("%C"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_CHAR);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 6: pointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, pointer_p)
{
    EXPECT_EQ(parse("%p"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_PVOID);
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(void *));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 7: special cases
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, escaped_percent)
{
    EXPECT_EQ(parse("%%"), 0u);
}

TEST_F(c_log_format_test, escaped_percent_then_d)
{
    EXPECT_EQ(parse("%%d"), 0u);
}

TEST_F(c_log_format_test, star_width_d)
{
    EXPECT_EQ(parse("%*d"), 2u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[1].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, star_precision_f)
{
    EXPECT_EQ(parse("%.*f"), 2u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[1].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, star_width_star_precision_f)
{
    EXPECT_EQ(parse("%*.*f"), 3u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[1].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[2].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, empty_format)
{
    EXPECT_EQ(parse(""), 0u);
}

TEST_F(c_log_format_test, no_specifiers)
{
    EXPECT_EQ(parse("hello world"), 0u);
}

TEST_F(c_log_format_test, fixed_width_int)
{
    EXPECT_EQ(parse("%10d"), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, flags_width_precision_prefix)
{
    EXPECT_EQ(parse("%-+#010.5ld"), 1u);
#if defined(G_OS_WINDOWS)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#elif defined(GTX64)
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT64);
#else
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 8: complex / multiple specifiers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, three_mixed)
{
    EXPECT_EQ(parse("%d %s %f"), 3u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[1].mu_type, P8_ARG_TYPE_USTR8);
    EXPECT_EQ(mo_args[2].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, complex_log_line)
{
    EXPECT_EQ(parse("[%04X] %-20s: %lld (%.2f%%)"), 4u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
    EXPECT_EQ(mo_args[1].mu_type, P8_ARG_TYPE_USTR8);
    EXPECT_EQ(mo_args[2].mu_type, P8_ARG_TYPE_INT64);
    EXPECT_EQ(mo_args[3].mu_type, P8_ARG_TYPE_DOUBLE);
}

TEST_F(c_log_format_test, ten_ints)
{
    EXPECT_EQ(parse("%d%d%d%d%d%d%d%d%d%d"), 10u);
    for(size_t li = 0; li < 10; li++)
    {
        EXPECT_EQ(mo_args[li].mu_type, P8_ARG_TYPE_INT32);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 9: edge cases
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, null_format)
{
    EXPECT_EQ(cp8_log::parse_format_string(mo_args, MAX_ARGS, NULL), 0u);
}

TEST_F(c_log_format_test, zero_max_args)
{
    EXPECT_EQ(parse_limited("%d", 0), 0u);
}

TEST_F(c_log_format_test, buffer_limit_one)
{
    EXPECT_EQ(parse_limited("%d %d", 1), 1u);
    EXPECT_EQ(mo_args[0].mu_type, P8_ARG_TYPE_INT32);
}

TEST_F(c_log_format_test, trailing_percent)
{
    EXPECT_EQ(parse("%"), 0u);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Group 10: bSize correctness
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_log_format_test, size_int32)
{
    parse("%d");
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uint32_t));
}

TEST_F(c_log_format_test, size_int64)
{
    parse("%lld");
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(uint64_t));
}

TEST_F(c_log_format_test, size_string_variable)
{
    parse("%s");
    EXPECT_EQ(mo_args[0].mu_size, 0u);
}

TEST_F(c_log_format_test, size_string_fixed_width)
{
    parse("%.10s");
    EXPECT_EQ(mo_args[0].mu_size, 1u);
}

TEST_F(c_log_format_test, size_pointer)
{
    parse("%p");
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(void *));
}

TEST_F(c_log_format_test, size_long_double)
{
    parse("%Lf");
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(long double));
}

TEST_F(c_log_format_test, size_double)
{
    parse("%f");
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(double));
}

TEST_F(c_log_format_test, size_char)
{
    parse("%c");
    EXPECT_EQ(mo_args[0].mu_size, P8_SIZE_OF_ARG(char));
}
