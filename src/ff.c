/*
 * ngfd - Non-graphical feedback daemon
 *
 * FF (Force Feedback) input device backend.
 * For devices using the kernel FF API (mainly written for Nothing Phone (1) - Spacewar).
 */

#include "implementation.h"

#include <ngf/log.h>

#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>

static int              ff_fd = -1;
static struct ff_effect effect;

static int input_device_supports_ff(const char *devpath)
{
    unsigned char features[1 + FF_MAX / 8] = {0};
    int fd, ret;

    fd = open(devpath, O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return 0;

    ret = ioctl(fd, EVIOCGBIT(EV_FF, sizeof(features)), features);
    close(fd);

    if (ret < 0)
        return 0;

    return (features[FF_RUMBLE / 8] >> (FF_RUMBLE % 8)) & 1;
}

static int find_ff_device(char *out, size_t out_len)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/dev/input");
    if (!dir)
        return -1;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        if (input_device_supports_ff(path)) {
            strncpy(out, path, out_len - 1);
            out[out_len - 1] = '\0';
            closedir(dir);
            return 0;
        }
    }

    closedir(dir);
    return -1;
}

int h_vibrator_open(const NProplist *properties)
{
    char devpath[64];
    (void) properties;

    if (find_ff_device(devpath, sizeof(devpath)) < 0) {
        N_INFO(LOG_CAT "no FF input device found");
        return -1;
    }

    ff_fd = open(devpath, O_RDWR | O_CLOEXEC);
    if (ff_fd < 0) {
        N_INFO(LOG_CAT "failed to open %s", devpath);
        return -1;
    }

    memset(&effect, 0, sizeof(effect));
    effect.type                    = FF_RUMBLE;
    effect.id                      = -1;
    effect.u.rumble.strong_magnitude = 0x6000;
    effect.u.rumble.weak_magnitude   = 0;
    effect.replay.length           = 0;
    effect.replay.delay            = 0;

    if (ioctl(ff_fd, EVIOCSFF, &effect) < 0) {
        N_INFO(LOG_CAT "failed to upload FF effect");
        close(ff_fd);
        ff_fd = -1;
        return -1;
    }

    N_DEBUG(LOG_CAT "opened FF vibrator: %s effect id: %d", devpath, effect.id);
    return 0;
}

void h_vibrator_close(void)
{
    if (ff_fd >= 0) {
        ioctl(ff_fd, EVIOCRMFF, effect.id);
        close(ff_fd);
        ff_fd = -1;
    }
}

void h_vibrator_on(uint32_t timeout_ms)
{
    struct input_event play;

    if (ff_fd < 0)
        return;

    effect.replay.length = (uint16_t)(timeout_ms > 0xFFFF ? 0xFFFF : timeout_ms);
    if (ioctl(ff_fd, EVIOCSFF, &effect) < 0) {
        N_ERROR(LOG_CAT "failed to update FF effect duration");
        return;
    }

    memset(&play, 0, sizeof(play));
    play.type  = EV_FF;
    play.code  = effect.id;
    play.value = 1;

    if (write(ff_fd, &play, sizeof(play)) != sizeof(play))
        N_ERROR(LOG_CAT "failed to write play event");
}

void h_vibrator_off(void)
{
    struct input_event stop;

    if (ff_fd < 0)
        return;

    memset(&stop, 0, sizeof(stop));
    stop.type  = EV_FF;
    stop.code  = effect.id;
    stop.value = 0;

    if (write(ff_fd, &stop, sizeof(stop)) != sizeof(stop))
        N_ERROR(LOG_CAT "failed to write stop event");
}
