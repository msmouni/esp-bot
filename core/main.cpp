#include "main.h"

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
    SsidPassword wifi_ssid_password = {
        .ssid = "My-SSID",
        .password = "MY-Password",
    };

    Wifi my_wifi = Wifi(wifi_ssid_password);

    my_wifi.init_nvs();
    esp_err_t status = my_wifi.init();

    if (status == ESP_OK)
    {
        // TMP
        while (my_wifi.get_state() != WifiState::ReadyToConnect)
        {
            printf("Init ... %d", (int)my_wifi.get_state());
            vTaskDelay(10); // TMP: TICK
        }

        my_wifi.log("CONNECT...");
        my_wifi.connect();

        // TMP
        while (my_wifi.get_state() != WifiState::Connected)
        {
            vTaskDelay(10); // TMP: TICK
        }
        my_wifi.start_tcp_server();
    }
    else
    {
        my_wifi.log("ERROR INIT");
    }
}
