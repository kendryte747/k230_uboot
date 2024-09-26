#include <common.h>
#include <div64.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>

#include <kburn.h>

#include <kendryte/pufs/pufs_rt/pufs_rt.h>
#include <stdint.h>
#include <stdio.h>

#define KBURN_OTP_SIZE  (1024)

struct kburn_otp_priv {
    uint8_t otp_data[KBURN_OTP_SIZE];
};

static int otp_get_medium_info(struct kburn *burn)
{
    burn->medium_info.valid = 1;
    burn->medium_info.wp = 0;

    burn->medium_info.capacity = KBURN_OTP_SIZE;
    burn->medium_info.erase_size = 4;
    burn->medium_info.blk_size = 4;
    burn->medium_info.timeout_ms = 5000;
    burn->medium_info.type = KBURN_MEDIA_OTP;

    return 0;
}

static int otp_read_medium(struct kburn *burn, u64 offset, void *buf, u64 *len)
{
    pr_err("TODO\n");

    return 0;
}

static int otp_erase_medium(struct kburn *burn, u64 offset, u64 *len)
{
    pr_err("otp not support erase\n");

    return 0;
}


// Function to check if new data changes any bit from 1 to 0 compared to old data
static inline int check_data(uint32_t old_data, uint32_t new_data) {
    if ((old_data & ~new_data) != 0) {
        // This means some bits are changing from 1 to 0
        return 1; // Error: bits are changing from 1 to 0
    }
    return 0; // No error
}

static int otp_write_medium(struct kburn *burn, u64 offset, const void *buf, u64 *len)
{
    struct kburn_otp_priv *priv = (struct kburn_otp_priv *)burn->dev_priv;

    volatile bool do_write = false;
    uint32_t old_data, new_data, write_addr;

    uint32_t *old_data_buffer = (uint32_t *)priv->otp_data;
    uint32_t *new_data_buffer = (uint32_t *)buf;
    const uint32_t *new_data_buffer_end = new_data_buffer + (*len / 4);

    if ((offset & 3) || (*len & 3)) {
        pr_err("write otp address(%lld) and length(%lld) should align to 4\n", offset, *len);
        return -1;
    }

    if((offset + *len) > burn->medium_info.capacity) {
        pr_err("write otp exceed, %lld > %lld\n", (offset + *len), burn->medium_info.capacity);
        return -2;
    }

    memset((uint8_t *)old_data_buffer, 0, sizeof(priv->otp_data));
    pufs_read_otp((uint8_t *)old_data_buffer, sizeof(priv->otp_data), 0); // read all data out

    write_addr = offset / 4;

    do {
        new_data = new_data_buffer[0];

        if(0x00 != new_data) {
            old_data = old_data_buffer[write_addr];

            if(0x00 == old_data) {
                // old is zero, can write any data
                do_write = true;
            } else if(old_data == new_data) {
                // old and new is same, skip it
                do_write = false;
            } else {
                // if new data change bit from 1 to 0, we raise a error
                if(0x00 != check_data(old_data, new_data)) {
                    pr_err("otp can not write 0x%08X to 0x%08X at 0x%x\n", old_data, new_data, write_addr * 4);
                    return (write_addr * 4);
                } else {
                    // if new data change bit from 0 to 1, we write it
#if 0
                    do_write = true;
#else
                    pr_err("otp can write 0x%08X to 0x%08X at 0x%x, but now we don't do this\n", old_data, new_data, write_addr * 4);
                    return (write_addr * 4);
#endif
                }
            }

            if(do_write) {
                pufs_status_t status = pufs_program_otp((uint8_t *)new_data_buffer, 4, write_addr * 4);

                printf("write 0x%08X to 0x%x, result %d, ", new_data, write_addr * 4, status);

                if(SUCCESS != status) {
                    printf("FAILED !!!\n");

                    return write_addr * 4;
                } else {
                    printf("SUCCESS !!!\n");
                }
            }
        }

        write_addr++;
        new_data_buffer++;
    } while(new_data_buffer < new_data_buffer_end);

    // verify
    memset((uint8_t *)old_data_buffer, 0, sizeof(priv->otp_data));
    pufs_read_otp((uint8_t *)old_data_buffer, sizeof(priv->otp_data), 0); // read all data out

    write_addr = offset / 4;
    new_data_buffer = (uint32_t *)buf;

    do {
        new_data = new_data_buffer[0];

        if(0x00 != new_data) {
            old_data = old_data_buffer[write_addr];

            if(old_data != new_data) {
                pr_err("otp verify failed, 0x%08X != 0x%08X at 0x%x\n", old_data, new_data, write_addr * 4);
            }
        }

        write_addr++;
        new_data_buffer++;
    } while(new_data_buffer < new_data_buffer_end);

    return 0;
}

static int otp_destory(struct kburn *burn)
{
    return 0;
}

struct kburn *kburn_otp_probe(void)
{
    struct kburn *burner;
    struct kburn_otp_priv *priv;

    burner = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*burner) + sizeof(*priv));
    if(NULL == burner) {
        pr_err("memaligin failed\n");
        return NULL;
    }
    memset(burner, 0, sizeof(*burner));

    priv = (struct kburn_otp_priv *)((char *)burner + sizeof(*burner));
    memset(priv->otp_data, 0, sizeof(priv->otp_data));

    burner->type = KBURN_MEDIA_OTP;
    burner->dev_priv = (void *)priv;

	burner->get_medium_info = otp_get_medium_info;
	burner->read_medium = otp_read_medium;
	burner->write_medium = otp_write_medium;
    burner->erase_medium = otp_erase_medium;
    burner->destory = otp_destory;

    return burner;
}
