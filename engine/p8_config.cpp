#include "p8_config.h"

#include "kit/ts_helpers.h"

#include <cstdio>
#include <cstdlib>

extern "C"
{

    char *p8_config_read_file(const tXCHAR *ip_path, size_t *op_size)
    {
        if(op_size)
        {
            *op_size = 0;
        }
        if(!ip_path)
        {
            return nullptr;
        }

#ifdef G_OS_WINDOWS
        FILE *lp_file = _wfopen(ip_path, L"rb");
#else
        FILE *lp_file = std::fopen(ip_path, "rb");
#endif
        if(!lp_file)
        {
            return nullptr;
        }

        if(std::fseek(lp_file, 0, SEEK_END) != 0)
        {
            std::fclose(lp_file);
            return nullptr;
        }
        const long li_pos = std::ftell(lp_file);
        if(li_pos < 0)
        {
            std::fclose(lp_file);
            return nullptr;
        }
        std::rewind(lp_file);

        const size_t lz_size = static_cast<size_t>(li_pos);
        char        *lp_buf  = static_cast<char *>(std::malloc(lz_size + 1));
        if(!lp_buf)
        {
            std::fclose(lp_file);
            return nullptr;
        }

        const size_t lz_read = std::fread(lp_buf, 1, lz_size, lp_file);
        std::fclose(lp_file);

        if(lz_read != lz_size)
        {
            std::free(lp_buf);
            return nullptr;
        }

        lp_buf[lz_size] = '\0';

        if(op_size)
        {
            *op_size = lz_size;
        }
        return lp_buf;
    }

} // extern "C"
