#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "../libdoipstack/ne_doip_server.h"

void
signal_exit_handler(int sig)
{
    ne_doip_server_deinit();
    exit(0);
}

//void
//signal_crash_handler(int sig)
//{
//    doip_server_deinit();
//    exit(-1);
//}

int main()
{
    signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGBUS, signal_crash_handler);
    // signal(SIGSEGV, signal_crash_handler);
    // signal(SIGFPE, signal_crash_handler);
    // signal(SIGABRT, signal_crash_handler);

    ne_doip_server_init("/tmp/doip_server_config.xml");
    ne_doip_set_vin_info("IVI88ABCD19800418");
    ne_doip_set_eid_info("180373CB6B36");
    ne_doip_set_gid_info("180373CB6B36");

    sleep(1);
    ne_doip_activation_line_switch_active();

    sleep(1);
    ne_doip_request_ip_addr_assignment("eno1");

    sleep(1);
    ne_doip_powermode_status_change(0x01);

    sleep(1000000);

    ne_doip_server_deinit();

    exit(0);
}
