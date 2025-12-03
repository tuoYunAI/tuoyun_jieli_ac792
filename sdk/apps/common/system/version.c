#include "system/includes.h"
#include "generic/log.h"
#include "app_config.h"

extern char __VERSION_BEGIN[];
extern char __VERSION_END[];

const char *sdk_version(void)
{
    return "AC792N SDK on branch [release/AC792N_SDK_V3] tag AC792N_SDK_BETA_V3.0.8_2025-10-28";
}

static int app_version_check()
{
    char *version;

    printf("================= SDK Version    %s     ===============\n", sdk_version());
#ifdef CONFIG_WIFI_SOUNDBOX_PROJECT_ENABLE
    printf("================= Media SDK Version %s ===============\n", "AC792N_soundbox_V1.0.0_2025-7-8_21-00");
#endif
    for (version = __VERSION_BEGIN; version < __VERSION_END;) {
        printf("%s\n", version);
        version += strlen(version) + 1;
    }
    puts("=======================================\n");

    return 0;
}
early_initcall(app_version_check);

