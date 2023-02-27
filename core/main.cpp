#include "main.h"

// static MainProgram main_prog;

/*
    Name mangling:
        extern "C" makes a function-name in C++ have C linkage (compiler does not mangle the name)
        so that client C code can link to (use) your function using a C compatible header file that contains just the declaration of your function.
        Your function definition is contained in a binary format (that was compiled by your C++ compiler)
        that the client C linker will then link to using the C name.

        Since C++ has overloading of function names and C does not, the C++ compiler cannot just use the function name as a unique id to link to,
        so it mangles the name by adding information about the arguments.
        A C compiler does not need to mangle the name since you can not overload function names in C.
        When you state that a function has extern "C" linkage in C++,
        the C++ compiler does not add argument/parameter type information to the name used for linkage.
*/

extern "C" void app_main(void)
{
    // main_prog.run();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
}
