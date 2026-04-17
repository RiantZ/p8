#include "p8_config.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace
{
/// @brief Unique per-run scratch directory under the OS temp location.
std::filesystem::path make_scratch_dir()
{
    const auto lo_root = std::filesystem::temp_directory_path()
                         / ("p8_config_test_"
                            + std::to_string(static_cast<uint64_t>(::testing::UnitTest::GetInstance()->random_seed()))
                            + "_" + std::to_string(static_cast<uint64_t>(std::rand())));
    std::filesystem::create_directories(lo_root);
    return lo_root;
}
} // namespace

class c_p8_config_read_file_test : public ::testing::Test
{
protected:
    std::filesystem::path mo_tmp_dir;

    void SetUp() override
    {
        mo_tmp_dir = make_scratch_dir();
    }

    void TearDown() override
    {
        std::error_code lo_ec;
        std::filesystem::remove_all(mo_tmp_dir, lo_ec);
    }

    /// @brief Write `ir_content` bytes to `mo_tmp_dir / ir_name` (binary) and return the path.
    std::filesystem::path write_file(const std::string &ir_name, std::string_view ir_content)
    {
        const auto    lo_path = mo_tmp_dir / ir_name;
        std::ofstream lo_out(lo_path, std::ios::binary);
        lo_out.write(ir_content.data(), static_cast<std::streamsize>(ir_content.size()));
        lo_out.close();
        return lo_path;
    }
};

TEST_F(c_p8_config_read_file_test, null_path_returns_null)
{
    size_t lz_size = 42;
    char  *lp_buf  = p8_config_read_file(nullptr, &lz_size);
    EXPECT_EQ(lp_buf, nullptr);
    EXPECT_EQ(lz_size, 0u);
}

TEST_F(c_p8_config_read_file_test, null_path_and_null_size_returns_null)
{
    EXPECT_EQ(p8_config_read_file(nullptr, nullptr), nullptr);
}

TEST_F(c_p8_config_read_file_test, nonexistent_file_returns_null)
{
    const auto lo_path = mo_tmp_dir / "does_not_exist.json";
    size_t     lz_size = 42;
    char      *lp_buf  = p8_config_read_file(lo_path.c_str(), &lz_size);
    EXPECT_EQ(lp_buf, nullptr);
    EXPECT_EQ(lz_size, 0u);
}

TEST_F(c_p8_config_read_file_test, empty_file_returns_terminated_zero_length_buffer)
{
    const auto lo_path = write_file("empty.txt", "");
    size_t     lz_size = 42;
    char      *lp_buf  = p8_config_read_file(lo_path.c_str(), &lz_size);
    ASSERT_NE(lp_buf, nullptr);
    EXPECT_EQ(lz_size, 0u);
    EXPECT_EQ(lp_buf[0], '\0');
    std::free(lp_buf);
}

TEST_F(c_p8_config_read_file_test, text_file_round_trips_and_is_nul_terminated)
{
    const std::string lo_content = R"({"k":"v","n":42,"arr":[1,2,3]})";
    const auto        lo_path    = write_file("cfg.json", lo_content);

    size_t lz_size               = 0;
    char  *lp_buf                = p8_config_read_file(lo_path.c_str(), &lz_size);
    ASSERT_NE(lp_buf, nullptr);
    EXPECT_EQ(lz_size, lo_content.size());
    EXPECT_EQ(std::string(lp_buf, lz_size), lo_content);
    EXPECT_EQ(lp_buf[lz_size], '\0');
    std::free(lp_buf);
}

TEST_F(c_p8_config_read_file_test, binary_content_preserved_with_embedded_nuls)
{
    const std::string lo_content("\x01\x00\x02\x00\x03", 5);
    const auto        lo_path = write_file("bin.dat", lo_content);

    size_t lz_size            = 0;
    char  *lp_buf             = p8_config_read_file(lo_path.c_str(), &lz_size);
    ASSERT_NE(lp_buf, nullptr);
    EXPECT_EQ(lz_size, 5u);
    EXPECT_EQ(std::memcmp(lp_buf, lo_content.data(), 5), 0);
    EXPECT_EQ(lp_buf[5], '\0');
    std::free(lp_buf);
}

TEST_F(c_p8_config_read_file_test, null_size_out_is_allowed)
{
    const auto lo_path = write_file("abc.txt", "abc");

    char *lp_buf       = p8_config_read_file(lo_path.c_str(), nullptr);
    ASSERT_NE(lp_buf, nullptr);
    EXPECT_STREQ(lp_buf, "abc");
    std::free(lp_buf);
}

TEST_F(c_p8_config_read_file_test, subdirectory_path_works)
{
    const auto lo_sub = mo_tmp_dir / "nested" / "deeper";
    std::filesystem::create_directories(lo_sub);
    std::ofstream lo_out(lo_sub / "file.txt", std::ios::binary);
    lo_out << "payload";
    lo_out.close();

    size_t lz_size = 0;
    char  *lp_buf  = p8_config_read_file((lo_sub / "file.txt").c_str(), &lz_size);
    ASSERT_NE(lp_buf, nullptr);
    EXPECT_EQ(lz_size, 7u);
    EXPECT_STREQ(lp_buf, "payload");
    std::free(lp_buf);
}
